#!/usr/bin/env python3
"""Plot a frelsim time-series CSV log.

frelsim's Overseer/Recorder (see frelsim/recorder/Recorder.hpp) writes a
CSV with a `time` column followed by one column per identifier named in a
System's `logged_outputs` - see docs/guide/running-simulations.md for how
to configure logging in a config. This script just plots every column
against time on one figure, since that's almost always what you want first
when checking a simulation's results are actually correct.

Usage:
    python3 scripts/plot_results.py path/to/log.csv
    python3 scripts/plot_results.py path/to/log.csv --save plot.png
"""
import argparse
import csv
import sys

import matplotlib.pyplot as plt


def load_csv(path):
    with open(path, newline="") as f:
        reader = csv.reader(f)
        try:
            header = next(reader)
        except StopIteration:
            raise ValueError(f"'{path}' is empty - no header row")

        columns = {name: [] for name in header}
        for row_number, row in enumerate(reader, start=2):
            if len(row) != len(header):
                raise ValueError(
                    f"'{path}' line {row_number}: expected {len(header)} columns, got {len(row)}")
            for name, value in zip(header, row):
                columns[name].append(float(value))

    return header, columns


def main():
    parser = argparse.ArgumentParser(description="Plot a frelsim Recorder CSV log.")
    parser.add_argument("csv_path", help="Path to a CSV file written by Overseer's Recorder")
    parser.add_argument("--save", metavar="PATH", help="Save the plot to PATH instead of showing it interactively")
    args = parser.parse_args()

    try:
        header, columns = load_csv(args.csv_path)
    except (OSError, ValueError) as e:
        print(f"plot_results.py: {e}", file=sys.stderr)
        sys.exit(1)

    if len(header) < 2:
        print(f"plot_results.py: '{args.csv_path}' has no logged columns besides time - nothing to plot",
              file=sys.stderr)
        sys.exit(1)

    time_column = header[0]
    times = columns[time_column]

    fig, ax = plt.subplots()
    for name in header[1:]:
        ax.plot(times, columns[name], label=name, marker=".")

    ax.set_xlabel(time_column)
    ax.set_ylabel("value")
    ax.set_title(args.csv_path)
    ax.legend()
    ax.grid(True)

    if args.save:
        fig.savefig(args.save)
        print(f"Saved plot to {args.save}")
    else:
        plt.show()


if __name__ == "__main__":
    main()
