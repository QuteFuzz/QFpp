# Differential Testing

## What is differential testing?

Differential testing compares multiple implementations of the same specification against each other on identical inputs. In qf++, the "multiple implementations" are the same quantum compiler at different optimisation levels. The specification is that semantic optimisation must preserve circuit semantics — the output probability distribution must not change.

For a circuit `C` and optimisation levels `0, 1, 2, 3`, the distributions `D(C, level_i)` and `D(C, level_0)` must be statistically indistinguishable.

## How it works

### 1. Circuit generation

The C++ fuzzer generates `prog.py` files. Each file:
1. Defines qubit and bit registers.
2. Applies gate operations.
3. Calls `X_testing().opt_ks_test(circuit, circuit_id)` at the end.

`opt_ks_test` is defined in `diff_testing/lib.py`:

```python
def opt_ks_test(self, circuit, circuit_number: int) -> None:
    counts0 = self.get_counts(circuit, opt_level=0, circuit_num=circuit_number)
    for i in range(3):
        counts_i = self.get_counts(circuit, opt_level=i+1, circuit_num=circuit_number)
        ks = self.ks_test(counts0, counts_i)
        print(f"Optimisation level {i+1} ks-test p-value: {ks}")
```

Each line of output is parsed by the regex in `scripts/run.py`:

```python
pattern = r"ks-test p-value:\s*([\d.]+)|Dot product\s*([\d.]+)"
```

### 2. The KS test

`Base.ks_test(counts1, counts2)` expands the count dictionaries into two sample lists of size `num_shots` (default 100,000) and calls `scipy.stats.ks_2samp(..., method='asymp')`.

The p-value returned is the probability of observing a KS statistic at least as extreme as the one measured, under the null hypothesis that both samples come from the same distribution.

A **low p-value** (below `MIN_KS_VALUE = 1e-8`) means the two distributions are unlikely to be the same — the compiler likely changed the circuit semantics.

### 3. Count normalisation

`preprocess_counts(counts, n_bits)` handles framework-specific quirks:
- Converts tuple keys (Pytket) and string keys (Qiskit) to base-10 integers.
- Flips Qiskit's reversed bit ordering.
- Truncates to `n_bits` to handle ancilla measurement outputs.
- Sorts by key for reproducibility.

### 4. Interesting circuit detection

`run.py` classifies each circuit result as:

| Classification | Condition |
|----------------|-----------|
| **Fuzzer error** | The process exited non-zero AND the output contained "Error" or "error" |
| **Compiler error** | KS p-value < 1e-8 for any optimisation level, OR dot product ≠ 1 for statevector tests |
| **Pass** | All p-values ≥ 1e-8 |

Fuzzer errors abort the run (the fuzzer has a bug). Compiler errors are saved for inspection.

## Framework-specific notes

### Pytket

`pytketTesting.get_counts()` uses `AerBackend` with `n_shots=100_000`. Optimisation levels 0–3 map to Pytket's `backend.get_compiled_circuit(circuit, optimisation_level=N)`.

The optional `tket2=True` mode applies `badger_pass` from the TKET2 compiler at level 3 (after stripping measurements and barriers that the HUGR graph cannot handle).

`pytket_qiskit_conv_test` converts the Pytket circuit to Qiskit and compares count distributions — a cross-compiler test.

### Qiskit

`qiskitTesting.get_counts()` uses `AerSimulator` with `transpile(circuit, backend, optimization_level=N)`.

Bit ordering is reversed relative to Pytket/Cirq: Qiskit orders bits from MSB=0 to LSB=n, others from LSB=0. `preprocess_counts()` flips the key string for Qiskit results.

### Cirq

Uses Cirq's built-in `Simulator`, not Aer. Optimisation levels are three custom transformer pipelines defined in `diff_testing/cirq.py`:
- Level 1: drop empty moments, merge 1-qubit unitaries.
- Level 2: eject Z and phased Paulis, defer measurements, synchronise terminal measurements.
- Level 3: expand composites, merge 2-qubit unitaries, optimise for CZ target gateset.

The `multi_measurement_histogram` is used because Cirq circuits can have multiple measurement keys.

### PennyLane

Optimisation levels use PennyLane's transform API:
- Level 0: bare circuit call.
- Level 1: `cancel_inverses`.
- Level 2: `merge_rotations` ∘ `cancel_inverses`.
- Level 3: `compile`.

PennyLane circuits are QNodes (decorated functions), not circuit objects. The generated programs produce a `@qml.qnode` callable named `main_circuit`.

## Statevector testing (Pytket only)

`pytketTesting.run_circ_statevector()` uses `AerStateBackend` and compares statevectors directly via the overlap `|⟨ψ₀|ψᵢ⟩|`. An overlap of exactly 1.0 (up to numerical precision at 6 decimal places) is required. This is stricter than the KS test and does not require sampling, so it finds smaller semantic differences — but it does not work for circuits with measurements or noise.

## Adding a new test mode

1. Add a method to your `*Testing` class that calls `get_counts()` at multiple levels and prints p-values in the format `ks-test p-value: X`.
2. Add a rule to your grammar's `testing_method`:
   ```qf
   testing_method = opt_ks_test | my_custom_test;
   my_custom_test = "mt.my_custom_test(" GET_CIRCUIT_NAME COMMA circuit_id RPAREN NL;
   ```
3. `run.py`'s regex already parses any line matching `ks-test p-value:` or `Dot product`.
