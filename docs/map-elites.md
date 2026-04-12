# MAP-Elites: Diversity-Driven Generation

## What it is and why it matters

A naive fuzzer generates circuits by sampling randomly from the grammar. This works, but the population clusters around the "average" program.

MAP-Elites (Multi-dimensional Archive of Phenotypic Elites) solves this by maintaining an **archive** partitioned into cells. Each cell represents a distinct region of a hand-designed **feature space** (structural properties of the circuit). Only the highest-quality program found for each cell is kept. The algorithm actively mutates existing archive entries to discover programs that fill empty cells.

The result is an archive of programs that are guaranteed to be structurally diverse — if the archive is well-filled, you have programs representing every combination of the chosen features, with each one being the best-quality example found for that region.

## Implementation

### Feature space

Defined in `src/ast/utils/features.cpp`.

The feature space is chosen such that the algorithm *explores* weird, varied AST shapes.

### Quality function

Defined in `src/ast/utils/quality.cpp`.

The quality score of a circuit is calculated statically from the AST. As such, the quality functions chosen are meant to be predictors of which circuits are more likely to stress test the compiler.

### Mutation

There's an entire library of mutations defined in `src/generator/mutate.cpp`. There's still work to be done to figure out which mutations are the most effective. Currently, various mutation strategies are definewd in `src/generator/archive.cpp`, and specific strategies are chosen dynamically at runtime based on the success rate of those mutations. Success rate increases if the mutation helped discover a new cell in the archive.

See [`archive.cpp`](../src/generator/archive.cpp) for full details about how the archive is built.

## Known weaknesses and where to go next

### 1. Crossover is stubbed

`Archive::crossover()` in `archive.cpp` currently just returns `genome_b`. First I want to use just mutations to explore the feature space, I think there's still room for improvement on that front before going for crossover.

### 2. No seeding from known bugs

Currently the archive is initialised from a fresh random population every run. Circuits from previous nightly runs that triggered low KS p-values could be loaded as seed genomes — they likely occupy interesting archive cells and give the mutation loop a head start on stress-testing the same code paths that caused bugs before.

### 3. Grammar-specific feature spaces

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
