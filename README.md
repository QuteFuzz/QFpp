# QuteFuzz

<p align="center" width="100%">
    <img width="25%" src="etc/qutefuzz.png">

Generating programs for circuit-based programming languages by leveraging a BNF-style meta-language, which is used to describe the program. The tool takes this as input, generates an AST from it, then outputs a program. 

See wiki for more details.

[![Unitary Foundation](https://img.shields.io/badge/Supported%20By-UNITARY%20FOUNDATION-brightgreen.svg?style=for-the-badge)](https://unitary.foundation)

## Setup

1. Dependencies
```sh
./scripts/setup/setup_deps.sh
```

2. Run
```sh
uv run scripts/ci_pipeline.py
```

to run fuzzer and differential testing.

To run fuzzer on its own, setup cmake build dir using 
```sh
./scripts/setup/setup_build.sh
```

then running `build/fuzzer`.

## Bugs found with the help of QuteFuzz 2.0

### Pytket:

| Compiler Bugs | Other Bugs |
|---------------|------------|
| [Issue 623 &#x1F41E;](https://github.com/CQCL/pytket-quantinuum/issues/623) | |
| [Issue 2004  &#x1F41E;](https://github.com/CQCL/tket/issues/2004) | 
| | |

### Guppy:

| Compiler Bugs | Other Bugs |
|---------------|------------|
| [Issue 1122 &#x1F41E;](https://github.com/CQCL/guppylang/issues/1122)  | [Issue 1101 &#x1F41E;](https://github.com/CQCL/guppylang/issues/1101)|
| [Issue 1224 &#x1F41E;](https://github.com/CQCL/guppylang/issues/1224) | |
| [Issue 1079 &#x1F41E;](https://github.com/CQCL/guppylang/issues/1079) | |
| | |



