import os
import shutil
from collections.abc import Callable
from dataclasses import dataclass
from pathlib import Path
from typing import List

from .utils import Color, log, modify_env, run_command

HOME = Path.home()
USR = Path("/usr")

BUILD_TYPE = "Debug"

LLVM_SYS_140_PREFIX = USR / "lib" / "llvm-14"
LLVM_CONFIG_PATH = USR / "bin" / "llvm-config-14"
CARGO_BIN = HOME / ".cargo" / "bin"
LOCAL_BIN = HOME / ".local" / "bin"

EXTERNAL_DIR = Path("external")
LINENOISE_DIR = EXTERNAL_DIR / "linenoise"
QIR_RUNNER_DIR = EXTERNAL_DIR / "qir-runner"
QISKIT_DIR = EXTERNAL_DIR / "qiskit"

TKET_DIR = EXTERNAL_DIR / "tket"
TKET_CONAN_OUT = TKET_DIR / "build" / "tket"
TKET_BUILD_DIR = TKET_CONAN_OUT / "build" / BUILD_TYPE

@dataclass
class RepoInstall:
    github_url: str
    dir: Path
    build_step: Callable | None = None


def install_rust_and_uv():
    log(">>> Installing Rust & uv ...", Color.BLUE)

    # Rust
    # We check shutil.which() (the PATH) AND the absolute path just in case
    # the environment variables haven't been reloaded in this Python run yet.
    if not shutil.which("cargo") and not (CARGO_BIN / "cargo").exists():
        run_command(
            [
                "bash",
                "-c",
                "curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y",
            ]
        )

    # uv
    if (
        not shutil.which("uv")
        and not (CARGO_BIN / "uv").exists()
        and not (LOCAL_BIN / "uv").exists()
    ):
        run_command(["bash", "-c", "curl -LsSf https://astral.sh/uv/install.sh | sh"])


def install_conan():
    log(">>> Installing Conan ...", Color.BLUE)

    if not shutil.which("conan"):
        run_command(["pipx", "install", "conan"])

    conan2_profile_path = Path.home() / ".conan2" / "profiles" / "default"

    if not conan2_profile_path.exists():
        log(">>> Generating default Conan profile...", Color.BLUE)
        run_command(["bash", "-c", "conan profile detect"])
    else:
        log(">>> Conan default profile already exists, skipping generation.")


def install_deps():
    log(">>> Installing system dependencies ... ", Color.BLUE)

    run_command(["sudo", "apt-get", "update"])

    packages = [
        "default-jre",
        "clang",
        "cmake",
        "graphviz",
        "git",
        "curl",
        "build-essential",
        "python3-venv",
        "gdb",
        "llvm-14",
        "llvm-14-dev",
        "libllvm14",
        "libpolly-14-dev",
        "zlib1g-dev",
        "libxml2-dev",
        "libncurses-dev",
        "libffi-dev",
        "gcovr",
        "pipx",
    ]

    run_command(["sudo", "apt-get", "install", "-y"] + packages)

    install_rust_and_uv()
    install_conan()


def install_repos():
    log(">>> Installing and building external dependencies ...", Color.BLUE)
    EXTERNAL_DIR.mkdir(exist_ok=True)

    def clone_externals(externals: List[RepoInstall]):
        for ext in externals:
            if not ext.dir.exists():
                log("Cloning " + ext.github_url)
                run_command(["git", "clone", ext.github_url, str(ext.dir)])

            if ext.build_step:
                ext.build_step()

    def build_qir_runner():
        log("Building qir-runner ...")

        llvm_lib_dir = LLVM_SYS_140_PREFIX / "lib"

        env = modify_env(
            {
                "LLVM_SYS_140_PREFIX": [LLVM_SYS_140_PREFIX],
                "LLVM_CONFIG_PATH": [LLVM_CONFIG_PATH],
                "RUSTFLAGS": f"-L native={llvm_lib_dir}",
            }
        )

        run_command(["cargo", "build", "--release"], cwd=str(QIR_RUNNER_DIR), env=env)

    def build_tket():
        log(">>> Building tket C++ core with Conan (Coverage Enabled) ...")

        env = modify_env(
            {"PATH": [LOCAL_BIN], "CXXFLAGS": "-fprofile-arcs -ftest-coverage", "LDFLAGS": "-lgcov"}
        )

        # Conan handles all the micro-libraries and the main core automatically
        # We just pass the exact flags from the README for coverage: https://github.com/Quantinuum/tket/blob/main/tket/README.md
        run_command(
            [
                "conan",
                "build",
                "tket",
                "--user=tket",
                "--channel=stable",
                "-s",
                f"build_type={BUILD_TYPE}",
                "--build=tket",
                "--build=missing",
                "-o",
                "boost/*:header_only=True",
                "-o",
                "tket/*:profile_coverage=True",
                # "-o", "test-tket/*:with_coverage=True",
                # "-o", "with_test=True",
                "-of",
                "build/tket",
            ],
            cwd=str(TKET_DIR),
            env=env,
        )

        # if TKET_BUILD_DIR.exists():
            # run_command(["sudo", "make", "install"], cwd=str(TKET_BUILD_DIR))

    clone_externals(
        [
            RepoInstall(
                github_url="https://github.com/CQCL/qir-runner.git",
                dir=QIR_RUNNER_DIR,
                build_step=build_qir_runner,
            ),
            RepoInstall(
                github_url="https://github.com/CQCL/tket.git", dir=TKET_DIR, build_step=build_tket
            ),
            RepoInstall(
                github_url="https://github.com/Qiskit/qiskit.git",
                dir=QISKIT_DIR,
            ),
            RepoInstall(
                github_url="https://github.com/antirez/linenoise.git",
                dir=LINENOISE_DIR,
            ),
        ]
    )


def sync_python_environment():
    log(">>> Syncing Python Environment ...", Color.BLUE)

    llvm_lib_dir = LLVM_SYS_140_PREFIX / "lib"

    env = modify_env(
        {
            "TKET_DIR": [TKET_BUILD_DIR.resolve()],
            "LLVM_SYS_140_PREFIX": [LLVM_SYS_140_PREFIX],
            "LLVM_CONFIG_PATH": [LLVM_CONFIG_PATH],
            "RUSTFLAGS": f"-L native={llvm_lib_dir}",
        }
    )

    if BUILD_TYPE == "Debug":
        env["CFLAGS"] = "-fprofile-arcs -ftest-coverage -O0 -g"
        env["CXXFLAGS"] = "-fprofile-arcs -ftest-coverage -O0 -g"
        env["LDFLAGS"] = "-fprofile-arcs -ftest-coverage -lgcov"
    else:
        env["CFLAGS"] = "-O3"
        env["CXXFLAGS"] = "-O3"

    run_command(["uv", "sync", "--reinstall-package", "pytket"], env=env)


def setup_ci_env():
    # In GitHub Actions, you export variables by writing them to specific files
    github_env = os.environ.get("GITHUB_ENV")
    github_path = os.environ.get("GITHUB_PATH")

    if github_env:
        log("Exporting LLVM prefix to GITHUB_ENV...")
        with open(github_env, "a") as f:
            f.write(f"LLVM_SYS_140_PREFIX={LLVM_SYS_140_PREFIX}\n")

    if github_path:
        log("Exporting cargo bin to GITHUB_PATH...")
        with open(github_path, "a") as f:
            f.write(f"{CARGO_BIN}\n")


if __name__ == "__main__":
    install_deps()
    install_repos()

    sync_python_environment()
    setup_ci_env()

    print("Setup complete!", Color.GREEN)
    print("Source .venv with source .venv/bin/activate")
    print("Run with uv run -m scripts.run", Color.BLUE)
