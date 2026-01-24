# QuteFuzz

<p align="center" width="100%">
    <img width="25%" src="etc/qutefuzz.png">

Generating programs for circuit-based programming languages by leveraging a BNF-style meta-language, which is used to describe the program. The tool takes this as input, generates an AST from it, then outputs a program.

See wiki for more details.

[![Unitary Foundation](https://img.shields.io/badge/Supported%20By-UNITARY%20FOUNDATION-brightgreen.svg?style=for-the-badge)](https://unitary.foundation)

## Setup

1. Dependencies
```sh
./scripts/setup/setup_env.sh
```

2. Run
```sh
uv run scripts/run.py
```

to run fuzzer and differential testing.

To run fuzzer on its own, setup cmake build dir using
```sh
./scripts/setup/setup_build.sh
```

then running `build/fuzzer`.

3. Use `converage html` to generate html for coverage report 

See `dev` for dev-specific docs

*Only GCC/clang compilers due to some use of GCC pragmas*
