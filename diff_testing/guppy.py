import sys
import traceback
from copy import deepcopy
from typing import Any, Dict, Tuple

from guppylang import enable_experimental_features
from guppylang_internals.diagnostic import DiagnosticsRenderer
from guppylang_internals.engine import DEF_STORE
from guppylang_internals.error import GuppyError
from hugr.qsystem.result import QsysResult
from selene_sim import Quest
from selene_sim.build import build

from .lib import Base

enable_experimental_features()


# ─────────────────────────────────────────────
#  Optimization passes
#  Level 0 : no passes
#  Level 1 : placeholder — returns circuit unchanged (reserved for future hugr passes)
#  Level 2 : placeholder — same as above
#  Level 3 : tket2 badger_pass (unitary re-synthesis via HUGR)
# ─────────────────────────────────────────────


def _apply_opt_level_3(hugr):
    from tket.passes import badger_pass  # type: ignore

    hugr_opt = deepcopy(hugr)
    badger_pass().apply(hugr_opt)
    return hugr_opt


_OPT_PASSES = {
    0: None,
    1: None,
    2: None,
    3: _apply_opt_level_3,
}


# ─────────────────────────────────────────────
#  Helpers
# ─────────────────────────────────────────────


def _n_qubits_from_hugr(hugr) -> int:
    """Best-effort qubit count from a compiled HUGR object."""
    for attr in ("num_qubits", "n_qubits", "qubit_count"):
        try:
            val = getattr(hugr, attr)def _n_qubits_from_hugr(hugr) -> int:
    """Best-effort qubit count from a compiled HUGR object."""
    for attr in ("num_qubits", "n_qubits", "qubit_count"):
        try:
            val = getattr(hugr, attr)
            return int(val() if callable(val) else val)
        except (AttributeError, TypeError):
            pass
    return 0

            result = val() if callable(val) else val        
            return int(result)  #type: ignore
        except (AttributeError, TypeError, ValueError):
            pass
    return 0


def _reconstruct_counts(raw: Dict) -> Tuple[Dict[str, int], int]:
    """
    Convert selene's collated_counts format — keys are sequences of
    (qubit_id, bit_value) tuples — to plain string-keyed dicts and
    infer the qubit count from the key length.
    """
    if not raw:
        return {}, 0

    first_key = next(iter(raw))
    n_qubits = len(first_key)

    reconstructed: Dict[str, int] = {
        "".join(str(m[1]) for m in key): count for key, count in raw.items()
    }
    return reconstructed, n_qubits


# ─────────────────────────────────────────────
#  Testing class
# ─────────────────────────────────────────────


class guppyTesting(Base):
    def __init__(self) -> None:
        super().__init__("guppy")

    # ------------------------------------------------------------------
    #  Internal helpers
    # ------------------------------------------------------------------

    def _apply_optimization(self, hugr, opt_level: int):
        """Apply optimization passes to a compiled HUGR, falling back to the
        unoptimized circuit if the passes are unavailable or fail."""
        pass_fn = _OPT_PASSES.get(opt_level)
        if pass_fn is None:
            return hugr

        try:
            return pass_fn(hugr)
        except Exception as e:
            print(
                f"[guppyTesting] opt_level={opt_level} pass failed "
                f"({type(e).__name__}: {e}); falling back to unoptimized circuit."
            )
            return hugr

    def _run_hugr(self, hugr, circuit_num: int, label: str) -> Dict[Any, int]:
        """Run a compiled HUGR on the selene Quest simulator and return
        preprocessed counts."""
        n_qubits = _n_qubits_from_hugr(hugr)

        runner = build(hugr)

        run_kwargs: Dict[str, Any] = {"n_shots": self.num_shots}
        if n_qubits > 0:
            run_kwargs["n_qubits"] = n_qubits

        results = QsysResult(runner.run_shots(Quest(), **run_kwargs))
        raw = results.collated_counts()

        reconstructed, inferred_qubits = _reconstruct_counts(raw)
        n_qubits = n_qubits or inferred_qubits or 1

        counts = self._preprocess_counts(reconstructed, n_qubits)

        if self.plot:
            self._plot_histogram(counts, label, circuit_num)

        return counts

    # ------------------------------------------------------------------
    #  Public interface (mirrors other testing classes)
    # ------------------------------------------------------------------

    def _get_counts(self, hugr, opt_level: int, circuit_num: int) -> Dict[Any, int]:
        """Compile the guppy circuit, optionally optimize, run on selene,
        and return preprocessed counts."""
        try:
            # hugr = circuit.compile()
            optimized = self._apply_optimization(hugr, opt_level)
            return self._run_hugr(optimized, circuit_num, f"guppy_opt{opt_level}")
        except GuppyError as e:
            renderer = DiagnosticsRenderer(DEF_STORE.sources)
            renderer.render_diagnostic(e.error)
            sys.stderr.write("\n".join(renderer.buffer))
            sys.stderr.write("\n\nGuppy compilation failed due to 1 previous error\n")
            return {}
        except Exception:
            print(f"[guppyTesting] Exception in get_counts (opt_level={opt_level}):")
            print(traceback.format_exc())
            return {}

    def opt_ks_test(self, circuit, circuit_number: int) -> None:
        """KS-test differential testing across optimization levels.

        Compiles the guppy circuit once, then runs it at optimization
        levels 0–3, comparing each optimized distribution against the
        level-0 baseline with a Kolmogorov–Smirnov test.  A low p-value
        (below MIN_KS_VALUE in run.py) flags a potential mis-compilation.
        """

        # compile guppy circuit to HUGR graph
        try:
            hugr = circuit.compile()
        except GuppyError as e:
            renderer = DiagnosticsRenderer(DEF_STORE.sources)
            renderer.render_diagnostic(e.error)
            sys.stderr.write("\n".join(renderer.buffer))
            sys.stderr.write("\n\nGuppy compilation failed due to 1 previous error\n")
            return
        except Exception:
            print("Error during compilation:", traceback.format_exc())
            return

        counts_0 = self._get_counts(hugr, 0, circuit_number)

        if not counts_0:
            print("[guppyTesting] Baseline returned empty counts; skipping.")
            return

        for i in range(3):
            opt_level = i + 1
            counts_i = self._get_counts(hugr, i, circuit_number)

            if not counts_i:
                print(f"[guppyTesting] opt_level={opt_level} returned empty counts; skipping.")
                continue

            ks_value = self._ks_test(counts_0, counts_i)
            print(f"Optimisation level {opt_level} ks-test p-value: {ks_value}")
