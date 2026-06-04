import subprocess

from params import CUDAQ_DIR, EXTERNAL_DIR, OUTPUT_DIR


def collect():
    llvm_profdata = (
        EXTERNAL_DIR / "cuda-quantum" / "tpls" / "llvm" / "build" / "bin" / "llvm-profdata"
    )
    llvm_cov = EXTERNAL_DIR / "cuda-quantum" / "tpls" / "llvm" / "build" / "bin" / "llvm-cov"

    cudaq_output_dir = OUTPUT_DIR / "cudaq"
    profdata_file = str(cudaq_output_dir / "coverage.profdata")

    cmd = [
        str(llvm_profdata),
        "merge",
        "-sparse",
    ]

    for path in cudaq_output_dir.rglob("*.profraw"):
        cmd.append(str(path))

    cmd += ["-o", profdata_file]

    subprocess.run(cmd)

    cudaq_bin_dir = CUDAQ_DIR / "build" / "bin"

    cmd = [
        str(llvm_cov),
        "show",
        f"--instr-profile={profdata_file}",
        "--format=html",
        f"--output-dir={cudaq_output_dir / 'coverage_report'}",
        "--coverage-watermark=75,50",
    ]

    objs = ["cudaq-quake", "cudaq-opt", "cudaq-translate"]

    for obj in objs:
        cmd.append(f"--object={str(cudaq_bin_dir / obj)}")

    subprocess.run(cmd)


if __name__ == "__main__":
    collect()
