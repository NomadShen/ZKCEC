#!/usr/bin/env python3
"""Parse ZKCEC logs, write result.csv, and create a performance plot."""

from __future__ import annotations

import csv
import os
import re
import sys
import tempfile
from pathlib import Path


RESULT_DIR = Path(__file__).resolve().parent
NON_OPT_DIR = RESULT_DIR / "non_opt"
OPT_DIR = RESULT_DIR / "opt"
RAW_OUTPUT_FILE = RESULT_DIR / "result_raw.csv"
OUTPUT_FILE = RESULT_DIR / "result.csv"
PLOT_FILE = RESULT_DIR / "performance_plot.pdf"

RAW_HEADER = [
    "Circuit",
    "Lits",
    "Cls",
    "R",
    "W",
    "t1",
    "t2",
    "t3",
    "t4",
    "ta",
    "com",
    "nt2",
    "nta",
    "ncom",
    "R'",
]

PROCESSED_HEADER = [
    "Circuit",
    "Lits",
    "Cls",
    "R",
    "W",
    "t1",
    "t2",
    "t3",
    "t4",
    "ta",
    "R'",
    "nta",
    "speedup",
]

NUMBER = r"[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?"


class AnalysisError(RuntimeError):
    """Raised when the result directories or logs are incomplete."""


def natural_sort_key(name: str) -> list[object]:
    """Sort embedded numbers numerically, e.g. x2 before x10."""
    return [int(part) if part.isdigit() else part.lower() for part in re.split(r"(\d+)", name)]


def find_logs(directory: Path) -> dict[str, Path]:
    if not directory.is_dir():
        raise AnalysisError(f"result directory not found: {directory}")

    logs = {path.stem: path for path in directory.glob("*.log") if path.is_file()}
    if not logs:
        raise AnalysisError(f"no .log files found in: {directory}")
    return logs


def extract_fields(log_path: Path, labels: list[str]) -> dict[str, str]:
    text = log_path.read_text(encoding="utf-8", errors="replace")
    values: dict[str, str] = {}
    missing: list[str] = []

    for label in labels:
        pattern = rf"^\s*{re.escape(label)}\s*:\s*({NUMBER})"
        matches = re.findall(pattern, text, flags=re.IGNORECASE | re.MULTILINE)
        if matches:
            # Use the last report if a log contains more than one completed run.
            values[label] = matches[-1]
        else:
            missing.append(label)

    if missing:
        raise AnalysisError(
            f"missing field(s) {', '.join(missing)} in {log_path}"
        )
    return values


def fixed_2(value: str) -> str:
    """Format a numeric log value with exactly two decimal places."""
    return f"{float(value):.2f}"


def rounded_t2(value: str) -> str:
    """Round t2 to two decimals while removing trailing zeroes."""
    return f"{round(float(value), 2):g}"


def build_rows() -> tuple[list[list[str]], list[list[str]]]:
    non_opt_logs = find_logs(NON_OPT_DIR)
    opt_logs = find_logs(OPT_DIR)

    missing_opt = sorted(set(non_opt_logs) - set(opt_logs), key=natural_sort_key)
    missing_non_opt = sorted(set(opt_logs) - set(non_opt_logs), key=natural_sort_key)
    if missing_opt or missing_non_opt:
        details: list[str] = []
        if missing_opt:
            details.append(f"missing opt logs: {', '.join(missing_opt)}")
        if missing_non_opt:
            details.append(f"missing non_opt logs: {', '.join(missing_non_opt)}")
        raise AnalysisError("; ".join(details))

    raw_rows: list[list[str]] = []
    processed_rows: list[list[str]] = []
    designs = sorted(non_opt_logs, key=natural_sort_key)
    for design_name in designs:
        non_opt = extract_fields(
            non_opt_logs[design_name],
            [
                "c1 lits",
                "c1 cls",
                "Length",
                "Width",
                "p1 cost",
                "p2 cost",
                "p3 cost",
                "p4 cost",
                "total cost",
                "communication",
            ],
        )
        opt = extract_fields(
            opt_logs[design_name],
            ["p2 cost", "total cost", "communication", "Length"],
        )

        opt_total_cost = float(opt["total cost"])
        if opt_total_cost == 0:
            raise AnalysisError(
                f"cannot calculate speedup: total cost is zero in "
                f"{opt_logs[design_name]}"
            )
        speedup = float(non_opt["total cost"]) / opt_total_cost

        circuit = design_name
        raw_rows.append(
            [
                circuit,
                non_opt["c1 lits"],
                non_opt["c1 cls"],
                non_opt["Length"],
                non_opt["Width"],
                non_opt["p1 cost"],
                non_opt["p2 cost"],
                non_opt["p3 cost"],
                non_opt["p4 cost"],
                non_opt["total cost"],
                non_opt["communication"],
                opt["p2 cost"],
                opt["total cost"],
                opt["communication"],
                opt["Length"],
            ]
        )
        processed_rows.append(
            [
                circuit,
                non_opt["c1 lits"],
                non_opt["c1 cls"],
                non_opt["Length"],
                non_opt["Width"],
                fixed_2(non_opt["p1 cost"]),
                rounded_t2(non_opt["p2 cost"]),
                fixed_2(str(float(non_opt["p3 cost"]) * 1e3)),
                fixed_2(str(float(non_opt["p4 cost"]) * 1e3)),
                fixed_2(non_opt["total cost"]),
                opt["Length"],
                fixed_2(opt["total cost"]),
                fixed_2(speedup),
            ]
        )
    return raw_rows, processed_rows


def write_csv(path: Path, header: list[str], rows: list[list[str]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def create_performance_plot(raw_csv_path: Path, plot_path: Path) -> None:
    cache_dir = Path(tempfile.gettempdir()) / "zkcec-matplotlib-cache"
    cache_dir.mkdir(parents=True, exist_ok=True)
    os.environ.setdefault("MPLCONFIGDIR", str(cache_dir))
    os.environ.setdefault("XDG_CACHE_HOME", str(cache_dir))

    try:
        import matplotlib

        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        import pandas as pd
    except ImportError as error:
        raise AnalysisError(
            "plotting requires the pandas and matplotlib Python packages"
        ) from error

    df = pd.read_csv(raw_csv_path)
    df["Complexity"] = df["R"] * df["W"]
    df["Complexity2"] = df["Cls"] * df["W"]
    df["Complexity3"] = df["R'"] * df["W"]
    df["speedup"] = df["ta"] / df["nta"]
    df = df.sort_values(by="Complexity")

    required_positive = [
        "Complexity",
        "Complexity2",
        "Complexity3",
        "R",
        "t2",
        "t3",
        "t4",
        "ta",
        "nta",
    ]
    if (df[required_positive] <= 0).any().any():
        raise AnalysisError("log-scale plot values must all be greater than zero")

    plots = [
        ("t2", "royalblue", "o"),
        ("t3", "firebrick", "^"),
        ("t4", "darkorange", "v"),
        ("ta", "purple", "D"),
        ("nta", "seagreen", "s"),
        ("speedup", "teal", "*"),
    ]
    x_columns = [
        "Complexity",
        "Complexity2",
        "Complexity2",
        "Complexity",
        "Complexity3",
        "R",
    ]
    x_labels = ["R x W", "Cls. x W", "Cls. x W", "R x W", "R' x W", "R"]
    y_labels = [
        "Time for P2 (s)",
        "Time for P3 (s)",
        "Time for P4 (s)",
        "Total Time (s)",
        "Optimized Total Time (s)",
        "Speedup (x)",
    ]

    fig, axes = plt.subplots(2, 3, figsize=(3.8 * 3, 2.5 * 2))
    axes = axes.flatten()
    try:
        for index, (column, color, marker) in enumerate(plots):
            axis = axes[index]
            axis.plot(
                df[x_columns[index]],
                df[column],
                color=color,
                marker=marker,
                linestyle="",
                linewidth=1.1,
                markersize=4,
            )
            axis.set_xlabel(x_labels[index], fontsize=10)
            axis.set_ylabel(y_labels[index], fontsize=10)
            axis.grid(True, linestyle="--", alpha=0.6)
            axis.set_xscale("log")
            if index < 5:
                axis.set_yscale("log")
            axis.text(
                0.5,
                -0.4,
                f"({chr(ord('a') + index)})",
                transform=axis.transAxes,
                fontsize=10,
                ha="center",
                va="center",
            )

        fig.subplots_adjust(hspace=0.5)
        fig.tight_layout()
        fig.savefig(plot_path, dpi=300, bbox_inches="tight")
    finally:
        plt.close(fig)


def main() -> int:
    try:
        raw_rows, processed_rows = build_rows()
        write_csv(RAW_OUTPUT_FILE, RAW_HEADER, raw_rows)
        write_csv(OUTPUT_FILE, PROCESSED_HEADER, processed_rows)
        create_performance_plot(RAW_OUTPUT_FILE, PLOT_FILE)
        RAW_OUTPUT_FILE.unlink()
    except (AnalysisError, OSError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1

    print(f"Wrote {len(processed_rows)} processed row(s) to {OUTPUT_FILE}")
    print(f"Saved performance plot to {PLOT_FILE}")
    print(f"Removed temporary raw results: {RAW_OUTPUT_FILE}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
