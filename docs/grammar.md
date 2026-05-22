# Grammar System Reference

QuteFuzz uses a BNF-inspired domain-specific language (`.qf` files) to describe quantum programs. The grammar language is processed by the C++ fuzzer; individual grammars live in `grammar_definitions/`.

## File structure

Every grammar file is combined with `meta-grammar.qf` before parsing. Rules in `meta-grammar.qf` act as defaults — a grammar can override any of them by redefining the rule name.

```
grammar_definitions/
├── meta-grammar.qf   ← shared defaults (DO NOT delete)
├── pytket.qf
├── qiskit.qf
├── cirq.qf
├── pennylane.qf
└── cudaq_py.qf
```

*Will add a proper import system in the future*

---

## Rule definitions

### Basic rule

```qf
rule_name = term1 term2 | term3;
```

A rule has one or more **branches** separated by `|`. The fuzzer picks one branch uniformly at random. The rule ends with `;`.

### Appending branches to an existing rule

```qf
gate_name += my_gate1 | my_gate2;
```

`+=` adds new branches to a rule without replacing existing ones. Used by grammar files to extend `gate_name` (defined in the meta-grammar) with framework-specific gate names.

### Inline grouping with parentheses

```qf
compound_stmt = qubit_op | (if_stmt)?;
```

Parentheses create an anonymous sub-rule. The sub-rule can have a wildcard applied to it (see below).

---

## Syntax literals

Anything in double or single quotes is a literal string in the output:

```qf
gate_op = GET_CIRCUIT_NAME ".append(" gate_name "(" gate_op_args "))";
```

Shorthand keywords that expand to common punctuation (avoids quoting issues):

| Keyword | Expands to |
|---------|-----------|
| `NL` | `\n` |
| `COMMA` | `,` |
| `LPAREN` | `(` |
| `RPAREN` | `)` |
| `LBRACK` | `[` |
| `RBRACK` | `]` |
| `LBRACE` | `{` |
| `RBRACE` | `}` |
| `SPACE` | ` ` |
| `DOT` | `.` |
| `SINGLE_QUOTE` | `'` |
| `DOUBLE_QUOTE` | `"` |
| `EQUALS` | `=` |

---

## Wildcards (term repetition)

Applied as a suffix to any term or parenthesised group:

`WILDCARD_MAX` = 10

| Suffix | Meaning |
|--------|---------|
| `*` | Zero or more (0 to `WILDCARD_MAX`) |
| `+` | One or more (1 to `WILDCARD_MAX`) |
| `?` | Zero or one |

---

## Expr `[...]`

Expressions can be written directly into the grammar to facilitate more complex rule creation.

See [`expr.h`](../include/grammar/expr.h) for all possible expressions, and grammar.

With expressions are tied to `Term`, where evaluating them returns a vector of terms.

Terms in the grammar are evaluated differently depending on the return type of the evaluation

### `int`

```c
qubit_defs = (qubit_def NL)[UNIFORM(3, 5)];
```

1. `(...)` triggers the creation of a rule, so the above can be seen as
```c
qubit_defs = NR_0[UNIFORM(3, 5)];
```

where `NR_0` is a term which contains the rule pointer.

2. when evaluated, the expression returns a random number between 3 and 5 (inclusive on both sides), and returns as many terms

### `string`
```c
singular_qubit_def_discard =  "measure(" ""[def.name] ")" NL;
```

the string replaces whatever term the expr is bound to. The term above contains the string `""`.

### `std::vector<std::shared_ptr<Rule>>`
```c
    qubit_defs_discard = [
        for def in ALL_QUBIT_DEFS {
            if (def.in_int_scope) {
                singular_qubit_def_discard =  "measure(" ""[def.name] ")" NL;
                register_qubit_def_discard =  "measure_array(" ""[def.name] ")" NL;
                if (def.is_singular): singular_qubit_def_discard
                else: register_qubit_def_discard
            }
        }
        ]
        NL;
```

the for loop returns a vector of rules, which are used to create terms

---

## Meta functions

Meta functions are resolved at AST build time. They emit a node whose content is determined by the current fuzzer state, not a literal string.

### Resource references

| Meta function | Emits |
|--------------|-------|
| `GET_NAME` | The name variable of the parent node (e.g. the variable name of a qubit register) |
| `GET_SIZE` | The size of the parent node (register width as an integer) |
| `GET_INDEX` | The index of the parent node (qubit/bit index within a register) |
| `GET_CIRCUIT_NAME` | The current circuit's owner name (`main_circuit` or `subN`) |
| `CIRCUIT_ID` | The global circuit counter (used in testing harness calls) |

### Value generators

| Meta function | Emits |
|--------------|-------|
| `MAKE_FLOAT` | A random float (seeded from node counter) |
| `MAKE_INTEGER` | A random unsigned integer |
| `MAKE_VAR` | A random 5-character alphanumeric string |

### Indentation

| Syntax | Behaviour |
|--------------|-----------|
| `ci<rule>` | Printout will print each child node preprended with a tab sized depending on current indentation depth. Increases indentation depth for child nodes. |
| `si<rule>` | Printout will prepend the node itself with a tab sized depending on current indentation depth. Increases indentation depth for child nodes. |
| `GET_INDENT_LEVEL` | Emits the current numeric indent depth as a digit (used in Qiskit's `else_N` pattern) |

Usage:

```qf
if_stmt =
    'with ' GET_CIRCUIT_NAME '.if_test(' classical_expr ') as else_' GET_INDENT_LEVEL ':' NL
    ci<compound_stmts>
    si<('with else_' GET_INDENT_LEVEL ':' NL ci<compound_stmts>>?);
```

### Reset

```qf
apply_measure = GET_CIRCUIT_NAME ".append(cirq.measure(" qubit ", key=m_key))" RESET;
```

`RESET` resets the qubit and bit usage counters so the next gate can reuse the same qubits. This is done automatically after each `qubit_op`, but is also exposed as  a meta function for cases where the operation is defined as a `qubit_op` like the `apply_measure` rule above.

---

## Scopes

Scopes allow a rule to have different definitions depending on context — most importantly, the difference between how qubits are *allocated* at the top level (EXTERNAL) versus how they are used inside a subroutine body (INTERNAL).

### Declaring a scoped block

```qf
EXTERNAL {
    qubit_def = singular_qubit_def | register_qubit_def;

    singular_qubit_def =
        GET_NAME " = QuantumRegister(1, '" GET_NAME "')" NL
        GET_CIRCUIT_NAME ".add_register(" GET_NAME ")";

    register_qubit_def =
        GET_NAME " = QuantumRegister(" GET_SIZE ", '" GET_NAME "')" NL
        GET_CIRCUIT_NAME ".add_register(" GET_NAME ")";
}
```

Rules defined inside `EXTERNAL { }` or `INTERNAL { }` exist only in that scope.

### Referencing a scoped rule from another rule

```qf
body = EXTERNAL::qubit_defs EXTERNAL::bit_defs NL compound_stmts;
```

`EXTERNAL::rule_name` forces the expansion to use the EXTERNAL-scoped version of that rule, even when the outer rule is in GLOB scope.

### Scope semantics in the AST builder

When the AST builder encounters a `QUBIT_DEF` node, it looks up which scope it should use (set by the `EXTERNAL::` or `INTERNAL::` prefix on the referring term). It then calls `context.nn_resource_def(scope, Resource_kind::QUBIT)` with that scope. Resources allocated in EXTERNAL scope are made available to callers of subroutines; resources allocated in INTERNAL scope are local.

---

## Configuration directives

At the top of a grammar file, set integer or string constants:

```qf
EXTENSION = ".py";
MAX_REG_SIZE = 2;
NESTED_MAX_DEPTH = 6;
```

| Directive | Type | Default | Meaning |
|-----------|------|---------|---------|
| `EXTENSION` | string | required | Output file extension (e.g. `".py"`, `".qasm"`) |
| `MAX_REG_SIZE` | integer | 10 (clamped down) | Maximum qubit/bit register width |
| `NESTED_MAX_DEPTH` | integer | 7 (clamped down) | Maximum control flow nesting depth |

Grammar-level values are **clamped down** to the defaults in `params.h` — you cannot exceed the hardcoded maximums.

---

## Comments

```qf
# Single line comment

(*
   Multi-line comment
   Can span multiple lines
*)
```

---

## Special rule names

The following rule names are recognised by the lexer and given specific `Token_kind` values, enabling the AST builder to generate specialised node types for them. Defining a rule with one of these names will trigger that special behaviour.

### Structure rules

| Rule name | Token kind | AST behaviour |
|-----------|-----------|---------------|
| `program` | `PROGRAM` | Top-level entry point |
| `circuit` / `sub_circuit` | `CIRCUIT` | Triggers `context.nn_circuit()` — creates a new `Circuit` object |
| `body` / `subroutine_body` | `BODY` | Marks the circuit body |
| `sub_circuit_defs` | `SUB_CIRCUIT_DEFS` | Triggers `context.nn_sub_circuit_defs()` — marks that we are under the subroutines node |
| `compound_stmt` | `COMPOUND_STMT` | Triggers `Compound_stmt::from_nested_depth()` with constraint injection |
| `compound_stmts` / `subroutine_compound_stmts` | `COMPOUND_STMTS` | Used by MAP-Elites mutation to find mutation sites |
| `qubit_op` | `QUBIT_OP` | Triggers `context.nn_qubit_op()` — resets qubit/bit usage |

### Resource definition rules

| Rule name | Token kind | AST behaviour |
|-----------|-----------|---------------|
| `qubit_def` | `QUBIT_DEF` | Triggers qubit resource definition |
| `bit_def` | `BIT_DEF` | Triggers bit resource definition |
| `param_def` | `PARAM_DEF` | Triggers parameter resource definition |
| `singular_qubit_def` | `SINGULAR_QUBIT_DEF` | Marks this def as non-register |
| `register_qubit_def` | `REGISTER_QUBIT_DEF` | Marks this def as a register |
| `singular_bit_def` | `SINGULAR_BIT_DEF` | As above for bits |
| `register_bit_def` | `REGISTER_BIT_DEF` | As above for bits |
| `singular_param_def` | `SINGULAR_PARAM_DEF` | As above for parameters |
| `register_param_def` | `REGISTER_PARAM_DEF` | As above for parameters |

### Resource reference rules

| Rule name | Token kind | AST behaviour |
|-----------|-----------|---------------|
| `qubit` | `QUBIT` | `context.get_random_resource(QUBIT)` |
| `bit` | `BIT` | `context.get_random_resource(BIT)` |
| `param` | `PARAM` | `context.get_random_resource(PARAM)` |
| `singular_qubit` | `SINGULAR_QUBIT` | Marks the parent resource as non-register |
| `register_qubit` | `REGISTER_QUBIT` | Marks the parent resource as a register |
| `singular_bit` | `SINGULAR_BIT` | As above for bits |
| `register_bit` | `REGISTER_BIT` | As above for bits |
| `singular_param` | `SINGULAR_PARAM` | As above for parameters |
| `register_param` | `REGISTER_PARAM` | As above for parameters |

### Gate rules

| Rule name | Token kind | AST behaviour |
|-----------|-----------|---------------|
| `gate_op` | `GATE_OP` | Gate operation node |
| `gate_name` | `GATE_NAME` | Triggers `Gate_name(current_circuit)` with automatic constraint injection to exclude gates needing more qubits/bits than the circuit has |
| `subroutine_op` | `SUBROUTINE_OP` | Redirects to `gate_op` if no subroutines are available |
| `subroutine` | `SUBROUTINE` | Triggers `context.nn_subroutine()` |
| `float_literal` | `FLOAT_LITERAL` | Float argument node |

### Gate name rules

Each of the following rule names maps to a specific `Token_kind` that carries gate metadata (number of qubits, bits, floats) from `supported_gates.h`:

`h`, `x`, `y`, `z`, `rz`, `rx`, `ry`, `cx`, `cy`, `cz`, `ccx`, `s`, `sdg`, `t`, `tdg`, `v`, `vdg`, `ch`, `crx`, `cry`, `crz`, `u1`, `u2`, `u3`, `u`, `phased_x`, `swap`, `cswap`, `toffoli`, `cnot`, `xx`, `yy`, `zz`, `measure`, `project_z`

### Control flow rules

| Rule name | Token kind | Used by |
|-----------|-----------|---------|
| `if_stmt` / `while_stmt` / `for_stmt` / `switch_stmt` | `CF_STMT` | MAP-Elites `control_flow_ratio` feature to detect all control flow statements |
| `classical_expr` / `bool_expr` / `uint_expr` | `EXPR` | Bit reuse reset on classical expressions |

---

## How the AST builder uses rules

When `Ast::build()` encounters a term pointing to a rule, it calls `Rule::pick_branch()`, which randomly selects a branch that satisfies the current node's constraints (set by `Gate_name` to exclude unavailable gates). It then recursively expands each term in that branch.

The constraint system on `Node` tracks how many times each `Token_kind` appears in a branch, allowing `pick_branch()` to enforce things like "this `compound_stmt` must contain exactly one `QUBIT_OP`" (enforced when `nested_depth == 0` to prevent infinite recursion).
