# MAP-Elites: Diversity-Driven Generation

## What it is and why it matters

A naive fuzzer generates circuits by sampling randomly from the grammar. This works, but the population clusters around the "average" program.

MAP-Elites (Multi-dimensional Archive of Phenotypic Elites) solves this by maintaining an **archive** partitioned into cells. Each cell represents a distinct region of a hand-designed **feature space** (structural properties of the circuit). Only the highest-quality program found for each cell is kept. The algorithm actively mutates existing archive entries to discover programs that fill empty cells.

The result is an archive of programs that are guaranteed to be structurally diverse — if the archive is well-filled, you have programs representing every combination of the chosen features, with each one being the best-quality example found for that region.

## Current implementation

### Feature space (5 dimensions, ~1024 cells)

Defined in `src/ast/utils/features.cpp`. Each feature is a ratio in `[0, 1]` binned into `n` bins (plus one overflow bin).

| Feature | Formula | Bins |
|---------|---------|------|
| `control_flow_ratio` | `CF_STMT count / COMPOUND_STMT count` | 3 |
| `subroutine_call_ratio` | `SUBROUTINE_OP count / QUBIT_OP count` | 3 |
| `parameterised_gate_ratio` | `gates with float args / total gates` | 3 |
| `barrier_op_ratio` | `BARRIER_OP count / COMPOUND_STMT count` | 3 |
| `multi_qubit_gate_ratio` | `gates with n_qubits > 1 / total gates` | 3 |

Each of the featues chosen relates to common passes made in quantum compilers. 

### Quality function (4 components, equal weight)

Defined in `src/ast/utils/quality.cpp`.

| Component | Measures |
|-----------|---------|
| `gate_arity_variance` | Variance in qubit count across gates — higher = more mixed gate types |
| `gate_type_entropy` | Shannon entropy of the gate type distribution — higher = more diverse gate usage |
| `adj_gate_pair_density` | Fraction of consecutive gate pairs that are identical — higher = more repetitive (questionable) |
| `max_control_flow_depth` | Maximum if/for/while nesting depth |

### Mutation

`Mutate_children` in `src/generator/mutate.cpp`: for each `COMPOUND_STMTS` block, randomly either adds a new compound statement (by re-running the AST builder for `compound_stmt`) or removes one.

There's an entire library of mutations defined in `src/generator/mutate.cpp`. There's still work to be done to figure out which mutations are the most effective.

See [`archive.cpp`](../src/generator/archive.cpp) for details about how the archive is built.

## Known weaknesses and where to go next

### 1. Crossover is stubbed

`Archive::crossover()` in `archive.cpp` currently just returns `genome_b`. First I want to use just mutations to explore the feature space, I think there's still room for improvement on that front before going for crossover.

### 2. Mutation is too coarse

Adding and removing whole compound statements changes the program length but not the character of individual operations. The archive fills up with programs that vary in *how many* statements they have but not in *what the statements look like*.

**Proposed operators:**

| Operator | What it does |
|----------|-------------|
| `Gate_parameter_mutation` | Perturb float arguments of a `FLOAT_LITERAL` node |
| `Gate_type_mutation` | Replace a gate with another gate of the same qubit arity from `supported_gates.h` |
| `Qubit_permutation_mutation` | Swap two qubit argument positions within a single gate application |
| `Control_flow_insertion` | Wrap an existing `qubit_op` sequence in a randomly generated `if_stmt` |
| `Subroutine_extraction` | Take a contiguous block of gate ops and factor them out as a new subroutine |

Implementing `Gate_type_mutation` is the highest-priority improvement: it diversifies the gate type distribution (improving `gate_type_entropy`) without changing circuit length.

### 3. Quality function incentivises the wrong things

`adj_gate_pair_density` rewards programs with *more* consecutive identical gates, which is arguably not what we want (identical adjacent gates are easy to cancel and likely to be handled by the simplest optimisation pass). Consider replacing it with:

- **Unique gate variety** — number of distinct `Token_kind` gate types used / total gate types available
- **Circuit depth estimate** — approximate depth (number of "layers" if gates are parallelised by non-overlapping qubits)

Also consider **weighting** quality components rather than treating them equally. Control flow depth is probably more valuable as a stress test than gate arity variance. I would like to measure how good the quality score is perhaps using compiler converage, then use that to manually tweak these weights, or just learn them.

### 4. Features don't capture qubit connectivity

All current features are about gate types and statement structure. None captures how qubits are *connected* — whether the circuit forms a linear chain, a star topology, or a dense entanglement graph. Two circuits can have the same gate type distribution but radically different qubit interaction graphs.

**Proposed additional features:**

| Feature | Formula |
|---------|---------|
| `qubit_reuse_density` | Mean number of times each qubit appears in `gate arguments / circuit length` |
| `entanglement_ratio` | `2-qubit gate count / (n_qubits x circuit_length)` — proxy for connectivity density |

Adding two more features with 3 bins each would grow the archive to 4^7 = **16384 cells**, which is larger but still tractable. The fill loop would need a higher initial population.

### 5. No seeding from known bugs

Currently the archive is initialised from a fresh random population every run. Circuits from previous nightly runs that triggered low KS p-values could be loaded as seed genomes — they likely occupy interesting archive cells and give the mutation loop a head start on stress-testing the same code paths that caused bugs before.

### 6. Grammar-specific feature spaces

The current feature space is grammar-agnostic: `COMPOUND_STMTS`, `CF_STMT`, `QUBIT_OP`, `BARRIER_OP` are all defined in the meta-grammar's token kinds. This works, but some features are meaningless for some grammars (e.g. `barrier_op_ratio` is always zero for PennyLane, which has no barrier operation).

Consider allowing grammars to define their own feature extractors, or at minimum defining a per-grammar feature mask that zeros out irrelevant dimensions.

---

## Running MAP-Elites

MAP-Elites is triggered from the fuzzer REPL with the `map-elites` toggle:

```
> pytket program
> map-elites          # enables map-elites mode
> 200                 # generate initial population of 200 genomes
```

Or programmatically in `Generator::map_elites(n_genomes, control, output_dir)`.

The archive state is dumped as JSON (`init_archive.json`, `final_archive.json`) in the output directory. Use `scripts/visualise/plot_coverage.py` to visualise the feature space projected to 2D via UMAP.

```sh
python scripts/visualise/plot_coverage.py \
    --json outputs/pytket/final_archive.json
```
