import argparse
import os
import shutil
from dataclasses import dataclass
from pathlib import Path
from utils import Color, log, modify_env, run_command

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
    url: str
    dest_dir: Path
    engine_dep: bool = False


REPOS = [
    Repo("https://github.com/CQCL/tket.git", TKET_DIR),
    Repo("https://github.com/Qiskit/qiskit.git", QISKIT_DIR),
    Repo("https://github.com/antirez/linenoise.git", LINENOISE_DIR, engine_dep=True),
]


def clone_repos(dev: bool):
    log("Cloning repos", Color.BLUE)

    for repo in REPOS:
        if not repo.dest_dir.exists() and (dev or repo.engine_dep):
            log("Cloning " + repo.url)
            run_command(["git", "clone", repo.url, str(repo.dest_dir)])


def parse():
    parser = argparse.ArgumentParser(description="QuteFuzz setup script")
    parser.add_argument(
        "--dev",
        action="store_true",
        help="Setup environment in dev mode (Install and build tket from source)",
    )
    return parser.parse_args()


def check_conan_profile():
    conan2_profile_path = Path.home() / ".conan2" / "profiles" / "default"

    if not conan2_profile_path.exists():
        log("Generating default Conan profile...", Color.BLUE)
        run_command(["bash", "-c", "conan profile detect"])

        log("Adding Quantinuum tket-libs remote...", Color.BLUE)
        run_command(
            [
                "bash",
                "-c",
                "conan remote add tket-libs "
                "https://quantinuumsw.jfrog.io/artifactory/api/conan/tket1-libs --index 0",
            ]
        )
    else:
        log("Conan default profile already exists, skipping generation.")


def install_rust_and_uv():
    log("Installing Rust & uv ...", Color.BLUE)

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


def install_cudaq():
    log("Installing CUDA-Q (nvq++) ...", Color.BLUE)
    
    cudaq_dir = HOME / ".cudaq"
    nvq_bin = cudaq_dir / "bin" / "nvq++"

    if nvq_bin.exists():
        log("CUDA-Q is already installed, skipping.", Color.GREEN)
        return

    installer_url = "https://github.com/NVIDIA/cuda-quantum/releases/latest/download/install_cuda_quantum_cu12.x86_64"
    installer_script = Path("install_cuda_quantum.sh")

    log("Downloading CUDA-Q installer ...", Color.YELLOW)
    run_command(["curl", "-sL", "-o", str(installer_script), installer_url])

    log("Running silent installer ...", Color.YELLOW)
    run_command(["bash", str(installer_script), "--accept", "--", "--installpath", str(cudaq_dir)])

    if installer_script.exists():
        installer_script.unlink()

    bashrc = HOME / ".bashrc"
    source_line = f"source {cudaq_dir}/set_env.sh"
    
    if bashrc.exists():
        content = bashrc.read_text()
        if source_line not in content:
            with open(bashrc, "a") as f:
                f.write("\n# CUDA-Q Environment Setup\n")
                f.write(f"{source_line}\n")

            log("Added nvq++ to ~/.bashrc", Color.GREEN)


def install_deps():
    log("Installing system dependencies ... ", Color.BLUE)

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
    install_cudaq()


def build_tket_with_coverage():
    log("Building tket C++ core with Conan (Coverage Enabled) ...", Color.BLUE)

    shared_opts = [
        "--user=tket",
        "--channel=stable",
        "-s",
        "build_type=Debug",
        "-o",
        "boost/*:header_only=True",
        "-o",
        "tket/*:shared=True",
        "-o",
        "tklog/*:shared=True",
        "-o",
        "tket/*:profile_coverage=True",
        "-of",
        "build/tket",
    ]

    log("Building tket with coverage flags ...", Color.YELLOW)
    run_command(
        ["uv", "run", "conan", "build", "tket", "--build=missing"] + shared_opts, cwd=str(TKET_DIR)
    )

    # register the built artifacts into the local Conan cache so that pytket's build can find the
    # instrumented library
    log("Exporting instrumented tket to Conan cache ...", Color.YELLOW)
    run_command(
        ["uv", "run", "conan", "export-pkg", "tket"]
        + shared_opts
        + ["-tf", ""],  # -tf "" skips running tests during export
        cwd=str(TKET_DIR),
    )

    log("tket coverage build complete.", Color.GREEN)
    log(f'    use `find {TKET_BUILD_DIR} -name "*.gcno"` to find .gcno files', Color.GREEN)


def inject_pytket_into_venv():
    log("Injecting instrumented pytket into virtual environment...", Color.BLUE)
    pytket_dir = TKET_DIR / "pytket"

    # dynamically patch tket/pytket/setup.py to inherit same options and settings

    # patching like this ensures that `conan create` used in the tket/pytket/setup.py
    # finds the same package ID in the conan cache as the instrumented libtket.so
    # also add -c flags to pass --coverage to linker and compiler such that all coverage
    # symbols are resolved correctly
    setup_file = pytket_dir / "setup.py"
    setup_content = setup_file.read_text()

    if '                "-s", "build_type=Debug",\n' not in setup_content:
        # add settings, options, and compiler coverage flags
        patch = (
            '"--build=missing",\n'
            '                "-s", "build_type=Debug",\n'
            '                "-o", "tket/*:profile_coverage=True",\n'
            '                "-c", "tools.build:cxxflags=[\\"--coverage\\"]",\n'
            '                "-c", "tools.build:cflags=[\\"--coverage\\"]",\n'
        )
        setup_content = setup_content.replace('"--build=missing",', patch)
        setup_file.write_text(setup_content)
        log("Patched pytket/setup.py with Debug and coverage flags.", Color.YELLOW)

    # add module linker flags to CMakeLists.txt
    cmake_file = pytket_dir / "CMakeLists.txt"

    if cmake_file.exists():
        cmake_content = cmake_file.read_text()

        if (
            'set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} --coverage")\n'
            not in cmake_content
        ):
            # Force coverage into the module linker
            module_patch = (
                'set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} --coverage")\n'
                'set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --coverage")\n'
            )
            cmake_file.write_text(module_patch + cmake_content)
            log("   Patched CMakeLists.txt for MODULE linker flags.", Color.YELLOW)

    env = modify_env({"SETUPTOOLS_SCM_PRETEND_VERSION": "2.16.0"})

    # manually creating the _version.py file to appease scm
    version_file = pytket_dir / "pytket" / "_version.py"
    version_file.write_text('__version__ = "2.16.0"\n')

    log("Cleaning stale build directories...", Color.YELLOW)
    for stale_dir in ["build", "pytket.egg-info"]:
        stale_path = pytket_dir / stale_dir
        if stale_path.exists():
            shutil.rmtree(stale_path)

    log("Reinstalling with instrumented libtket.so ...", Color.YELLOW)
    run_command(
        ["uv", "pip", "install", "--reinstall", "--no-cache", "--no-build-isolation", "."],
        env=env,
        cwd=pytket_dir,
    )


def build_pytket():
    build_tket_with_coverage()
    inject_pytket_into_venv()


def build_external_deps():
    log("Building external dependencies ...", Color.BLUE)
    EXTERNAL_DIR.mkdir(exist_ok=True)

    build_pytket()


def setup_ci_env():
    # In GitHub Actions, you export variables by writing them to specific files
    github_env = os.environ.get("GITHUB_ENV")
    github_path = os.environ.get("GITHUB_PATH")

    if github_env:
        log(" Exporting LLVM prefix to GITHUB_ENV...")
        with open(github_env, "a") as f:
            f.write(f"LLVM_SYS_140_PREFIX={LLVM_SYS_140_PREFIX}\n")

    if github_path:
        log("Exporting cargo bin to GITHUB_PATH...")
        with open(github_path, "a") as f:
            f.write(f"{CARGO_BIN}\n")

        log("Exporting CUDA-Q bin to GITHUB_PATH...")
        cudaq_bin = HOME / ".cudaq" / "bin"
        if cudaq_bin.exists():
            with open(github_path, "a") as f:
                f.write(f"{cudaq_bin}\n")


if __name__ == "__main__":
    parser = parse()

    install_deps()

    clone_repos(parser.dev)

    log("Running initial uv sync to prepare the venv...", Color.BLUE)
    # these flags are needed for cargo builds to work
    env = modify_env(
        {
            "LLVM_SYS_140_PREFIX": [LLVM_SYS_140_PREFIX],
            "LLVM_CONFIG_PATH": [LLVM_CONFIG_PATH],
            "RUSTFLAGS": f"-L native={LLVM_LIB_DIR}",
        }
    )

    run_command(["uv", "sync"], env=env)

    if parser.dev:
        check_conan_profile()
        build_external_deps()

    setup_ci_env()

    print("Setup complete!", Color.GREEN)
