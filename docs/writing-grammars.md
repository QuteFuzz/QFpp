# Writing a Grammar

This is a practical guide for adding support for a new quantum framework. By the end you *may* (still a work in progress tool) have a `.qf` file that generates runnable programs, a Python differential testing class, and a working CI integration.

---

## Step 1: Understand what you need to produce

Before writing any grammar, write one program by hand. For example, a minimal Qiskit program looks like:

```python
from qiskit import QuantumCircuit, QuantumRegister, ClassicalRegister
from qiskit.circuit import Parameter
from diff_testing.qiskit import qiskitTesting

p0 = Parameter('p0')

qreg_a = QuantumRegister(2, 'a')
main_circuit = QuantumCircuit(0)
main_circuit.add_register(qreg_a)

creg_b = ClassicalRegister(2, 'b')
main_circuit.add_register(creg_b)

main_circuit.h(qreg_a[0])
main_circuit.cx(qreg_a[0], qreg_a[1])

main_circuit.measure_all()
qt = qiskitTesting()
qt.opt_ks_test(main_circuit, 0)
```

Now identify the **variable parts**: qubit register names, register sizes, gate names, gate arguments. Everything else is fixed structure.

---

## Step 2: Required rules

Every grammar **must** define or inherit all of the following. Rules inherited from `meta-grammar.qf` are marked (M); rules you must define are marked (Y):

### Configuration

| Directive | Who defines | Notes |
|-----------|------------|-------|
| `EXTENSION` | **Y** | File extension string, e.g. `".py"` |
| `MAX_REG_SIZE` | Y/M | Integer; omit to use default (10). Usually set to 2–4 for Python frameworks |
| `NESTED_MAX_DEPTH` | Y/M | Integer; omit to use default (7) |

### Structure

| Rule | Who defines | Notes |
|------|------------|-------|
| `program` | **Y** | Top-level; must eventually produce imports, circuit, footer |
| `circuit` or `subroutine_circuit` | **Y** | The `CIRCUIT` token triggers circuit creation |
| `body` or `subroutine_body` | **Y** | The `BODY` token marks circuit contents |
| `imports` | **Y** | Framework imports; plain string rules |
| `program_footer` | **Y** | Testing harness call at end of file |
| `circuit_def_header` | **Y** | Whatever introduces a new circuit object |
| `compound_stmts` | Y/M | Meta-grammar defines `(compound_stmt NEWLINE)[UNIFORM(3,5)]`; override if needed |
| `compound_stmt` | Y/M | Meta-grammar has no default; you must define it |
| `qubit_op` | Y/M | Meta-grammar defines `gate_op \| subroutine_op`; override if needed |

### Resource definitions (all six required, in all scopes used)

The run-time requires pointers to all six of these rules in EXTERNAL, INTERNAL, and GLOB scopes. Note that `meta-grammar.qf` already defines all these rules in all 3 scopes. If your framework defines a certain resource in a certain style, you **must** use these rules to define it.

| Rule | Notes |
|------|-------|
| `singular_qubit_def` | Template for a single-qubit allocation |
| `register_qubit_def` | Template for a register allocation (N qubits) |
| `singular_bit_def` | Template for a single classical bit |
| `register_bit_def` | Template for a classical register |
| `singular_param_def` | Template for a single symbolic parameter |
| `register_param_def` | Template for a parameter register |

### Resource references

| Rule | Notes |
|------|-------|
| `singular_qubit` | How a single qubit is referenced (e.g. `GET_NAME "[0]"` or just `GET_NAME`) |
| `register_qubit` | How a register qubit is referenced (e.g. `GET_NAME "[" GET_INDEX "]"`) |
| `singular_bit` | Analogous for bits |
| `register_bit` | Analogous for bits |

### Gate rules

| Rule | Notes |
|------|-------|
| `gate_op` | **Required in GLOB scope**. Full gate application expression |
| `gate_name` | Which gates are available. Use existing gate names (h, x, rx, etc.) plus `+=` for extras |

The meta-grammar defines a base `gate_name` with: `h | x | y | z | rz | rx | ry | cx | cy | cz | ccx`. Use `gate_name +=` to add more.

### Testing harness

| Rule | Notes |
|------|-------|
| `compiler_call` / `testing_method` | Usually calls `opt_ks_test` or `statevector_test` |
| `opt_ks_test` | Must print `ks-test p-value: X` to stdout |
| `circuit_id` | Use the `CIRCUIT_ID` meta function |

---

## Step 3: Write the grammar file

Create `grammar_definitions/myframework.qf`. A minimal skeleton:

```qf
# myframework.qf

EXTENSION = ".py";
MAX_REG_SIZE = 2;

# ── Gate names ──────────────────────────────────────────────
x = "MyFramework.X";
h = "MyFramework.H";
cx = "MyFramework.CX";
rx = "MyFramework.RX";

# gate_name is already defined in meta-grammar; extend it:
# (for gates already in the meta-grammar, just redefine the name string)

# ── Resource definitions ─────────────────────────────────────
EXTERNAL {
    qubit_defs = (qubit_def NEWLINE)[UNIFORM(2, 4)];

    singular_qubit_def = GET_DEF_NAME " = framework.qubit('" GET_NAME "')";
    register_qubit_def = GET_DEF_NAME " = framework.qvec(" GET_SIZE ", '" GET_NAME "')";

    # If no bit support:
    singular_bit_def = "";
    register_bit_def = "";

    # If no param support:
    singular_param_def = "";
    register_param_def = "";
}

# ── Resource references ──────────────────────────────────────
singular_qubit = GET_DECL_NAME;
register_qubit = GET_DECL_NAME "[" GET_INDEX "]";

singular_bit = GET_DECL_NAME;
register_bit = GET_DECL_NAME "[" GET_INDEX "]";

# ── Gate operation ────────────────────────────────────────────
gate_op = gate_name "(" gate_op_args ")";

# ── Program structure ─────────────────────────────────────────
program = imports NEWLINE NEWLINE circuit NEWLINE program_footer;

circuit = circuit_def_header NEWLINE body NEWLINE;

circuit_def_header =
    "@framework.kernel" NEWLINE
    "def " GET_CIRCUIT_NAME "():";

body = CHILD_INDENT<EXTERNAL::qubit_defs NEWLINE compound_stmts);

compound_stmts = (compound_stmt NEWLINE)[UNIFORM(5, 15)];

compound_stmt = qubit_op;

imports =
    "import myframework as framework" NEWLINE
    "from diff_testing.myframework import myframeworkTesting";

# ── Footer ────────────────────────────────────────────────────
program_footer = "mt = myframeworkTesting()" NEWLINE testing_method;

testing_method = opt_ks_test;

opt_ks_test = "mt.opt_ks_test(" GET_CIRCUIT_NAME COMMA circuit_id RPAREN NEWLINE;
```

---

## Step 4: Write the differential testing class

Create `diff_testing/myframework.py`:

```python
from .lib import Base

class myframeworkTesting(Base):
    def __init__(self) -> None:
        super().__init__("myframework")

    def get_counts(self, circuit, opt_level: int, circuit_num: int):
        # 1. Apply optimisation at opt_level
        # 2. Run circuit for self.num_shots shots
        # 3. Return self.preprocess_counts(raw_counts, n_bits)
        raise NotImplementedError
```

`preprocess_counts(counts, n_bits)` normalises the count dict to use integer keys (base-10 representation of the measurement bitstring). See `diff_testing/lib.py` for the full signature.

---

## Step 5: Register in the runner

In `scripts/run.py`, add your grammar name to the `GRAMMARS` list and `SIMULATION_CAP` dict:

```python
GRAMMARS = ["pytket", "qiskit", "cirq", "pennylane", "myframework"]
SIMULATION_CAP = {
    "pytket": 64,
    "qiskit": 64,
    "cirq": 8,
    "pennylane": 64,
    "myframework": 64,  # lower if simulation is slow
}
```

*Will make this a yaml file in the future*

---

## Step 6: Test it

```sh
# Interactive REPL
./build/qf
> myframework program
> 3

# Check the generated files
cat outputs/myframework/circuit0/prog.py

# Run end-to-end
uv run scripts/run.py --grammars myframework --num-tests 5
```

---

## Common mistakes

### Qubit references break at runtime

`singular_qubit` and `register_qubit` are expanded when a `qubit` node is encountered. The `GET_NAME` and `GET_INDEX` meta functions resolve to the *parent node's* name and index — which is the `Resource` object allocated by the `qubit_def`. Make sure your qubit reference syntax matches your qubit allocation syntax.

### Gate applies wrong number of arguments

`GATE_QUBITS`, `GATE_BITS`, and `GATE_PARAMS` resolve to the counts for the *currently selected gate*. These are defined in `include/utils/supported_gates.h`. If you define a gate name like `my_gate = "SomeGate"` and it matches an existing `Token_kind` (e.g. `cx`), the supported count from `supported_gates.h` is used. If it does not match any known gate, the fuzzer assigns a random qubit count and logs a warning.

### Subroutines not working

For subroutine support you need:
- `subroutine_defs = (subroutine_circuit NEWLINE)[UNIFORM(1, 3)];`
- `subroutine_circuit` — a rule that creates a sub-circuit with EXTERNAL qubit parameters
- `subroutine_def_footer` — any post-circuit wrapping (e.g. `.to_gate()`)
- `subroutine_op` — how to call the subroutine from the main circuit
- `subroutine_op_args` — the qubit arguments for the call

Look at `qiskit.qf` or `pytket.qf` for a complete example.
