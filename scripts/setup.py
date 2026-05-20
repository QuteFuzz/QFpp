import argparse
import os
import shutil
import subprocess
import sys
from argparse import Namespace
from collections.abc import Callable
from dataclasses import dataclass
from pathlib import Path
from typing import List

from params import CUDAQ_DIR, EXTERNAL_DIR, LINENOISE_DIR, TKET_DIR
from utils import Color, log, modify_env, run_command

HOME = Path.home()
USR = Path("/usr")

CARGO_BIN = HOME / ".cargo" / "bin"
LOCAL_BIN = HOME / ".local" / "bin"

TKET_CONAN_OUT = TKET_DIR / "build" / "tket"
TKET_BUILD_DIR = TKET_CONAN_OUT / "build" / "Debug"

REQUIRED_PACKAGES = [
    "default-jre",
    "clang-22",
    "cmake",
    "graphviz",
    "git",
    "curl",
    "build-essential",
    "python3-venv",
    "gdb",
    "lld",
    "ninja-build",
    "llvm-22-dev",
    "libmlir-22-dev",
    "mlir-22-tools",
    "libclang-22-dev",
    "libopenblas-dev",
    "libedit-dev",
    "libcurl4-openssl-dev",
    "libpolly-14-dev",
    "zlib1g-dev",
    "libxml2-dev",
    "libncurses-dev",
    "libffi-dev",
    "gcovr",
    "libzstd-dev",  # LLVM 22 relies on Zstandard compression
    "pkg-config",  # Required for CMake to find FFI and zstd
]


@dataclass
class Repo:
    name: str
    url: str
    dest_dir: Path
    build_func: Callable[[bool], None] | None = None
    engine_dep: bool = False


REPOS = [
    Repo(
        "tket",
        "https://github.com/CQCL/tket.git",
        TKET_DIR,
        lambda with_coverage: build_pytket(with_coverage),
    ),
    Repo(
        "cuda-quantum",
        "https://github.com/NVIDIA/cuda-quantum.git",
        CUDAQ_DIR,
        lambda with_coverage: build_cudaq(with_coverage),
    ),
    Repo("linenoise", "https://github.com/antirez/linenoise.git", LINENOISE_DIR, engine_dep=True),
]


def verify_required_tools():
    missing_pkgs = set()

    for pkg in REQUIRED_PACKAGES:
        res = subprocess.run(
            ["dpkg", "-s", pkg], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
        )

        if res.returncode != 0:
            missing_pkgs.add(pkg)

    if missing_pkgs:
        log("\nFatal Error: Missing system dependencies.", Color.RED)
        print("Please install them by running the following command:\n")

        apt_cmd = f"sudo apt-get update && sudo apt-get install -y {' '.join(missing_pkgs)}"

        if os.environ.get("GITHUB_PATH"):
            run_command(apt_cmd)

        print(f"    {apt_cmd}\n")

        print("After installation, re-run this setup script.")
        sys.exit(1)  # Gracefully halt the script


def clone_repos(libs: List[str]):
    log("Cloning repos", Color.BLUE)

    for repo in REPOS:
        if not repo.dest_dir.exists() and ((repo.name in libs) or repo.engine_dep):
            log("Cloning " + repo.url)
            cmd = ["git", "clone", repo.url, str(repo.dest_dir)]
            run_command(cmd)


def parse():
    parser = argparse.ArgumentParser(description="QuteFuzz setup script")
    parser.add_argument(
        "--libs",
        nargs="+",
        required=False,
        choices=[repo.name for repo in REPOS],
        default=[],
        help="List of external libraries to build",
    )
    parser.add_argument(
        "--cov", action="store_true", help="Compile cudaq with coverage information"
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


def install_deps():
    verify_required_tools()

    log("Installing Rust and UV dependencies ... ", Color.BLUE)
    install_rust_and_uv()


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


def build_pytket(with_coverage: bool):
    if with_coverage:
        check_conan_profile()
        build_tket_with_coverage()
        inject_pytket_into_venv()


def build_bundled_llvm():
    log("Building bundled LLVM/MLIR (this will take a while)...", Color.BLUE)
    llvm_src = CUDAQ_DIR / "tpls" / "llvm"
    llvm_build = llvm_src / "build"

    if llvm_build.exists():
        log("Bundled LLVM already built, skipping.", Color.YELLOW)
        return

    llvm_build.mkdir(exist_ok=True)

    run_command(
        [
            "cmake",
            str(llvm_src / "llvm"),
            "-G",
            "Ninja",
            f"-DCMAKE_C_COMPILER={USR / 'bin' / 'clang-22'}",
            f"-DCMAKE_CXX_COMPILER={USR / 'bin' / 'clang++-22'}",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DLLVM_ENABLE_PROJECTS=clang;mlir",
            "-DLLVM_TARGETS_TO_BUILD=X86;NVPTX",
            "-DLLVM_ENABLE_ASSERTIONS=OFF",
            "-DLLVM_INSTALL_UTILS=ON",
            "-DLLVM_ENABLE_ZLIB=ON",
            "-DLLVM_ENABLE_ZSTD=ON",
            "-DLLVM_ENABLE_TERMINFO=OFF",
            "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
        ],
        cwd=str(llvm_build),
    )

    run_command(["ninja"], cwd=str(llvm_build))
    log("Bundled LLVM build complete.", Color.GREEN)


def build_nvq(with_coverage: bool):
    log("Building nvq++ with coverage", Color.BLUE)
    build_dir = CUDAQ_DIR / "build"

    if build_dir.exists():
        shutil.rmtree(build_dir)

    build_dir.mkdir(exist_ok=True)

    llvm_build = CUDAQ_DIR / "tpls" / "llvm" / "build"

    cmd = [
        "cmake",
        "..",
        "-G",
        "Ninja",
        f"-DCMAKE_C_COMPILER={USR / 'bin' / 'clang-22'}",
        f"-DCMAKE_CXX_COMPILER={USR / 'bin' / 'clang++-22'}",
        "-DCMAKE_BUILD_TYPE=Release",
        f"-DLLVM_DIR={llvm_build / 'lib' / 'cmake' / 'llvm'}",
        f"-DMLIR_DIR={llvm_build / 'lib' / 'cmake' / 'mlir'}",
        f"-DClang_DIR={llvm_build / 'lib' / 'cmake' / 'clang'}",
        "-DCUDAQ_BUILD_TESTS=OFF",
        f"-DZLIB_LIBRARY={USR / 'lib' / 'x86_64-linux-gnu' / 'libz.so'}",
        "-DZLIB_USE_STATIC_LIBS=OFF",
        "-DCMAKE_COMPILE_WARNING_AS_ERROR=OFF",
    ]

    if with_coverage:
        link_flags = "-fprofile-instr-generate"
        cmd.extend(
            [
                "-DCUDAQ_ENABLE_CC=ON",
                f"-DCMAKE_EXE_LINKER_FLAGS={link_flags}",
                f"-DCMAKE_SHARED_LINKER_FLAGS={link_flags}",
                f"-DCMAKE_MODULE_LINKER_FLAGS_RELEASE={link_flags}",
            ]
        )

    run_command(cmd, cwd=str(build_dir))

    run_command(["ninja"], cwd=str(build_dir))


def build_cudaq(with_coverage: bool):
    # init only the llvm submodule — not all of them (too large)
    log("Initializing tpls/llvm submodule...", Color.BLUE)
    run_command(["git", "submodule", "update", "--init", "tpls/llvm"], cwd=str(CUDAQ_DIR))

    build_bundled_llvm()
    build_nvq(with_coverage)


def build_external_deps(parser: Namespace):
    log("Building external dependencies ...", Color.BLUE)
    EXTERNAL_DIR.mkdir(exist_ok=True)

    for repo in REPOS:
        if repo.name in parser.libs and repo.build_func is not None:
            repo.build_func(parser.cov)


def setup_ci_env():
    # In GitHub Actions, you export variables by writing them to specific files
    github_path = os.environ.get("GITHUB_PATH")

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

    if "tket" in parser.libs and parser.cov:
        log("Cannot run instrumented tket compiler", Color.RED)
        sys.exit(-1)

    install_deps()

    clone_repos(parser.libs)

    log("Running initial uv sync to prepare the venv...", Color.BLUE)

    run_command(["uv", "sync"])

    build_external_deps(parser)

    setup_ci_env()

    print("Setup complete!", Color.GREEN)
