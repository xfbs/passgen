#!/usr/bin/env python3
"""Compares two runs of the benchmarks.

Each run is what `passgen-bench --tsv` prints, such as from before and after a
change:

    passgen-bench --tsv > before.tsv
    # change something, rebuild
    passgen-bench --tsv > after.tsv
    scripts/bench-compare.py before.tsv after.tsv

For every benchmark in both, it shows how the time of an iteration changed.
A change is only called faster or slower when the middle halves of the
samples of the two runs don't overlap, and it is larger than the threshold
(5% unless it is given with `--threshold PERCENT`); otherwise it is noise.
Runs of the same code differ by a few percent, more so when something else
is running, so a change is worth confirming with another pair of runs.
"""

import argparse
import csv
import sys


def read(path):
    with open(path, newline="") as file:
        return {row["name"]: row for row in csv.DictReader(file, delimiter="\t")}


def time(seconds):
    for unit, factor in (("s", 1), ("ms", 1e-3), ("us", 1e-6)):
        if seconds >= factor:
            return f"{seconds / factor:7.2f} {unit}"
    return f"{seconds / 1e-9:7.2f} ns"


def main():
    parser = argparse.ArgumentParser(description="Compares two runs of the benchmarks.")
    parser.add_argument("before", help="what passgen-bench --tsv printed before")
    parser.add_argument("after", help="what passgen-bench --tsv printed after")
    parser.add_argument(
        "--threshold",
        type=float,
        default=5,
        metavar="PERCENT",
        help="the smallest change which is not noise (default: 5)",
    )
    args = parser.parse_args()

    before = read(args.before)
    after = read(args.after)
    threshold = args.threshold / 100

    print(f"{'benchmark':<24} {'before':>10} {'after':>10} {'change':>8}")
    for name, new in after.items():
        old = before.get(name)
        if old is None:
            print(f"{name:<24} {'':>10} {time(float(new['median'])):>10}      new")
            continue

        old_median = float(old["median"])
        new_median = float(new["median"])
        change = (new_median - old_median) / old_median

        if abs(change) < threshold:
            verdict = "noise"
        elif float(new["q3"]) < float(old["q1"]):
            verdict = "faster"
        elif float(new["q1"]) > float(old["q3"]):
            verdict = "slower"
        else:
            verdict = "noise"

        print(
            f"{name:<24} {time(old_median):>10} {time(new_median):>10} "
            f"{100 * change:+7.1f}% {verdict}"
        )

    for name in before:
        if name not in after:
            print(f"{name:<24} {time(float(before[name]['median'])):>10} {'':>10}     gone")
    return 0


if __name__ == "__main__":
    sys.exit(main())
