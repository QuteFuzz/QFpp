import os
from pdb import run
import shutil
from collections.abc import Callable
from dataclasses import dataclass
from pathlib import Path
from typing import List

from .utils import Color, log, modify_env, run_command

HOME = Path.home()
USR = Path("/usr")

LLVM_SYS_140_PREFIX = USR / "lib" / "llvm-14"
LLVM_LIB_DIR = LLVM_SYS_140_PREFIX / "lib"
LLVM_CONFIG_PATH = USR / "bin" / "llvm-config-14"
CARGO_BIN = HOME / ".cargo" / "bin"
LOCAL_BIN = HOME / ".local" / "bin"

EXTERNAL_DIR = Path("external")
LINENOISE_DIR = EXTERNAL_DIR / "linenoise"
QISKIT_DIR = EXTERNAL_DIR / "qiskit"

TKET_DIR = EXTERNAL_DIR / "tket"
TKET_CONAN_OUT = TKET_DIR / "build" / "tket"
TKET_BUILD_DIR = TKET_CONAN_OUT / "build" / "Debug"

@dataclass
class Repo:
    url : str
    dest_dir  : Path

REPOS = [
    Repo("https://github.com/CQCL/tket.git", TKET_DIR),
    Repo("https://github.com/Qiskit/qiskit.git", QISKIT_DIR),
    Repo("https://github.com/antirez/linenoise.git",LINENOISE_DIR)
]


def clone_repos():
    log(">>> Cloning repos", Color.BLUE)

    for repo in REPOS:
        if not repo.dest_dir.exists():
            log("Cloning " + repo.url)
            run_command(["git", "clone", repo.url, str(repo.dest_dir)])

def check_conan_profile():
    conan2_profile_path = Path.home() / ".conan2" / "profiles" / "default"

    if not conan2_profile_path.exists():
        log(">>> Generating default Conan profile...", Color.BLUE)
        run_command(["bash", "-c", "conan profile detect"])

        log(">>> Adding Quantinuum tket-libs remote...", Color.BLUE)
        run_command([
            "bash", "-c", 
            "conan remote add tket-libs https://quantinuumsw.jfrog.io/artifactory/api/conan/tket1-libs --index 0"
        ])
    else:
        log(">>> Conan default profile already exists, skipping generation.")

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
    ]

    run_command(["sudo", "apt-get", "install", "-y"] + packages)

    install_rust_and_uv()

def build_tket_with_coverage():
    log(">>> Building tket C++ core with Conan (Coverage Enabled) ...")

    shared_opts = [
        "--user=tket", "--channel=stable",
        "-s", "build_type=Debug",
        "-o", "boost/*:header_only=True",
        "-o", "tket/*:profile_coverage=True",
        "-of", "build/tket",
    ]

    log(">>> Building tket with coverage flags ...", Color.YELLOW)
    run_command(
        ["conan", "build", "tket", "--build=missing"] + shared_opts,
        cwd=str(TKET_DIR)
    )
    
    log(">>> tket coverage build complete.", Color.GREEN)
    log(f"    use `find {TKET_BUILD_DIR} -name \"*.gcno\"` to find .gcno files", Color.GREEN)


def inject_pytket_into_venv():
    log(">>> Injecting instrumented pytket into virtual environment...", Color.BLUE)
    pytket_dir = TKET_DIR / "pytket"

    # manually creating the _version.py file to appease scm
    version_file = pytket_dir / "pytket" / "_version.py"
    version_file.write_text('__version__ = "2.16.0"\n')
    
    run_command(
        [
            "uv", "pip", "install", "--reinstall", "--no-build-isolation", "."
        ],
        env=env,
        cwd=pytket_dir
    )

def build_pytket():
    build_tket_with_coverage()
    # inject_pytket_into_venv()

def build_external_deps():
    log(">>> Building external dependencies ...", Color.BLUE)
    EXTERNAL_DIR.mkdir(exist_ok=True)

    build_pytket()


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
    clone_repos()

    log(">>> Running initial uv sync to prepare the venv...", Color.BLUE)
    # these flags are needed for cargo builds to work
    env = modify_env(
        {
            "LLVM_SYS_140_PREFIX": [LLVM_SYS_140_PREFIX],
            "LLVM_CONFIG_PATH": [LLVM_CONFIG_PATH],
            "RUSTFLAGS": f"-L native={LLVM_LIB_DIR}",
        }
    )

    run_command(["uv", "sync"], env=env)
    run_command(["bash", "-c", "source .venv/bin/activate"])

    check_conan_profile()

    build_external_deps()

    setup_ci_env()

    print("Setup complete!", Color.GREEN)

    print("\nRun tests with .venv/bin/python3 test.py NOT uv run", Color.YELLOW)
