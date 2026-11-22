#!/usr/bin/env python3
import argparse
import csv
import json
import urllib.request
from pathlib import Path

import matplotlib.pyplot as plt

RESULTS_DIR = Path(__file__).parent / "results"


def build_index(url, threads):
    body = json.dumps({"threads": threads}).encode()
    request = urllib.request.Request(url + "/api/index", data=body)
    with urllib.request.urlopen(request, timeout=3600) as response:
        return json.load(response)["time_ms"]


def measure(url, thread_counts, repeats):
    build_index(url, max(thread_counts))
    times = {}
    for threads in thread_counts:
        times[threads] = min(build_index(url, threads) for _ in range(repeats))
        print(f"threads {threads}: {times[threads]} ms", flush=True)
    return times


def save_csv(rows, path):
    with open(path, "w", newline="") as file:
        writer = csv.writer(file)
        writer.writerow(["threads", "time_ms", "speedup", "efficiency"])
        for threads, time_ms, speedup in rows:
            writer.writerow(
                [threads, time_ms, f"{speedup:.3f}", f"{speedup / threads:.3f}"]
            )


def save_plot(rows, path):
    threads = [row[0] for row in rows]
    fig, (left, right) = plt.subplots(1, 2, figsize=(11, 4))
    left.plot(threads, [row[1] / 1000 for row in rows], marker="o")
    left.set_xlabel("threads")
    left.set_ylabel("build time, s")
    right.plot(threads, [row[2] for row in rows], marker="o", label="measured")
    right.plot(threads, threads, linestyle="--", label="ideal")
    right.set_xlabel("threads")
    right.set_ylabel("speedup")
    right.legend()
    for axis in (left, right):
        axis.grid(True)
    fig.savefig(path, dpi=150, bbox_inches="tight")


def main():
    parser = argparse.ArgumentParser(description="Benchmark index build time.")
    parser.add_argument("--url", default="http://localhost:8080")
    parser.add_argument("--threads", type=int, nargs="+", default=[1, 2, 4, 8, 16])
    parser.add_argument("--repeats", type=int, default=3)
    args = parser.parse_args()

    times = measure(args.url, args.threads, args.repeats)
    base = times[args.threads[0]]
    rows = [(t, times[t], base / times[t]) for t in args.threads]

    RESULTS_DIR.mkdir(exist_ok=True)
    save_csv(rows, RESULTS_DIR / "index_time.csv")
    save_plot(rows, RESULTS_DIR / "index_time.png")


if __name__ == "__main__":
    main()
