import os
from pathlib import Path

CWD = Path.cwd()
HOME = Path.home()

USR = Path("/usr")

EXTERNAL_DIR = CWD / Path("external")
BUILD_DIR = CWD / Path("build")
OUTPUT_DIR = CWD / Path("outputs")
NIGHTLY_DIR = Path("nightly_results")

LINENOISE_DIR = EXTERNAL_DIR / "linenoise"
CUDAQ_DIR = EXTERNAL_DIR / "cuda-quantum"
TKET_DIR = EXTERNAL_DIR / "tket"

CPU_COUNT = os.cpu_count()

SIMULATION_CAP = {
    "pytket": 64,
    "qiskit": 64,
    "cirq": 8,
    "pennylane": 64,
    "cudaq": 64,
    "qasm2": 64,
    "guppy": 64,
}

FUZZER_EXECUTABLE = "./qf"
ENTRY_POINT = "program"
