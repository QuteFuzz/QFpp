"""
Minimal differential testing library

There are 2 forms of differential testing:
- probability distribution comparison by ks-test after running on simulator
- statvector comparison

What is being tested?
- results after running circuit through specific compiler passes
- results after running circuit through compiler at different optimisation levels

Results:
- probability distributions plotted
- ks value printed in logs

Ordering (relevant for QASM conversion tests):
- pytket orders classical bits alphanumerically
- qiskit orders classical bits in the order in which they're defined in the file
- cirq returns a frozenset of measurement keys, which means the ordering is random

in order to be consistent, the insertion order in the files is naturally alphabetical,
so pytket and qiskit counts are comparable. For cirq, we sort the results, making them
also comparable.

an alternative is to just use a single qreg and creg to skip over all these ordering quirks!
"""

import argparse
import os
import traceback
from abc import ABC, abstractmethod
from pathlib import Path
from typing import Any, Dict, List

import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
from numpy.typing import NDArray
from scipy.stats import ks_2samp


class Base(ABC):
    OUTPUT_DIR = (Path(__file__).parent.parent / "outputs").resolve()
    TIMEOUT_SECONDS = 30

    def __init__(self, qss_name, endianess="little") -> None:
        self.parser = argparse.ArgumentParser()
        self.parser.add_argument(
            "--plot", action="store_true", help="Plot results after running circuit"
        )
        self.args = self.parser.parse_args()
        self.plot: bool = self.args.plot
        self.qss_name = qss_name
        self.endianess = endianess

        self.num_shots = 10 if os.environ.get("RUN_MODE") == "CI" else 1000

    @abstractmethod
    def _get_counts(self, circuit, opt_level, circuit_num) -> Dict[Any, int]:
        raise NotImplementedError("`_get_counts` func not implemented")

    @abstractmethod
    def _get_statevector(self, circuit, opt_level):
        raise NotImplementedError("`_get_statevector` func not implemented")

    def _preprocess_counts(self, counts: Dict[Any, int]) -> Dict[Any, int]:
        """
        Given a dict mapping binary values to number of times they appear,
        return a sorted dict with each binary tuple/string converted into a base 10 int.
        Dict is sorted by key.
        """
        out = {}
        for k, v in counts.items():
            if isinstance(k, str):
                str_key = k.replace(" ", "")

                if str_key.strip("10"):
                    base = 10
                else:
                    base = 2

                int_key = (
                    int(str_key, base) if self.endianess == "little" else int(str_key[::-1], base)
                )

            elif isinstance(k, tuple):
                int_key = 0
                # dynamically builds the integer (in big endian)
                for bit in k:
                    int_key = (int_key << 1) | bit

            out[int_key] = out.get(int_key, 0) + v

        return dict(sorted(out.items()))

    def _ks_test(self, counts1: Dict[Any, int], counts2: Dict[Any, int]) -> float:
        """
        Carries out K-S test on two frequency lists
        """
        sample1: List[int] = []
        sample2: List[int] = []

        for val, count in counts1.items():
            sample1 += [val] * count

        for val, count in counts2.items():
            sample2 += [val] * count

        if len(sample1) != self.num_shots or len(sample2) != self.num_shots:
            # print(counts1, " ", counts2)
            print(
                f"Sample size(sample1: {len(sample1)}, sample2: {len(sample2)})"
                f" does not match number of shots ({self.num_shots})"
            )

        res = ks_2samp(sorted(sample1), sorted(sample2), method="asymp")
        return float(res.pvalue)  # type: ignore

    def compare_statevectors(
        self, sv1: NDArray[np.complex128], sv2: NDArray[np.complex128], precision: int = 6
    ) -> float:
        return float(np.round(abs(np.vdot(sv1, sv2)), precision))

    def opt_statevector_test(self, circuit) -> None:
        """
        Runs circuit and compares statevectors
        """
        try:
            no_pass_statevector = self._get_statevector(circuit, 0)

            for i in range(3):
                pass_statevector = self._get_statevector(circuit.copy(), i + 1)

                dot_prod = self.compare_statevectors(no_pass_statevector, pass_statevector, 6)
                print("Dot product: ", dot_prod)

        except Exception:
            print("Exception :", traceback.format_exc())

    def opt_ks_test(self, circuit, circuit_number: int) -> None:
        """
        Runs circuit and compares distributions
        """

        counts1 = self._get_counts(circuit, opt_level=0, circuit_num=circuit_number)

        for i in range(3):
            counts2 = self._get_counts(circuit, opt_level=i + 1, circuit_num=circuit_number)
            ks_value = self._ks_test(counts1, counts2)
            print(f"Optimisation level {i + 1} ks-test p-value: {ks_value}")

    def _plot_ecdf(
        self, counts1: Dict[Any, int], counts2: Dict[Any, int], title: str, circuit_number: int = 0
    ) -> None:
        """
        Plots the Empirical Cumulative Distribution Function (eCDF) for two sets of counts
        to visually demonstrate the KS Test.eCDF for
        """
        plots_dir = self.OUTPUT_DIR / self.qss_name / f"circuit{circuit_number}"
        if not plots_dir.exists():
            plots_dir.mkdir(parents=True, exist_ok=True)

        plots_path = plots_dir / f"{title}_ecdf.png"

        # expand the dictionaries back into raw shot arrays
        sample1, sample2 = [], []
        for val, count in counts1.items():
            sample1.extend([val] * count)
        for val, count in counts2.items():
            sample2.extend([val] * count)

        x1 = np.sort(sample1)
        x2 = np.sort(sample2)

        y1 = np.arange(1, len(x1) + 1) / len(x1)
        y2 = np.arange(1, len(x2) + 1) / len(x2)

        plt.figure(figsize=(10, 6))

        # 'where=post' ensures the line stays flat until it hits the next data point,
        # creating the true staircase effect
        plt.step(x1, y1, label="Baseline (Opt 0)", where="post", linewidth=2, alpha=0.8)
        plt.step(x2, y2, label="Optimized", where="post", linewidth=2, linestyle="--", alpha=0.8)

        # Styling
        plt.title(f"KS Test eCDF: {title}")
        plt.xlabel("Measured State (Integer)")
        plt.ylabel("Cumulative Probability")
        plt.legend()
        plt.grid(True, alpha=0.3)

        plt.tight_layout()
        plt.savefig(plots_path)
        plt.close()

    def _plot_histogram(self, res: Dict[Any, int], title: str, circuit_number: int = 0) -> None:
        plots_dir = self.OUTPUT_DIR / self.qss_name / f"circuit{circuit_number}"
        if not plots_dir.exists():
            plots_dir.mkdir(parents=True, exist_ok=True)

        plots_path = plots_dir / f"{title}.png"

        # Plot the histogram
        values = list(res.keys())
        freqs = list(res.values())

        bar_width = 0.5
        plt.bar(values, freqs, width=bar_width, edgecolor="black")
        ax = plt.gca()
        ax.xaxis.set_major_locator(ticker.MaxNLocator(nbins=10, integer=True))
        plt.xlabel("Possible results")
        plt.ylabel("Number of occurances")
        plt.title(title)
        plt.tight_layout()
        plt.savefig(plots_path)
        plt.close()
