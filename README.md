# qf++

<p align="center" width="100%">
    <img width="25%" src="etc/qutefuzz.png">


qf++ is a grammar-driven fuzzing framework for quantum compilers. It generates structurally valid quantum programs across multiple quantum software stacks, then differentially tests them by comparing circuit simulation outputs across compiler optimisation levels.

[![Unitary Foundation](https://img.shields.io/badge/Supported%20By-UNITARY%20FOUNDATION-brightgreen.svg?style=for-the-badge)](https://unitary.foundation)


## Contents

| Doc | What it covers |
|-----|----------------|
| [Ethos](docs/ethos.md) | Why the project exists and its design philosophy |
| [Architecture](docs/architecture.md) | How the C++ fuzzer, Python tester, and CI pipeline fit together |
| [Grammar System](docs/grammar.md) | Full reference for the `.qf` grammar language |
| [Writing a Grammar](docs/writing-grammars.md) | Step-by-step guide to adding a new quantum framework |
| [MAP-Elites](docs/map-elites.md) | How diversity-driven generation works and where to take it next |
| [Differential Testing](docs/diff-testing.md) | How circuits are validated and bugs are classified |
| [Dev](docs/dev.md) | Notes for dev environment |

## Quick start

```sh
# 1. Setup
python3 -m scripts.setup

# 2. Run CI pipeline (10 circuits per grammar)
uv run -m scripts.run --num-tests 10

# 3. Run nightly (1200 circuits, saves interesting ones)
uv run -m scripts.run --nightly --num-tests 1200 --grammars pytket qiskit

# 4. Use the interactive fuzzer REPL directly
./build/qf
> pytket program   # set grammar + entry point
> 5               # generate 5 circuits
> quit
```

A Dockerfile has also been provided. To use it run 
```sh
./scripts/docker/build.sh
```

to create the docker image, then
```sh
./scripts/docker/run.sh
```

to create a docker container and start a shell inside it. From there, run setup commands as normal.

## Supported quantum frameworks

| Grammar file | Framework | Test method |
|---|---|---|
| `pytket.qf` | Pytket (TKET compiler) | KS test across optimisation levels 0–3 |
| `qiskit.qf` | Qiskit + Aer | KS test across optimisation levels 0–3 |
| `cirq.qf` | Cirq | KS test across 3 custom transpile levels |
| `pennylane.qf` | PennyLane Lightning | KS test across 4 transform pipelines |


## Bugs found

| QSS | Bug in | Status |
|-----|------|-------|
| Pytket | [tket compiler](https://github.com/Quantinuum/tket/issues/2109) | fixed |
| Pytket | [tket2 compiler](https://github.com/Quantinuum/tket2/issues/1417) | ack |

## Acknowledgements

- [Linenoise](https://github.com/antirez/linenoise) library for nicities in REPL loop like command history and tab completion.

## Note
- *Only GCC/clang compilers due to some use of GCC pragmas*
- *>= C++20 required*
