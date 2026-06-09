# qf++

<p align="center" width="100%">
    <img width="25%" src="etc/qutefuzz.png">

[![Unitary Foundation](https://img.shields.io/badge/Supported%20By-UNITARY%20FOUNDATION-red.svg?style=for-the-badge)](https://unitary.foundation)

qf++ is a framework for quantum compilers. It generates structurally valid quantum programs across multiple quantum software stacks, then differentially tests them by comparing circuit simulation outputs across compiler optimisation levels. 

## Quick start

```sh
# 1. Setup
python3 -m scripts.setup

# 2. Run CI pipeline (10 circuits per grammar)
uv run -m scripts.qf --num-tests 10

# 3. Run nightly (1200 circuits, saves interesting ones)
uv run -m scripts.qf --nightly --num-tests 1200 --grammars pytket qiskit

# 4. Use the interactive fuzzer REPL directly
./build/qf
> pytket program   # set grammar + entry point
> 5               # generate 5 circuits
> quit
```

To run inside a separate environment, pull the docker image using
```sh
docker pull ghcr.io/qutefuzz/qutefuzz-env:latest
```
 
If you prefer to build the image from sratch, run 
```sh
./scripts/docker/build.sh
```

then
```sh
./scripts/docker/run.sh
```

to create a docker container and start a shell inside it. From there, run setup commands as normal.

## Install external libs

To install external repos

```sh
python3 -m scripts.setup --libs {EXTERNAL LIBS}
```

To clone and build `cuda-quantum` from source without coverage collection enabled
```sh
python3 -m scripts.setup --libs cuda-quantum
```

To clone and build from source with coverage collection enabled
```sh
python3 -m scripts.setup --cov
```

**Note:**

- Run script is not set up to run fuzzing campaigns on instrumented compilers, due to the large volumes of data that would be dumped, and is not collected and deleted in batches. The coverage flag is more for development testing to see how well the test cases stress test the compiler. Currently, only CUDAQ compiler can be instrumented.

- If `conan` command fails, this is because the run needs to happen within the virtual env. Source first with 
    
    ```sh
    source .venv/bin./activate
    ```

    then re-run setup command.

- Make sure that any time you use `uv run`, it is with the `--no-sync` flag, or use `python3` directly. This is because the setup script in dev mode "tampers with" the .venv, uv sees that
and tries to sync everything to match the lockfile. As a result, it reinstalls the release, uninstrumented pytket version from PyPI. 

## Supported quantum frameworks

Templates written in `templates` directory. Corresponding testing harnesses written in `diff_testing`.

## Bugs found

| QSS | Bug in | Status |
|-----|------|-------|
| Pytket | [optimiser](https://github.com/Quantinuum/tket/issues/2109) | fixed |
| Pytket | [optimiser](https://github.com/Quantinuum/tket2/issues/1417) | ack |
| CUDA-Q | [parser](https://github.com/NVIDIA/cuda-quantum/issues/4562) | ack |
| CUDA-Q | [parser](https://github.com/NVIDIA/cuda-quantum/issues/4600) | fixed |
| CUDA-Q | [parser](https://github.com/NVIDIA/cuda-quantum/issues/4689) |  |
| CUDA-Q | [MLIR to LLVM translator](https://github.com/NVIDIA/cuda-quantum/issues/4694) |  |
| Qiskit | [basis translator](https://github.com/Qiskit/qiskit/issues/16251) | ack, duplicate |
| Pytket | [parser](https://github.com/Quantinuum/tket/issues/2179) | ack |
| Pytket | [routing pass](https://github.com/Quantinuum/tket/issues/2180) | ack |
| Guppy  | [compiler](https://github.com/Quantinuum/tket2/issues/1632) |  | 

## Acknowledgements

- [Linenoise](https://github.com/antirez/linenoise) library for nicities in REPL loop like command history and tab completion.

## Note
- *Only GCC/clang compilers due to some use of GCC pragmas*
- *>= C++20 required*
