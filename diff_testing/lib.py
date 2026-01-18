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
from collections import Counter
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

    def __init__(self, qss_name) -> None:
        self.parser = argparse.ArgumentParser()
        self.parser.add_argument(
            "--plot", action="store_true", help="Plot results after running circuit"
        )
        self.args = self.parser.parse_args()
        self.plot: bool = self.args.plot
        self.qss_name = qss_name

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

    def preprocess_counts(self, counts: Dict[Any, int]) -> Counter[int]:
        """
        Given a dict mapping binary values to number of times they appear,
        return a sorted dict with each binary tuple/string converted into a base 10 int.
        Dict is sorted by key.
        """
        out: Counter[int] = Counter()
        for k, v in counts.items():
            # k can be a tuple of bits or a string of bits
            if isinstance(k, tuple):
                key_str = "".join(str(x) for x in k).replace(" ", "")
            else:
                key_str = str(k).replace(" ", "")

            if key_str:
                out[int(key_str, 2)] = v

        return Counter(dict(sorted(out.items())))

    def ks_test(self, counts1: Counter[int], counts2: Counter[int], total_shots: int) -> float:
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

        assert (len(sample1) == total_shots) and (len(sample2) == total_shots), (
            "Sample size does not match number of shots"
        )

        res = ks_2samp(sorted(sample1), sorted(sample2), method="asymp")
        return float(res.pvalue)  # type: ignore

    def compare_statevectors(
        self, sv1: NDArray[np.complex128], sv2: NDArray[np.complex128], precision: int = 6
    ) -> float:
        return float(np.round(abs(np.vdot(sv1, sv2)), precision))

    def plot_histogram(
        self, res: Counter[int], title: str, compilation_level: int, circuit_number: int = 0
    ) -> None:
        plots_dir = self.OUTPUT_DIR / self.qss_name / f"circuit{circuit_number}"
        if not plots_dir.exists():
            plots_dir.mkdir(parents=True, exist_ok=True)

        plots_path = plots_dir / f"plot_{compilation_level}.png"

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
