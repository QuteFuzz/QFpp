from pytket import Circuit
from pytket.passes import FullPeepholeOptimise

def main():
    print("1. Initializing Circuit...")
    # Create a simple Bell state circuit
    circ = Circuit(2, 2)
    circ.H(0).CX(0, 1)
    circ.measure_all()
    
    print("2. Running C++ Optimization Pass...")
    # This triggers the heavy C++ core logic
    success = FullPeepholeOptimise().apply(circ)
    
    if success:
        print(">>> SUCCESS: Circuit optimized via C++ backend!")
    else:
        print(">>> Circuit ran, but no optimization was needed.")

    # import ctypes
    # from pathlib import Path

    # print("Forcing gcov dump...")
    # try:
    #     # Find the exact .so file we know contains the __gcov_dump symbol
    #     so_path = list(Path(".venv").rglob("circuit.abi3.so"))[0]
        
    #     # Load the library into ctypes
    #     tket_lib = ctypes.CDLL(str(so_path))
        
    #     # Manually execute the coverage flush!
    #     tket_lib.__gcov_dump()
    #     print("Dump successful!")
    # except Exception as e:
    #     print(f"Failed to dump coverage: {e}")

if __name__ == "__main__":
    main()

import ctypes
from pathlib import Path

# ... your circuit code ...

print("\n--- Forcing core gcov dump ---")
try:
    # Point directly to the newly built libtket.so
    lib_path = Path("external/tket/build/tket/build/Debug/libtket.so").resolve()
    tket_lib = ctypes.CDLL(str(lib_path))
    tket_lib.__gcov_dump()
    print("Dump successful!")
except Exception as e:
    print(f"FAILED: {e}")