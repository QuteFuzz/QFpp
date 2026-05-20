from pathlib import Path

CWD = Path.cwd()

EXTERNAL_DIR = CWD / Path("external")
BUILD_DIR = CWD / Path("build")
OUTPUT_DIR = CWD / Path("outputs")

LINENOISE_DIR = EXTERNAL_DIR / "linenoise"
CUDAQ_DIR = EXTERNAL_DIR / "cuda-quantum"
TKET_DIR = EXTERNAL_DIR / "tket"
