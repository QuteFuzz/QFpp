# Ethos

To improve upon work done on [QuteFuzz](https://github.com/QuteFuzz/QuteFuzz) by introducing a grammar language and [MAP-elites](https://arxiv.org/abs/1504.04909) algorithm for diverse program generation.

## The core problem

Quantum compilers are hard to test. A compiler optimisation that incorrectly transforms a circuit can subtly change its output probability distribution rather than crashing — meaning standard assertion-based testing misses it entirely. The quantum state is unobservable except through measurement statistics, and small numerical errors in gate decomposition or qubit routing can silently alter those statistics in ways that matter for real algorithms.

The conventional approach is a hand-written test suite of known circuits. This has two problems:

1. **Coverage is bounded by human imagination.** Compiler bugs live in structural corners: rarely used gate combinations, deep nesting, subroutine calls with unusual qubit mappings.

2. **Tests go stale.** As compilers evolve, old test circuits may stop exercising the code paths that matter.

## What qf++ does instead

qf++ automatically generates structurally diverse, valid quantum programs by describing each quantum framework as a formal grammar and then sampling from that grammar to produce ASTs. A generated circuit is always syntactically and semantically valid — it imports the right libraries, allocates qubits correctly, respects register sizes, and calls the right compiler entry points.

It then validates circuits by **differential testing**: running the same circuit through the same compiler at multiple optimisation levels and comparing the output probability distributions using a Kolmogorov–Smirnov test. A semantically correct compiler must produce statistically indistinguishable distributions across all optimisation levels. A low KS p-value signals a potential bug. In practice, real bugs lead to p-values of 0.

This approach has already found bugs in TKET and TKET2 (see [README](../README.md)) using this tool. 

## Design principles

### Grammars over templates

Every grammar is a plain-text `.qf` file. Adding support for a new framework means writing a grammar, not modifying the C++ core. The grammar language is expressive enough to describe:

- Language-specific syntax (circuit object methods, decorated functions, bare function calls)
- Scope distinctions between subroutine parameters and top-level register allocations
- Dynamic term constraints tied to the current circuit's qubit/bit/float counts
- Indentation-aware output for Python

The meta-grammar (`meta-grammar.qf`) defines rules shared by all grammars — gate argument structure, float literals, qubit lists — so individual grammars stay concise.

### Semantic validity, not just syntactic validity

Generating Python that represents a *correct* quantum program is hard. QuteFuzz tracks circuit state in the `Context` object while building the AST:

- Resources (qubits, bits, parameters) are allocated and tracked. Qubit references always refer to a real, allocated qubit in the current circuit.
- Gate arguments are resolved dynamically: `GET_GATE_QUBITS` expands to the actual number of qubit arguments the chosen gate requires, not a hardcoded constant. All supported gates are preset in the generation engine. See [`supported_gates.h`](../include/utils/supported_gates.h)
- Subroutine calls are only inserted when the calling circuit has enough qubits to satisfy the subroutine's external interface.
- Control flow depth is bounded by `NESTED_MAX_DEPTH` to prevent unbounded recursion.

The generated programs are ready to run without hand-editing.

See [`context.cpp`](../src/ast/context.cpp) and [`context.h`](../include/ast/context.h) for more information about context, and see [`ast.cpp`](../src/ast/ast.cpp) for how it is used.

### Diversity as a first-class concern

A fuzzer that always generates the same class of circuits will always find the same bugs (or none). QuteFuzz uses [MAP-Elites](map-elites.md) to maintain an archive of programs that are *structurally diverse* — spread across a feature space that captures things like control flow density, subroutine use, parameterised gate frequency, and multi-qubit gate ratio. The algorithm actively pushes generation towards underexplored regions of this space.

### Reproducibility

Every generation run records its global seed in `regression_seed.txt`. A seed that produces an interesting circuit can be replayed exactly. Nightly run artefacts include this seed file alongside the circuit and an `analysis.txt` explaining why the circuit was flagged.

### Minimal friction

The C++ fuzzer is driven by a REPL that supports tab completion and command history. The Python pipeline wraps everything in a single `uv run scripts/run.py` call. CI and nightly runs are automated GitHub Actions workflows.

