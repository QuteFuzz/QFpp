# Architecture

## High-level flow

```
.qf grammar files
       |
       ▼
 C++ fuzzer (build/qf)
  ├── Grammar parser (src/grammar/)
  ├── AST builder   (src/ast/)
  ├── MAP-Elites    (src/generator/archive.cpp)
  └── outputs/
        ├── pytket/circuit0/prog.py
        ├── pytket/circuit1/prog.py
        └── ...
       │
       ▼
 Python test runner (scripts/run.py)
  ├── Parallel circuit execution
  ├── KS-test p-value parsing
  └── Interesting circuit collection
       │
       ▼
 nightly_results/  (saved if p-value < threshold)
```

## C++ fuzzer

### Grammar layer (`include/grammar/`, `src/grammar/`)

**`Lexer`** (`lex.h`) tokenises a `.qf` file using a single regex. Every token is classified into one of three groups:

- **Rule kinds** (`RULE_KINDS_TOP` to `RULE_KINDS_BOTTOM`): terminals that the AST builder knows how to specialise — `GATE_NAME`, `QUBIT_DEF`, `CIRCUIT`, `COMPOUND_STMT`, etc.
- **Meta functions** (`META_FUNC_TOP` to `META_FUNC_BOTTOM`): dynamic tokens resolved at AST build time — `GET_CIRCUIT_NAME`, `GET_GATE_QUBITS`, `GET_NAME`, `GET_SIZE`, `GET_INDEX`, `MAKE_FLOAT`, etc.
- **Grammar syntax**: `=`, `|`, `;`, `(`, `)`, `[`, `]`, `{`, `}`, `*`, `+`, `?`, `<`, `>`, `::`, `+=`.

**`Grammar`** (`grammar.h/cpp`) builds a graph of `Rule` objects from tokens. Each rule has one or more `Branch`es; each branch is a sequence of `Term`s. Terms are either syntax literals or pointers to other rules (allowing recursion). A `Term_constraint` on a term controls how many times it is repeated when the AST builder expands that term.

### AST layer (`include/ast/`, `src/ast/`)

**`Ast::build(entry_rule)`** recursively expands a grammar rule into a tree of `Node` objects. The key method is `make_child(parent, term)`, a large switch over the term's `Token_kind`. Depending on the kind, `make_child` either:

- Returns a plain `Node` (syntax, keywords),
- Creates a specialised node type (`Circuit`, `Gate`, `Resource`, `Qubit_op`, `Compound_stmt`, `Float`, `UInt`, `Variable`, …),
- Or redirects the expansion to a different rule (e.g. `SUBROUTINE_OP` → `GATE_OP` when no subroutines are available).

All semantic decisions — which qubit to pick, what gate to use, whether subroutines are available — happen here through the `Context` object.

**`Context`** is the semantic state for one AST. It tracks:
- All `Circuit` objects created so far (one per subroutine + one main)
- The `Current_nodes` struct (current gate, resource, resource_def, qubit_op)
- Nested depth
- The `Control` struct (grammar-level config: `MAX_REG_SIZE`, `NESTED_MAX_DEPTH`, expected rules)

**Printing** — `Node::print_program()` traverses the tree and writes text. `Print_mode` on a node controls indentation: `DEFAULT` (recurse into children), `CHILD_INDENT` (add one tab level for all children), `SELF_INDENT` (add the current tab level prefix before this node), `GET_INDENT_LEVEL` (emit the raw integer depth for things like Qiskit's `else_N` variable names).

### Generator layer (`include/generator/`, `src/generator/`)

**`Generator`** orchestrates `generate_n_asts()` — seeds `rng()`, calls `Ast::build()`, and collects `Ast_entry` structs (AST root + context snapshot).

**`map_elites()`** calls `generate_n_asts()` for the initial population, then fills an `Archive` using the `fill_archive()` mutation loop.

**`ast_parse()`** writes each `Ast_entry` to disk: one subdirectory per circuit containing `prog.<ext>` and optionally `ast.png` (GraphViz render).

### MAP-Elites archive (`src/generator/archive.cpp`)

See [MAP-Elites](map-elites.md) for a full description. Briefly:

- **`Features`** (`src/ast/utils/features.cpp`) computes 5 float ratios from the AST and bins them. The product of bin counts gives the archive size (~1024 cells).
- **`Quality`** (`src/ast/utils/quality.cpp`) scores a genome on 4 metrics. The cell keeps only the highest-quality genome for its feature vector slot.
- **`mutation()`** currently applies `Mutate_children` — probabilistically adding or erasing compound statements in each `COMPOUND_STMTS` block.

## Python test runner (`scripts/run.py`)

`Check_grammar.check()` runs two phases:

1. **`generate_tests()`** — pipes a control string into the fuzzer REPL (`grammar entry_point\nN\nquit\n`), which writes circuits to `outputs/<grammar>/`.
2. **`validate_generated_circuits()`** — runs each `prog.py` in a `ThreadPoolExecutor`, parses KS p-values from stdout, and flags circuits where any optimisation level produces a p-value below `MIN_KS_VALUE` (1e-8).

Interesting circuits are saved to `nightly_results/<timestamp>/<grammar>/` with an `analysis.txt`.

## Differential testing library (`diff_testing/lib.py`)

`Base.opt_ks_test(circuit, circuit_number)` is the standard test harness called by every generated program. It:
1. Runs the circuit at optimisation level 0 (unoptimised).
2. Runs the circuit at levels 1, 2, 3.
3. Calls `ks_test()` on the count dictionaries.
4. Prints `Optimisation level N ks-test p-value: X` — the regex in `run.py` parses this.

`preprocess_counts()` normalises count dictionaries: it handles tuples vs strings, Qiskit's reversed bit ordering, and truncates to `n_bits` to keep comparisons fair.

## Key data structures

| Type | File | Role |
|------|------|------|
| `Rule` | `include/grammar/rule.h` | Named rule with N branches |
| `Branch` | `include/grammar/branch.h` | Ordered sequence of Terms |
| `Term` | `include/grammar/qf_term.h` | Either a syntax literal or a weak_ptr to a Rule |
| `Term_constraint` | `include/grammar/term_constraint.h` | Repetition count (fixed, random, or dynamic) |
| `Node` | `include/ast/node/node.h` | AST node; subclassed by Gate, Resource, Circuit, … |
| `Context` | `include/ast/context.h` | Per-AST semantic state |
| `Ast_entry` | `include/ast/ast.h` | (AST root, Context snapshot) |
| `Features` | `include/ast/utils/info.h` | 5-dimensional feature vector |
| `Quality` | `include/ast/utils/info.h` | Scalar quality score |
| `Cell` | `include/generator/archive.h` | One archive cell: best genome + its FV + quality |
| `Archive` | `include/generator/archive.h` | Full MAP-Elites archive |
| `Control` | `include/run_utils.h` | Per-grammar config and required rule pointers |
