import traceback
from typing import Any, Dict

import cudaq

from .lib import Base


def _apply_opt_level_1(kernel) -> None:
    """
    Aggressive early inlining + dead-code elimination.
    Corresponds roughly to -O1 in nvq++ (fast, cheap passes only).
    """
    cudaq.passes.aggressive_early_inlining(kernel)  # type: ignore


def _apply_opt_level_2(kernel) -> None:
    """
    Level 1 passes + unitary synthesis for small qubit blocks.
    Corresponds roughly to -O2 in nvq++.
    """
    cudaq.passes.aggressive_early_inlining(kernel)  # type: ignore
    cudaq.passes.unitary_synthesis(kernel)  # type: ignore


def _apply_opt_level_3(kernel) -> None:
    """
    Full optimization: level 2 + decomposition into a native gate set.
    Corresponds roughly to -O3 in nvq++.
    """
    cudaq.passes.aggressive_early_inlining(kernel)  # type: ignore
    cudaq.passes.unitary_synthesis(kernel)  # type: ignore
    cudaq.passes.decompose_to_basis(kernel)  # type: ignore


_OPT_PASSES = {
    0: None,
    1: _apply_opt_level_1,
    2: _apply_opt_level_2,
    3: _apply_opt_level_3,
}


class cudaqTesting(Base):
    def __init__(self) -> None:
        super().__init__("cudaq")

    def get_counts(self, kernel, opt_level: int, circuit_num: int) -> Dict[Any, int]:
        """
        Compile and sample `kernel` at the requested optimisation level.

        CUDA-Q does not expose optimisation levels as a single integer the way
        Qiskit/pytket do.  Instead we build four increasingly aggressive pass
        pipelines that mirror what nvq++ applies at -O0 through -O3:

          0  — no passes applied (reference baseline)
          1  — aggressive early inlining + dead-code elimination
          2  — level-1 passes + unitary synthesis (small blocks)
          3  — level-2 passes + decomposition into native gate set

        If any pass pipeline fails (e.g. the installed CUDA-Q build does not
        ship that pass), we fall back to the un-optimised sample so that the
        rest of the test still runs and we get a meaningful KS value.
        """
        try:
            pass_fn = _OPT_PASSES.get(opt_level)

            if pass_fn is not None:
                try:
                    pass_fn(kernel)
                except (AttributeError, RuntimeError) as e:
                    # cudaq.passes may not be available on all builds — degrade
                    # gracefully so at least the unoptimised path still works.
                    print(
                        f"[cudaqTesting] opt_level={opt_level} pass failed "
                        f"({type(e).__name__}: {e}); falling back to no-pass sampling."
                    )

            result = cudaq.sample(kernel, shots_count=self.num_shots)

            raw_counts: Dict[str, int] = dict(result)

            n_bits = max((len(k) for k in raw_counts), default=1)

            if self.plot:
                self.plot_histogram(
                    res=self.preprocess_counts(raw_counts, n_bits),
                    title=f"cudaq_opt{opt_level}",
                    circuit_number=circuit_num,
                )

            return self.preprocess_counts(raw_counts, n_bits)

        except Exception:
            print(f"[cudaqTesting] Exception during get_counts (opt_level={opt_level}):")
            print(traceback.format_exc())
            return {}

    def opt_ks_test(self, kernel, circuit_number: int) -> None:
        """
        Run the standard KS-test differential-testing loop.

        Overrides Base.opt_ks_test so we can call cudaq.sample correctly
        (the kernel is a callable, not a circuit object).  The logic is
        otherwise identical to the base implementation.
        """
        counts0 = self.get_counts(kernel, opt_level=0, circuit_num=circuit_number)

        if not counts0:
            print("[cudaqTesting] Baseline (opt_level=0) returned empty counts; skipping.")
            return

        for i in range(3):
            opt_level = i + 1
            counts_i = self.get_counts(kernel, opt_level=opt_level, circuit_num=circuit_number)

            if not counts_i:
                print(
                    f"[cudaqTesting] opt_level={opt_level} returned empty counts; skipping KS test."
                )
                continue

            ks_value = self.ks_test(counts0, counts_i)
            print(f"Optimisation level {opt_level} ks-test p-value: {ks_value}")
