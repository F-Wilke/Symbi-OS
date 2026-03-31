#!/usr/bin/env python3
"""
Base version was generated with:
        ANHTROPIC Claude 4.6 Sonnet

plot_echo.py — latency distribution plotter for echoclient
Supports 1 or 2 runs on the same plot for side-by-side comparison.

Only non-stdlib dependency: matplotlib

Usage
─────
  # single run
  ./echoclient -s 256 -r 5000 -d 30 -v > run_a.txt
  python3 plot_echo.py -f run_a.txt -s 256 -r 5000

  # two-run comparison
  ./echoclient -s 256 -r 5000  -d 30 -v > baseline.txt
  ./echoclient -s 256 -r 10000 -d 30 -v > highload.txt
  python3 plot_echo.py -f baseline.txt highload.txt \\
                       -l "5k TPS" "10k TPS"        \\
                       -s 256 -o compare.pdf

  # plain µs values, one per line, are also accepted
"""

import sys
import re
import math
import argparse
import statistics
from pathlib import Path

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import matplotlib.ticker as ticker
    from matplotlib.backends.backend_pdf import PdfPages
except ImportError:
    sys.exit("matplotlib is required:  pip install matplotlib")


# ── colour palettes ───────────────────────────────────────────────────────────
#   each run gets its own palette; lines for mean/p99 are distinct within
#   a palette so the two runs remain legible even when heavily overlapping
PALETTES = [
    {   # run A — blue / red / amber
        "hist":  "#4878CF",
        "mean":  "#E84545",
        "p99":   "#F5A623",
        "alpha": 0.55,
    },
    {   # run B — green / purple / teal
        "hist":  "#53A548",
        "mean":  "#9B59B6",
        "p99":   "#1ABC9C",
        "alpha": 0.55,
    },
]


# ── parsing ───────────────────────────────────────────────────────────────────

_RTT_RE = re.compile(r"rtt\s+([\d.]+)\s*[µu]s", re.IGNORECASE)

def load_samples(source: str) -> list:
    """Return list[float] of RTT µs values from a file path or '-' for stdin."""
    if source == "-":
        text = sys.stdin.read()
    else:
        text = Path(source).read_text(encoding="utf-8", errors="replace")

    samples = []
    for line in text.splitlines():
        m = _RTT_RE.search(line)
        if m:
            samples.append(float(m.group(1)))
            continue
        s = line.strip()
        if s and not s.startswith("#"):
            try:
                samples.append(float(s))
            except ValueError:
                pass
    return samples


# ── statistics (pure stdlib) ─────────────────────────────────────────────────

def pct(sorted_data: list, p: float) -> float:
    """Linear-interpolation percentile on a pre-sorted list."""
    n = len(sorted_data)
    if n == 1:
        return sorted_data[0]
    idx  = p / 100.0 * (n - 1)
    lo   = int(idx)
    frac = idx - lo
    if lo + 1 >= n:
        return sorted_data[-1]
    return sorted_data[lo] * (1.0 - frac) + sorted_data[lo + 1] * frac

def compute_stats(samples: list) -> dict:
    srt = sorted(samples)
    return dict(
        srt  = srt,
        n    = len(srt),
        mean = statistics.mean(srt),
        med  = statistics.median(srt),
        p50  = pct(srt, 50),
        p75  = pct(srt, 75),
        p90  = pct(srt, 90),
        p99  = pct(srt, 99),
        p999 = pct(srt, 99.9),
        lo   = srt[0],
        hi   = srt[-1],
    )

def ecdf_points(data: list):
    xs = sorted(data)
    n  = len(xs)
    ys = [(i + 1) / n * 100.0 for i in range(n)]
    return xs, ys


# ── axis / line helpers ───────────────────────────────────────────────────────

def _comma(v, _pos=None):
    return f"{int(v):,}"

def _vline(ax, x, color, ls, lw=1.6, label=None):
    ax.axvline(x, color=color, linewidth=lw, linestyle=ls,
               label=label, zorder=3)

def _hline(ax, y, color, ls, lw=1.6, label=None):
    ax.axhline(y, color=color, linewidth=lw, linestyle=ls,
               label=label, zorder=3)


# ── per-run panel helpers ─────────────────────────────────────────────────────

def draw_histogram(ax, st: dict, pal: dict, label: str,
                   hist_xmax: float, density: bool):
    visible = [x for x in st["srt"] if x <= hist_xmax]
    n_bins  = min(200, max(50, int(math.sqrt(len(visible)))))
    ax.hist(visible, bins=n_bins, color=pal["hist"], alpha=pal["alpha"],
            edgecolor="none", label=label, density=density)
    _vline(ax, st["mean"], pal["mean"], "--",
           label=f'{label} mean {st["mean"]:.1f} µs')
    _vline(ax, st["p99"],  pal["p99"],  "-.",
           label=f'{label} p99  {st["p99"]:.1f} µs')


def draw_cdf(ax, samples: list, st: dict, pal: dict,
             label: str, ann_y_offset: float):
    xs, ys = ecdf_points(samples)
    n      = len(xs)
    step   = max(1, n // 50_000)
    ax.plot(xs[::step], ys[::step], color=pal["hist"], linewidth=1.4,
            label=label)
    _vline(ax, st["mean"], pal["mean"], "--",
           label=f'{label} mean {st["mean"]:.1f} µs')
    _vline(ax, st["p99"],  pal["p99"],  "-.",
           label=f'{label} p99  {st["p99"]:.1f} µs')

    # cross-hair guides at p50 / p99 / p99.9
    # ann_y_offset staggers annotations so two runs don't overlap
    for pv, pval in [(50, st["p50"]), (99, st["p99"]), (99.9, st["p999"])]:
        ax.plot([0, pval, pval], [pv, pv, 0],
                color=pal["hist"], linewidth=0.7, linestyle=":", zorder=2)
        ax.annotate(f"{label} p{pv}={pval:.1f}µs",
                    xy=(pval, pv),
                    xytext=(4, 3 + ann_y_offset),
                    textcoords="offset points",
                    fontsize=6.5, color=pal["hist"])


def draw_series(ax, samples: list, st: dict, pal: dict, label: str):
    n    = len(samples)
    step = max(1, n // 20_000)
    idx  = list(range(0, n, step))
    vals = [samples[i] for i in idx]
    ax.scatter(idx, vals, s=1.5, alpha=0.25, color=pal["hist"],
               rasterized=True, label=label)
    _hline(ax, st["mean"], pal["mean"], "--",
           label=f'{label} mean {st["mean"]:.1f} µs')
    _hline(ax, st["p99"],  pal["p99"],  "-.",
           label=f'{label} p99  {st["p99"]:.1f} µs')


# ── footer summary ────────────────────────────────────────────────────────────

def _stat_row(label: str, st: dict, pal: dict) -> str:
    return (
        f"{label}:  n={st['n']:,}  "
        f"min={st['lo']:.1f}  mean={st['mean']:.1f}  "
        f"p50={st['p50']:.1f}  p75={st['p75']:.1f}  "
        f"p90={st['p90']:.1f}  p99={st['p99']:.1f}  "
        f"p99.9={st['p999']:.1f}  max={st['hi']:.1f}  (µs)"
    )


# ── main plotting entry-point ─────────────────────────────────────────────────

def make_plot(runs: list,        # [(samples: list, label: str), ...]
              pkt_size: int,
              rate: int,
              out_path: str):

    n_runs     = len(runs)
    all_stats  = [compute_stats(s) for s, _ in runs]
    palettes   = PALETTES[:n_runs]

    # histogram x-axis clipped at max(p99.9) across all runs
    hist_xmax  = max(st["p999"] for st in all_stats) * 1.05
    # two runs: use density so different sample counts stay visually comparable
    use_density = n_runs > 1

    # ── figure ────────────────────────────────────────────────────────────── #
    fig, axes = plt.subplots(3, 1, figsize=(9, 12))

    title = f"TCP echo — packet size {pkt_size} B"
    if rate:
        title += f",  target {rate:,} TPS"
    if n_runs > 1:
        title += "  [comparison]"
    fig.suptitle(title, fontsize=13, fontweight="bold", y=0.99)

    # ── panel 1: histogram ────────────────────────────────────────────────── #
    ax = axes[0]
    for (samples, label), st, pal in zip(runs, all_stats, palettes):
        draw_histogram(ax, st, pal, label, hist_xmax, density=use_density)

    clipped = sum(
        sum(1 for x in s if x > hist_xmax) for s, _ in runs
    )
    note = (f"  ({clipped:,} samples beyond {hist_xmax:.0f} µs not shown)"
            if clipped else "")
    ax.set_title(f"Latency histogram{note}", fontsize=10)
    ax.set_xlabel("RTT (µs)", fontsize=9)
    ax.set_ylabel("Probability density" if use_density else "Count", fontsize=9)
    if not use_density:
        ax.yaxis.set_major_formatter(ticker.FuncFormatter(_comma))
    ax.legend(fontsize=8, loc="upper right")

    # ── panel 2: empirical CDF ────────────────────────────────────────────── #
    ax = axes[1]
    # stagger cross-hair annotations: run 0 above the line, run 1 below
    offsets = [8, -12] if n_runs > 1 else [4]
    for (samples, label), st, pal, off in zip(runs, all_stats, palettes, offsets):
        draw_cdf(ax, samples, st, pal, label, ann_y_offset=off)

    ax.set_xlim(left=0)
    ax.set_ylim(0, 101)
    ax.set_xlabel("RTT (µs)",        fontsize=9)
    ax.set_ylabel("Percentile (%)",  fontsize=9)
    ax.set_title("Empirical CDF",    fontsize=10)
    ax.legend(fontsize=8, loc="lower right")

    # ── panel 3: RTT time-series ──────────────────────────────────────────── #
    ax = axes[2]
    for (samples, label), st, pal in zip(runs, all_stats, palettes):
        draw_series(ax, samples, st, pal, label)

    ax.set_xlabel("Transaction index", fontsize=9)
    ax.set_ylabel("RTT (µs)",          fontsize=9)
    ax.set_title("RTT over time",      fontsize=10)
    ax.xaxis.set_major_formatter(ticker.FuncFormatter(_comma))
    ax.legend(fontsize=8, loc="upper right")

    # ── summary footer (one line per run) ─────────────────────────────────── #
    footer = "\n".join(
        _stat_row(lbl, st, pal)
        for (_, lbl), st, pal in zip(runs, all_stats, palettes)
    )
    # scale bottom margin to the number of footer lines
    bottom = 0.017 * n_runs
    fig.text(0.5, 0.002, footer,
             ha="center", va="bottom", fontsize=7,
             color="#555555", style="italic", multialignment="center")

    plt.tight_layout(rect=[0, bottom, 1, 0.975])

    with PdfPages(out_path) as pdf:
        pdf.savefig(fig, dpi=150)
        d = pdf.infodict()
        d["Title"]   = title
        d["Subject"] = "TCP echo server latency benchmark"

    plt.close(fig)
    print(f"written → {out_path}")
    for (_, lbl), st in zip(runs, all_stats):
        print(f"  [{lbl}]  n={st['n']:,}  "
              f"mean={st['mean']:.1f} µs  p99={st['p99']:.1f} µs  "
              f"p99.9={st['p999']:.1f} µs")


# ── CLI ───────────────────────────────────────────────────────────────────────

def main():
    ap = argparse.ArgumentParser(
        description="Plot echoclient latency as a PDF — 1 or 2 runs",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    ap.add_argument("-f", "--file",
                    nargs="+", required=True, metavar="FILE",
                    help="one or two input files  ('-' = stdin)")
    ap.add_argument("-l", "--label",
                    nargs="+", metavar="LABEL",
                    help="display labels matching each file  "
                         "(default: stem of filename)")
    ap.add_argument("-s", "--size",
                    type=int, required=True,
                    help="packet size in bytes (used in title)")
    ap.add_argument("-r", "--rate",
                    type=int, default=0,
                    help="target TPS (used in title, optional)")
    ap.add_argument("-o", "--output",
                    default="latency.pdf",
                    help="output PDF path  (default: latency.pdf)")
    args = ap.parse_args()

    if len(args.file) > 2:
        ap.error("at most two input files are supported")

    # build label list — pad / default to file stems
    labels = list(args.label or [])
    for f in args.file[len(labels):]:
        labels.append(Path(f).stem if f != "-" else "stdin")

    runs = []
    for fpath, label in zip(args.file, labels):
        samples = load_samples(fpath)
        if not samples:
            sys.exit(f"no RTT samples found in {fpath!r}  "
                     "(did you run echoclient with -v?)")
        src = "stdin" if fpath == "-" else fpath
        print(f"loaded {len(samples):,} samples from {src!r}  [{label}]")
        runs.append((samples, label))

    make_plot(runs, args.size, args.rate, args.output)


if __name__ == "__main__":
    main()
