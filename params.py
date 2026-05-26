import os
from pathlib import Path

CWD = Path.cwd()
HOME = Path.home()

USR = Path("/usr")

EXTERNAL_DIR = CWD / Path("external")
BUILD_DIR = CWD / Path("build")
OUTPUT_DIR = CWD / Path("outputs")

LINENOISE_DIR = EXTERNAL_DIR / "linenoise"
CUDAQ_DIR = EXTERNAL_DIR / "cuda-quantum"
TKET_DIR = EXTERNAL_DIR / "tket"

CPU_COUNT = os.cpu_count()
