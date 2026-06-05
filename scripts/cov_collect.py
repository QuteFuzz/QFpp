import subprocess

from params import CUDAQ_DIR, EXTERNAL_DIR, OUTPUT_DIR


def collect():
    llvm_profdata = (
        EXTERNAL_DIR / "cuda-quantum" / "tpls" / "llvm" / "build" / "bin" / "llvm-profdata"
    )
    llvm_cov = EXTERNAL_DIR / "cuda-quantum" / "tpls" / "llvm" / "build" / "bin" / "llvm-cov"

    cudaq_output_dir = OUTPUT_DIR / "cudaq"
    profdata_file = str(cudaq_output_dir / "coverage.profdata")

    merge_cmd = [
        str(llvm_profdata),
        "merge",
        "-sparse",
    ]

    for path in cudaq_output_dir.rglob("*.profraw"):
        merge_cmd.append(str(path))

    merge_cmd += ["-o", profdata_file]

    subprocess.run(merge_cmd, check=True)

    cudaq_bin_dir = CUDAQ_DIR / "build" / "bin"
    cudaq_lib_dir = CUDAQ_DIR / "cudaq" / "lib"
    cudaq_inc_dir = CUDAQ_DIR / "cudaq" / "include"
    eigen_dir = CUDAQ_DIR / "tpls" / "eigen"

    objs = ["cudaq-quake", "cudaq-opt", "cudaq-translate"]

    shared_parts = [
        str(llvm_cov),
        "show",
        f"--instr-profile={profdata_file}",
        "--format=html",
        "--coverage-watermark=75,50",
        str(cudaq_bin_dir / objs[0]),
    ]

    for obj in objs[1:]:
        shared_parts.append(f"--object={str(cudaq_bin_dir / obj)}")

    # full report
    full_report_cmd = shared_parts + [f"--output-dir={cudaq_output_dir / 'full_coverage_report'}"]
    subprocess.run(full_report_cmd, check=True)

    # cudaq report
    whitelist = [
        str(cudaq_lib_dir / "Frontend/"),
        str(cudaq_lib_dir / "Optimizer/"),
        str(cudaq_inc_dir / "cudaq/Frontend/"),
        str(cudaq_inc_dir / "cudaq/Optimizer/"),
    ]

    cudaq_report_cmd = (
        shared_parts
        + [f"--output-dir={cudaq_output_dir / 'cudaq_coverage_report'}", "--"]
        + whitelist
    )
    subprocess.run(cudaq_report_cmd, check=True)

    whitelist = [str(eigen_dir)]
    eigen_report_cmd = (
        shared_parts
        + [f"--output-dir={cudaq_output_dir / 'eigen_coverage_report'}", "--"]
        + whitelist
    )
    subprocess.run(eigen_report_cmd, check=True)


if __name__ == "__main__":
    collect()
