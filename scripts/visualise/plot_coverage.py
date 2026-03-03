import json
from matplotlib.axes import Axes
import matplotlib.colors as mcolors
from matplotlib.figure import Figure
import matplotlib.pyplot as plt
import numpy as np
import umap
from scipy.stats import gaussian_kde
from numpy.typing import NDArray
import argparse
from dataclasses import dataclass
from enum import Enum
from typing import Dict, List

"""
    Visualisation script for test case coverage information

    Written with a lot of help from Claude
"""

OCCUPIED_CELLS_MIN = 4

@dataclass(frozen=True)
class Palette:
    name:         str
    bg:           str   # figure background
    bg_ax:        str   # axes background
    grid:         str   # gridlines / spines
    tick:         str   # tick labels
    text_primary: str   # titles, labels
    text_secondary: str # subtitles, axis labels
    text_dim:     str   # annotations, minor text
    accent:       str   # highlights, mean lines, arrows
    bar_edge:     str   # histogram bar edges
    scatter_edge: str   # scatter point edges
    cmap:         str   # matplotlib colormap name for quality
    kde_cmap:     str   # colormap for density contours
    stats_box_bg: str   # stats text box background
    stats_box_edge: str


class Theme(Enum):
    DARK       = "dark"
    WARM_LIGHT = "warm_light"
    COOL_LIGHT = "cool_light"


PALETTES: dict[Theme, Palette] = {

    Theme.DARK: Palette(
        name          = "Dark",
        bg            = "#080f1a",
        bg_ax         = "#080f1a",
        grid          = "#223344",
        tick          = "#667788",
        text_primary  = "#ffffff",
        text_secondary= "#aaccdd",
        text_dim      = "#667788",
        accent        = "#ffcc88",
        bar_edge      = "#080f1a",
        scatter_edge  = "#ffffff",
        cmap          = "plasma",
        kde_cmap      = "Blues",
        stats_box_bg  = "#0d1b2a",
        stats_box_edge= "#223344",
    ),

    # warm off-white — ink on paper feel
    Theme.WARM_LIGHT: Palette(
        name          = "Warm Light",
        bg            = "#faf7f2",
        bg_ax         = "#f5f0e8",
        grid          = "#d8cfc4",
        tick          = "#7a6e64",
        text_primary  = "#2b2218",
        text_secondary= "#4a3f35",
        text_dim      = "#9a8e84",
        accent        = "#c0392b",
        bar_edge      = "#faf7f2",
        scatter_edge  = "#2b2218",
        cmap          = "YlOrRd",
        kde_cmap      = "YlOrBr",
        stats_box_bg  = "#eee8de",
        stats_box_edge= "#c8bfb4",
    ),

    # cool blue-grey — technical / scientific report feel
    Theme.COOL_LIGHT: Palette(
        name          = "Cool Light",
        bg            = "#f4f6f9",
        bg_ax         = "#edf0f5",
        grid          = "#c8d0dc",
        tick          = "#6a7a8e",
        text_primary  = "#1a2332",
        text_secondary= "#344556",
        text_dim      = "#8a9aaa",
        accent        = "#2563eb",
        bar_edge      = "#f4f6f9",
        scatter_edge  = "#1a2332",
        cmap          = "viridis",
        kde_cmap      = "GnBu",
        stats_box_bg  = "#e4e8f0",
        stats_box_edge= "#b8c4d0",
    ),
}


def apply_theme(fig : Figure, axes : Dict[str, Axes], p: Palette):
    """Apply a palette to an existing figure and list of axes."""
    fig.patch.set_facecolor(p.bg)
    for ax in axes.values():
        ax.set_facecolor(p.bg_ax)
        for spine in ax.spines.values():
            spine.set_edgecolor(p.grid)
        ax.tick_params(colors=p.tick, labelsize=8)
        ax.xaxis.label.set_color(p.text_secondary)
        ax.yaxis.label.set_color(p.text_secondary)
        ax.title.set_color(p.text_primary)
        ax.grid(True, linewidth=0.4, alpha=0.6)

def load_archive(path : str):
    with open(path) as f:
        data = json.load(f)
    return data["dims"], data["cells"]


def coords_from_flat(flat_idx : int, dims : List[Dict]):
    coords = []
    for d in reversed(dims):
        coords.append(flat_idx % d["bins"])
        flat_idx //= d["bins"]
    return list(reversed(coords))

def dim_reduction(X : NDArray, seed : int | None) -> NDArray:
    if X.shape[1] > 2:
        reducer = umap.UMAP(
            n_components=2,
            n_neighbors=min(15, len(X) - 1),
            min_dist=0.1,
            random_state=seed,
            metric="euclidean",
        )

        return reducer.fit_transform(X)
    else:
        return X  # already 2D

def setup_canvas(title : str, qualities : NDArray, p : Palette):
    fig = plt.figure(figsize=(14, 9))
    fig.suptitle(title, fontsize=13, y=0.98, color=p.text_primary)

    gs = fig.add_gridspec(
        2, 3, width_ratios=[3, 1.2, 1.2], height_ratios=[1, 1], hspace=0.4, wspace=0.35
    )

    axes = {
        "scatter" : fig.add_subplot(gs[:, 0]),
        "text_stats" : fig.add_subplot(gs[0, 1]),
        "hist" : fig.add_subplot(gs[1, 1]),
        "bar" : fig.add_subplot(gs[0, 2])
    }

    for ax in axes.values():
        ax.tick_params(labelsize=8)

    cmap = plt.get_cmap(p.cmap)
    norm = mcolors.Normalize(vmin=qualities.min(), vmax=qualities.max())

    apply_theme(fig, axes, p)

    return cmap, norm, fig, axes

def scatter_plot(X2d : NDArray, n_dims : int, axis : Axes, qualities : NDArray, palette : Palette, norm : mcolors.Normalize):
    cmap = plt.get_cmap(palette.cmap)

    if len(X2d) > OCCUPIED_CELLS_MIN:
        try:
            # background KDE to show density (cluster detection)
            kde = gaussian_kde(X2d.T, bw_method=0.3)
            xmin, xmax = X2d[:, 0].min() - 0.5, X2d[:, 0].max() + 0.5
            ymin, ymax = X2d[:, 1].min() - 0.5, X2d[:, 1].max() + 0.5
            xx, yy = np.meshgrid(np.linspace(xmin, xmax, 120), np.linspace(ymin, ymax, 120))
            zz = kde(np.vstack([xx.ravel(), yy.ravel()])).reshape(xx.shape)
            # normalise density to [0,1] for alpha
            zz = (zz - zz.min()) / (zz.max() - zz.min() + 1e-9)
            axis.contourf(xx, yy, zz, levels=12, cmap=cmap, alpha=0.25)

        except Exception:
            pass  # KDE can fail with very few points

    sc = axis.scatter(
        X2d[:, 0],
        X2d[:, 1],
        c=qualities,
        cmap=cmap,
        norm=norm,
        s=60,
        alpha=0.85,
        linewidths=0.4,
        edgecolors=palette.scatter_edge,
        zorder=3,
    )

    cbar = plt.colorbar(sc, ax=axis, fraction=0.03, pad=0.02)
    cbar.set_label("quality", fontsize=8, color=palette.text_secondary)
    cbar.ax.tick_params(labelsize=7, color=palette.text_secondary)

    axis.set_title(
        "Feature Space  (UMAP projection)" if n_dims > 2 else "Feature Space",
        fontsize=9,
        pad=8,
    )
    axis.set_xlabel(
        "UMAP dim 1" if n_dims > 2 else dims[0]["name"], fontsize=8
    )
    axis.set_ylabel(
        "UMAP dim 2" if n_dims > 2 else dims[1]["name"], fontsize=8
    )

    # annotate sparsely: top-5 quality points
    top5 = np.argsort(qualities)[-5:]
    for idx in top5:
        axis.annotate(
            f"q={qualities[idx]:.2f}",
            xy=(X2d[idx, 0], X2d[idx, 1]),
            xytext=(8, 8),
            textcoords="offset points",
            fontsize=6.5,
            color=palette.text_primary,
            arrowprops=dict(arrowstyle="-", lw=0.6, color=palette.text_secondary),
        )

def quality_histogram(axis : Axes, qualities : NDArray, cmap : mcolors.Colormap, norm : mcolors.Normalize):
    bins = np.linspace(0, 1, 16)

    n, _, patches = axis.hist(qualities, bins=bins)

    for patch, left_edge in zip(patches, bins[:-1]):
        bin_mid = left_edge + (bins[1] - bins[0]) / 2
        patch.set_facecolor(cmap(norm(bin_mid)))
        patch.set_alpha(0.85)

    axis.set_title("Quality distribution", fontsize=8, pad=6)
    axis.set_xlabel("quality", fontsize=7)
    axis.set_ylabel("count", fontsize=7)
    axis.axvline(
        qualities.mean(),
        lw=1,
        linestyle="--",
        label=f"mean={qualities.mean():.2f}",
    )
    axis.legend(fontsize=7)


def dim_occupancies(axis : Axes, dims : List[Dict], cells : List[Dict], cmap : mcolors.Colormap, norm : mcolors.Normalize):
    dim_occupancies = []
    for di, d in enumerate(dims):
        bin_filled = 0
        for b in range(d["bins"]):
            for c in cells:
                if c["occupied"]:
                    coords = coords_from_flat(c["index"], dims)
                    if coords[di] == b:
                        bin_filled += 1
                        break
        dim_occupancies.append(bin_filled / d["bins"])#

    y_pos = np.arange(len(dims))
    axis.set_yticks(y_pos)
    axis.set_yticklabels([d["name"] for d in dims], fontsize=7)
    axis.set_xlim(0, 1)
    axis.set_xlabel("bin coverage", fontsize=7)
    axis.set_title("Per-dim coverage", fontsize=8, pad=6)
    axis.axvline(1.0, lw=0.5, linestyle="--")

    bars = axis.barh(y_pos, dim_occupancies)

    for bar, v in zip(bars, dim_occupancies):
        bar.set_facecolor(cmap(norm(v)))
        axis.text(
            v + 0.02,
            bar.get_y() + bar.get_height() / 2,
            f"{v:.0%}",
            va="center",
            fontsize=7,
        )

def stats_text(X2d : NDArray, axis : Axes, cells : List[Dict], occupied : List[Dict], qualities : NDArray, p : Palette):
    total_cells = len(cells)
    filled_cells = len(occupied)
    coverage_pct = 100 * filled_cells / total_cells

    # cluster score: ratio of std to range in UMAP space (low = clustered)
    spread = np.std(X2d, axis=0).mean()
    full_range = (X2d.max(axis=0) - X2d.min(axis=0)).mean()
    cluster_score = spread / (full_range + 1e-9)  # 0=clustered, 0.5=spread

    stats_text = (
        f"cells:      {filled_cells} / {total_cells}\n"
        f"coverage:   {coverage_pct:.1f}%\n"
        f"\n"
        f"quality\n"
        f"  mean:     {qualities.mean():.3f}\n"
        f"  max:      {qualities.max():.3f}\n"
        f"  min:      {qualities.min():.3f}\n"
        f"\n"
        f"spread score: {cluster_score:.2f}\n"
        f"(higher = more spread out)"
    )

    axis.axis("off")
    axis.text(
        0.05,
        0.95,
        stats_text,
        transform=axis.transAxes,
        fontsize=8,
        va="top",
        color=p.text_primary,
        fontfamily="monospace",
        bbox=dict(boxstyle="round", alpha=0.8, facecolor=p.stats_box_bg, edgecolor=p.stats_box_edge)
    )

def plot_coverage(dims : List[Dict], cells : List[Dict], palette : Palette, seed : int | None, title : str="Feature Space Coverage"):
    # build feature matrix — only occupied cells
    occupied = [c for c in cells if c["occupied"]]
    if len(occupied) < OCCUPIED_CELLS_MIN:
        print("Not enough occupied cells to plot")
        return

    X = np.array([coords_from_flat(c["index"], dims) for c in occupied], dtype=float)

    # normalise each feature axis to [0,1]
    for i, d in enumerate(dims):
        if d["bins"] > 1:
            X[:, i] /= d["bins"] - 1

    qualities = np.array([c["quality"] for c in occupied])

    X2d = dim_reduction(X, seed)

    # setup figure
    cmap, norm, fig, axes = setup_canvas(title, qualities, palette)

    n_dims = X.shape[1]
    scatter_plot(X2d, n_dims, axes["scatter"], qualities, palette, norm)

    quality_histogram(axes["hist"], qualities, cmap, norm)

    dim_occupancies(axes["bar"], dims, cells, cmap, norm)

    stats_text(X2d, axes["text_stats"], cells, occupied, qualities, palette)

    plt.savefig("coverage.png", dpi=150, bbox_inches="tight", facecolor=fig.get_facecolor())
    # plt.show() using non-iteractive backend


def parse():
    parser = argparse.ArgumentParser(description="Plot coverage information from JSON file")
    parser.add_argument(
        "--json",
        type=str,
        help="Path to JSON file",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=42,
        help="Seed for plotting and np random"
    )
    parser.add_argument(
        "--theme",
        type=str,
        help="Theme (dark, cool-light, warm-light)",
        default="warm-light"
    )
    return parser.parse_args()


if __name__ == "__main__":
    args = parse()

    if args.theme == "warm-light":
        palette = PALETTES[Theme.WARM_LIGHT]
    elif args.theme == "cool-light":
        palette = PALETTES[Theme.COOL_LIGHT]
    else:
        palette = PALETTES[Theme.DARK]

    if (args.json is None):
        # synthetic test data
        dims = [
            {"name": "control_flow_depth", "bins": 4},
            {"name": "subroutine_depth", "bins": 3},
            {"name": "statement_count", "bins": 3},
            {"name": "has_parametrised", "bins": 2},
        ]
        total = 1
        for d in dims:
            total *= d["bins"]

        if args.seed is not None:
            np.random.seed(args.seed)

        cells = [
            {"index": i, "occupied": np.random.rand() > 0.4, "quality": float(np.random.rand())}
            for i in range(total)
        ]
    else:
        dims, cells = load_archive(args.json)

    plot_coverage(dims, cells, palette, args.seed)
