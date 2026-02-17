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
"""

import argparse
import os
from itertools import zip_longest
from pathlib import Path
from typing import Any, Dict, List

import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
import qnexus as qnx
from numpy.typing import NDArray
from scipy.stats import ks_2samp


class Base:
    OUTPUT_DIR = (Path(__file__).parent.parent / "outputs").resolve()
    TIMEOUT_SECONDS = 30

    def __init__(self, qss_name, native) -> None:
        self.parser = argparse.ArgumentParser()
        self.parser.add_argument(
            "--plot", action="store_true", help="Plot results after running circuit"
        )
        self.args = self.parser.parse_args()
        self.plot: bool = self.args.plot
        self.qss_name = qss_name
        self.native = native

        self.num_shots = 100000

    def qnexus_login(self) -> None:
        """
        Logs into QNexus using environment variables for running QIR jobs
        """
        user_email = os.getenv("NEXUS_USERNAME")
        user_password = os.getenv("NEXUS_PWD")

        if not user_email or not user_password:
            print("Error: NEXUS_USERNAME or NEXUS_PWD environment variables are not set.")
            return

        try:
            qnx.client.auth.login_no_interaction(user_email, user_password)  # type: ignore
        except Exception as e:
            print("Error logging into Nexus:", e)

    def qnexus_check_login_status(self) -> bool:
        """
        Checks if logged into QNexus to prevent trying login multiple times
        """
        try:
            qnx.teams.get_all()
            return True
        except Exception:
            return False

    def preprocess_counts(self, counts: Dict[Any, int], n_qubits: int) -> Dict[Any, int]:
        """
        Given a dict mapping binary values to number of times they appear,
        return a sorted dict with each binary tuple/string converted into a base 10 int.
        Dict is sorted by key.
        """
        out = {}
        for k, v in counts.items():
            # k can be a tuple of bits or a string of bits
            if isinstance(k, tuple):
                key_str = "".join(str(x) for x in k).replace(" ", "")
            else:
                key_str = str(k).replace(" ", "")

            if self.qss_name == "qiskit":
                key_str = key_str[::-1]  # flip to match <0001| indexed as [0,1,2,3], flip

            if not self.native:
                assert int(key_str[n_qubits:], 2) == 0, (
                    f"Bits not in [0, {n_qubits - 1}] must have no information"
                )

                key_str = key_str[:n_qubits]  # only consider bits which correspond to actual qubits

                # if self.qss_name == "qiskit":
                # key_str = key_str[::-1]  # flip to match <0001| indexed as [0,1,2,3], flip

            out[int(key_str, 2)] = v

        return dict(sorted(out.items()))

    def ks_test(self, counts1: Dict[Any, int], counts2: Dict[Any, int]) -> float:
        """
        Carries out K-S test on two frequency lists
        """
        sample1: List[int] = []
        sample2: List[int] = []

        for p1, p2 in zip_longest(counts1.items(), counts2.items(), fillvalue=None):
            if p1:
                sample1 += [p1[0]] * p1[1]

            if p2:
                sample2 += [p2[0]] * p2[1]

        assert (len(sample1) == self.num_shots) and (len(sample2) == self.num_shots), (
            f"Sample size(sample1: {len(sample1)}, sample2: {len(sample2)})"
            f"does not match number of shots ({self.num_shots})"
        )

        res = ks_2samp(sorted(sample1), sorted(sample2), method="asymp")
        return float(res.pvalue)  # type: ignore

    def compare_statevectors(
        self, sv1: NDArray[np.complex128], sv2: NDArray[np.complex128], precision: int = 6
    ) -> float:
        return float(np.round(abs(np.vdot(sv1, sv2)), precision))

    def plot_histogram(self, res: Dict[Any, int], title: str, circuit_number: int = 0) -> None:
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
