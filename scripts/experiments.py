import argparse
import csv
import json
import statistics
from collections import defaultdict
from dataclasses import dataclass
from itertools import zip_longest
from pathlib import Path
from typing import Dict, List

from params import CPU_COUNT, SIMULATION_CAP
from utils import Color, Run, Run_mode, log

from .qf import Fuzz

N_REPEATS = 10
ARCHIVE_FILES = ("init_archive.json", "final_archive.json")
EXPERIMENTS = ["map-elites-scaling", "gen-timing", "val-timing", "feature-space-vis"]


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


def _make_latex_scatter_fig(name: str, x_label: str, y_label: str, data_table: str):
    """Create latex axis template for a 2D feature space scatter plot"""
    return f"""\\begin{{figure}}[h!]
    \\centering
    \\begin{{tikzpicture}}
        \\begin{{axis}}[
            width=12cm,
            height=9cm,
            xlabel={{{x_label}}},
            ylabel={{{y_label}}},
            title={{Feature Space Coverage}},
            grid=both,
            grid style={{line width=0.3pt, draw=gray!30}},
            minor grid style={{line width=0.15pt, draw=gray!15}},
            minor tick num=1,
            tick label style={{font=\\small}},
            label style={{font=\\small}},
            title style={{font=\\small\\bfseries}},
            colorbar,
            colorbar style={{ylabel={{Cell Quality}}}},
            colormap/viridis,
            enlargelimits=0.05,
        ]

        \\addplot[
            scatter,
            only marks,
            mark=*,
            mark size=3.5pt,
            scatter src=explicit,
            fill opacity=0.85,
            draw=black!70,
            line width=0.5pt
        ] table [meta=quality] {{
            x y quality
{data_table}
        }};

        \\end{{axis}}
    \\end{{tikzpicture}}
    \\label{{fig:{name}}}
\\end{{figure}}"""


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
        name: str,
        nproc: int,
        seed: int | None = None,
    ) -> None:
        super().__init__(name, nproc, False, seed, Run_mode.CI, False)
        self.json_path = self.current_output_dir / "map_elites_scaling.json"

    def _get_res_json(self, results_per_archive: dict[str, list[MapElitesScalingRes]]):
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

    def _map_elites_scaling_plot(self, res_json: Dict, n_repeats: int):
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

    def _map_elites_violin_plot(self, res_json: Dict, n_genomes_sweep: List[int]):
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

    def _feature_space_scatter_plot(self):
        for archive_name in ["final_archive.json", "init_archive.json"]:
            json_path = self.current_output_dir / archive_name
            if not json_path.exists():
                print(f"Archive not found at {json_path}")
                return

            with open(json_path, "r") as f:
                data = json.load(f)

            dims = data.get("dims")

            assert len(dims) == 2

            x, y = dims[0], dims[1]

            x_label = x.get("name", "x_label")
            y_label = y.get("name", "y_label")

            assert x.get("bin_width", 0) == 1 and y.get("bin_width", 0) == 1

            n_columns = x.get("n_bins", 0) + 1

            table_rows = []
            for cell in data.get("cells", []):
                if cell.get("occupied"):
                    x = cell.get("index", 0) % n_columns
                    y = cell.get("index", 0) // n_columns
                    q = cell.get("quality", 0)

                    table_rows.append(f"            {x} {y} {q:.4f}")

            if not table_rows:
                print("No occupied cells found to plot")
                return

            data_table = "\n".join(table_rows)

            latex_fig = _make_latex_scatter_fig(
                f"{archive_name.replace('.json', '')}", x_label, y_label, data_table
            )

            out_path = self.current_output_dir / f"{archive_name}_feature_space.tex"
            with open(out_path, "w") as f:
                f.write(latex_fig)

    def _get_map_elites_data(self, json_path: Path) -> MapElitesData:
        if not json_path.exists():
            raise FileNotFoundError(f"Archive not found at {json_path}")

        with open(json_path) as f:
            data = json.load(f)

        cells = data["cells"]
        occupied = [c for c in cells if c["occupied"]]
        n_occupied = len(occupied)
        qualities = [c["quality"] for c in occupied]

        return MapElitesData(n_occupied, qualities)

    def _map_elites_scaling(self, n_genomes_sweep: List[int], n_repeats: int) -> None:
        log("\n " + "=" * 60, Color.BLUE)
        log("  MAP Elites scaling experiment", Color.BLUE)
        log(f"  Grammar      : {self.name}", Color.BLUE)
        log(f"  Genome counts: {n_genomes_sweep}", Color.BLUE)
        log(f"  Repeats/point: {n_repeats}", Color.BLUE)
        log("=" * 60 + "\n", Color.BLUE)

        if self.json_path.exists():
            with open(self.json_path, "r") as f:
                res_json = json.load(f)
                self._map_elites_scaling_plot(res_json, n_repeats)
                self._map_elites_violin_plot(res_json, n_genomes_sweep)

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
                        data = self._get_map_elites_data(json_path)

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
                res_json = self._get_res_json(res)
                json.dump(res_json, f, indent=4)
                self._map_elites_scaling_plot(res_json, n_repeats)
                self._map_elites_violin_plot(res_json, n_genomes_sweep)

    def _gen_timing(self, num_genomes_sweep: List[int]):
        log("\n " + "=" * 60, Color.BLUE)
        log("  Generation timing experiment", Color.BLUE)
        log(f"  Grammar      : {self.name}", Color.BLUE)
        log(f"  Genome counts: {num_genomes_sweep}", Color.BLUE)
        log("=" * 60 + "\n", Color.BLUE)

        coords = ""
        max_y = 0
        y_label = "Total Time (s)"

        for num_tests in num_genomes_sweep:
            print("num tests:", num_tests)

            _, total_time = self.generate_tests(num_tests)

            # y_val = total_time

            y_val = num_tests / total_time

            coords += f"({num_tests}, {y_val:.4f})"
            if y_val > max_y:
                max_y = y_val

        latex_fig = _make_latex_fig("gen-times", y_label, max(num_genomes_sweep), max_y, coords, 1)

        path = self.current_output_dir / "gen_timing.tex"
        with open(path, "w") as f:
            f.write(latex_fig)

    def _val_timing(self, num_genomes_sweep: List[int]):
        log("\n " + "=" * 60, Color.BLUE)
        log("  Validation (compilation and executiion) timing experiment", Color.BLUE)
        log(f"  Grammar      : {self.name}", Color.BLUE)
        log(f"  Genome counts: {num_genomes_sweep}", Color.BLUE)
        log("=" * 60 + "\n", Color.BLUE)

        coords = ""
        max_y = 0
        y_label = "Total Time (s)"

        fuzz = Fuzz(self.name, self.sim_proc, False, self.seed, Run_mode.CI, False)

        for num_tests in num_genomes_sweep:
            print("num tests:", num_tests)

            self.generate_tests(num_tests)
            _, total_time = fuzz.validate_generated_circuits()

            y_val = total_time

            coords += f"({num_tests}, {y_val:.4f})"
            if y_val > max_y:
                max_y = y_val

        latex_fig = _make_latex_fig("val-times", y_label, max(num_genomes_sweep), max_y, coords, 1)

        path = self.current_output_dir / "val_timing.tex"
        with open(path, "w") as f:
            f.write(latex_fig)

    def run(self, args):
        if args.exp == "map-elites-scaling":
            self.map_elites = True
            self._map_elites_scaling(args.num_tests, args.n_repeats)
        elif args.exp == "gen-timing":
            self._gen_timing(args.num_tests)
        elif args.exp == "val-timing":
            self._val_timing(args.num_tests)
        elif args.exp == "feature-space-vis":
            self._feature_space_scatter_plot()


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
    parser.add_argument("--seed", type=int, help="Seed for random number generator", default=None)
    parser.add_argument("--nproc", type=int, default=CPU_COUNT, help="Num workers")
    parser.add_argument(
        "--n_repeats",
        type=int,
        help="Number of times to repeat each experiment iteration",
        default=N_REPEATS,
    )
    parser.add_argument(
        "--exp",
        type=str,
        help="Type of experiment to run",
        choices=EXPERIMENTS,
        default=EXPERIMENTS[0],
    )

    args = parser.parse_args()

    for g_name in args.grammars:
        Experiment(g_name, args.nproc, args.seed).run(args)
