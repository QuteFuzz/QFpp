import argparse
import csv
import json
import statistics
from collections import defaultdict
from dataclasses import dataclass
from datetime import datetime
from itertools import zip_longest
from pathlib import Path
from typing import Dict, List

from params import CPU_COUNT, SIMULATION_CAP
from utils import Color, Run, Run_mode, log

N_REPEATS = 10
ARCHIVE_FILES = ("init_archive.json", "final_archive.json")


def _make_latex_fig(name: str, y_label: str, x_max: int, y_max: float, coords: str, n_repeats: int):
    """Create latex axis template with experiment results"""
    return (
        r"""
\begin{figure}[h!]
    \centering
    \begin{tikzpicture}
        \begin{axis}[
            width=12cm,
            height=7cm,
            xlabel      = {num of initial seed circuits},
            ylabel      = {"""
        + y_label
        + r"""},
            title       = {"""
        + y_label
        + r""" vs init genomes},
            xmin        = 0,  xmax        = """
        + str(x_max)
        + r""",
            ymin        = 0,  ymax        = """
        + str(y_max)
        + r""",
            xtick       = data,
            grid        = both,
            grid style  = {line width=0.3pt, draw=gray!30},
            minor grid style = {line width=0.15pt, draw=gray!15},
            minor tick num = 1,
            tick label style = {font=\small},
            label style      = {font=\small},
            title style      = {font=\small\bfseries},
            error bars/y dir      = both,
            error bars/y explicit = true,
            legend pos   = south east,
            legend style = {font=\small, inner sep=2pt},
        ]
        \addplot[
            color      = blue,
            mark       = *,
            mark size  = 2pt,
            line width = 1.2pt,
            error bars/error bar style = {line width=0.6pt, color=blue},
            error bars/error mark options = {scale=0.5, rotate=90, color=blue},
        ] coordinates {"""
        + coords
        + r"""};
        \addlegendentry{mean $\pm$ std ($n="""
        + str(n_repeats)
        + r"""$ repeats)}
        \end{axis}
    \end{tikzpicture}
    \label{fig:"""
        + name
        + r"""}
\end{figure}
    """
    )


def _make_latex_violin_fig(name: str, y_label: str, y_max: float, csv_filename: str):
    """Create latex axis template for violin plots using a generated CSV file"""
    return (
        r"""
\begin{figure}[h!]
    \centering
    \begin{tikzpicture}
        \violinplotsset{
            width=12cm,
            height=7cm,
            ylabel      = {"""
        + y_label
        + r"""},
            title       = {"""
        + y_label
        + r""" vs init genomes},
            ymin        = 0,  ymax = """
        + str(y_max)
        + r""",
            xmin        = 0,  xmax = 3,
            xtick       = {1, 2},
            xticklabels = {Initial, Final},
            grid        = both,
            grid style  = {line width=0.3pt, draw=gray!30},
            minor grid style = {line width=0.15pt, draw=gray!15},
            minor tick num = 1,
            tick label style = {font=\small},
            label style      = {font=\small},
            title style      = {font=\small\bfseries},
        }

        \violinplot[
            col sep=comma,
            index=initial,
            relative position=1,
            color=orange,
            fill opacity=0.5
        ]{"""
        + csv_filename
        + r"""}

        \violinplot[
            col sep=comma,
            index=final,
            relative position=2,
            color=blue,
            fill opacity=0.5
        ]{"""
        + csv_filename
        + r"""}

    \end{tikzpicture}
    \label{fig:"""
        + name
        + r"""}
\end{figure}
    """
    )


@dataclass
class MapElitesData:
    n_occupied_cells: int
    qualities: List[float]


@dataclass
class MapElitesScalingRes:
    n_genomes: int
    n_repeats: int
    n_occupied: List[int]
    qualities: List[float]

    def occupied_mean(self):
        return statistics.mean(self.n_occupied) if self.n_occupied else 0.0

    def qualities_mean(self):
        return statistics.mean(self.qualities) if self.qualities else 0.0

    def occupied_stdev(self):
        return statistics.pstdev(self.n_occupied) if len(self.n_occupied) > 1 else 0.0

    def qualities_stdev(self):
        return statistics.pstdev(self.n_occupied) if len(self.n_occupied) > 1 else 0.0


class Experiment(Run):
    def __init__(
        self,
        timestamp: str,
        name: str,
        nproc: int,
        map_elites_json_file: str | None,
        seed: int | None = None,
    ) -> None:
        super().__init__(timestamp, name, nproc, True, seed, Run_mode.CI, False)
        self.json_path = self.current_output_dir / (
            "me_exp.json" if map_elites_json_file is None else map_elites_json_file
        )

    def get_res_json(self, results_per_archive: dict[str, list[MapElitesScalingRes]]):
        out = defaultdict(dict)

        for archive_file, results in results_per_archive.items():
            for res in results:
                out[archive_file][res.n_genomes] = {
                    "occupied": res.n_occupied,
                    "qualities": res.qualities,
                    "occupied_mean": res.occupied_mean(),
                    "occupied_stdev": res.occupied_stdev(),
                    "qualities_mean": res.qualities_mean(),
                    "qualities_stdev": res.qualities_stdev(),
                }
        return out

    def to_latex_plot(self, res_json: Dict, n_repeats: int):
        for element in ["occupied", "qualities"]:
            mean_key = element + "_mean"
            stdev_key = element + "_stdev"

            for archive_file in ARCHIVE_FILES:
                results = res_json.get(archive_file, {})

                if len(results) == 0:
                    raise Exception("No results read from json")

                def _coords() -> str:
                    parts = []
                    for n_genomes, res in results.items():
                        parts.append(
                            f"  ({n_genomes}, {res[mean_key]:.4f}) +- (0, {res[stdev_key]:.4f})"
                        )
                    return "\n".join(parts)

                archive_name = archive_file.replace(".json", "")

                n_genomes = [int(k) for k in results.keys()]
                x_max = int(max(n_genomes) * 1.1)
                y_max = max(v[mean_key] + v[stdev_key] for v in results.values()) * 1.15

                latex_fig = _make_latex_fig(
                    "occ" + archive_name,
                    f"Average {element} ({archive_name})",
                    x_max,
                    y_max,
                    _coords(),
                    n_repeats,
                )

                path = self.current_output_dir / f"{archive_name}_{element}.tex"
                with open(path, "w") as f:
                    f.write(latex_fig)

    def to_latex_violin_plot(self, res_json: Dict, n_genomes_sweep: List[int]):
        for target_n_genomes in n_genomes_sweep:
            csv_filename = f"violin_data_{target_n_genomes}_genomes.csv"
            csv_path = self.current_output_dir / csv_filename

            init_data = res_json["init_archive.json"][str(target_n_genomes)]["qualities"]
            final_data = res_json["final_archive.json"][str(target_n_genomes)]["qualities"]

            with open(csv_path, "w", newline="") as f:
                writer = csv.writer(f)
                writer.writerow(["initial", "final"])
                for row in zip_longest(init_data, final_data, fillvalue=""):
                    writer.writerow(row)

            y_max = max(init_data + final_data) * 1.15 if (init_data or final_data) else 2.0
            latex_fig = _make_latex_violin_fig(
                name=f"violin_{target_n_genomes}",
                y_label="Average cell quality",
                y_max=y_max,
                csv_filename=csv_filename,
            )

            tex_path = self.current_output_dir / f"violin_{target_n_genomes}_qualities.tex"
            with open(tex_path, "w") as f:
                f.write(latex_fig)

    def get_map_elites_data(self, json_path: Path) -> MapElitesData:
        if not json_path.exists():
            raise FileNotFoundError(f"Archive not found at {json_path}")

        with open(json_path) as f:
            data = json.load(f)

        cells = data["cells"]
        occupied = [c for c in cells if c["occupied"]]
        n_occupied = len(occupied)
        qualities = [c["quality"] for c in occupied]

        return MapElitesData(n_occupied, qualities)

    def map_elites_scaling(self, n_genomes_sweep: List[int], n_repeats: int) -> None:
        log("\n " + "=" * 60, Color.BLUE)
        log("  MAP Elites scaling experiment", Color.BLUE)
        log(f"  Grammar      : {self.name}", Color.BLUE)
        log(f"  Genome counts: {n_genomes_sweep}", Color.BLUE)
        log(f"  Repeats/point: {n_repeats}", Color.BLUE)
        log("=" * 60 + "\n", Color.BLUE)

        if self.json_path.exists():
            with open(self.json_path, "r") as f:
                res_json = json.load(f)
                self.to_latex_plot(res_json, n_repeats)
                self.to_latex_violin_plot(res_json, n_genomes_sweep)

        else:
            res: Dict[str, List[MapElitesScalingRes]] = defaultdict(list)

            for n_genomes in n_genomes_sweep:
                log(f"n_genomes: {n_genomes}")

                n_occupied_per_archive: Dict[str, List[int]] = defaultdict(list)
                qualities_per_archive: Dict[str, List[float]] = defaultdict(list)

                for _ in range(n_repeats):
                    self.generate_tests(n_genomes)

                    for json_file in ARCHIVE_FILES:
                        json_path = self.current_output_dir / json_file
                        data = self.get_map_elites_data(json_path)

                        n_occupied_per_archive[json_file].append(data.n_occupied_cells)
                        qualities_per_archive[json_file].extend(data.qualities)

                for json_file in ARCHIVE_FILES:
                    res[json_file].append(
                        MapElitesScalingRes(
                            n_genomes,
                            n_repeats,
                            n_occupied_per_archive[json_file],
                            qualities_per_archive[json_file],
                        )
                    )

            with open(self.json_path, "w") as f:
                res_json = self.get_res_json(res)
                json.dump(res_json, f, indent=4)
                self.to_latex_plot(res_json, n_repeats)
                self.to_latex_violin_plot(res_json, n_genomes_sweep)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="qf++ runner")
    parser.add_argument(
        "--num-tests",
        type=int,
        help="Sweep over these number of programs",
        nargs="*",
        default=[1],
    )
    parser.add_argument(
        "--grammars",
        nargs="+",
        default=SIMULATION_CAP.keys(),
        help="Grammars to test (default: all)",
    )
    parser.add_argument(
        "--map-elites-json", help="Raw map elites data from previous run", type=str, default=None
    )
    parser.add_argument("--seed", type=int, help="Seed for random number generator", default=None)
    parser.add_argument("--nproc", type=int, default=CPU_COUNT, help="Num workers")
    parser.add_argument(
        "--n_repeats",
        type=int,
        help="Number of times to repeat each experiment iteration",
        default=N_REPEATS,
    )

    args = parser.parse_args()

    run_timestamp = datetime.now().strftime("%d_%m_%Y_%H_%M_%S")

    for g_name in args.grammars:
        Experiment(
            run_timestamp, g_name, args.nproc, args.seed, args.map_elites_json
        ).map_elites_scaling(args.num_tests, args.n_repeats)
