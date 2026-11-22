#!/usr/bin/env python3
import argparse
import csv
import json
import random
import sys
import threading
import time
import urllib.request
from pathlib import Path

import matplotlib.pyplot as plt

RESULTS_DIR = Path(__file__).parent / "results"
QUERIES = [
    "moon landing",
    "football",
    "river",
    "photosynthesis",
    "jazz musician",
    "railway station",
    "election",
    "volcano",
    "medieval castle",
    "quantum mechanics",
    "olympic games",
    "symphony",
    "bacteria",
    "film director",
    "glacier",
]


def client(url, deadline, counters, lock):
    done = errors = 0
    waited = 0.0
    while time.perf_counter() < deadline:
        body = json.dumps(
            {"query": {"text": random.choice(QUERIES), "mode": "keyword"}}
        ).encode()
        started = time.perf_counter()
        try:
            request = urllib.request.Request(url + "/api/search", data=body)
            with urllib.request.urlopen(request, timeout=30) as response:
                response.read()
            done += 1
            waited += time.perf_counter() - started
        except OSError:
            errors += 1
    with lock:
        counters["done"] += done
        counters["errors"] += errors
        counters["waited"] += waited


def measure(url, clients, duration):
    counters = {"done": 0, "errors": 0, "waited": 0.0}
    lock = threading.Lock()
    deadline = time.perf_counter() + duration
    threads = [
        threading.Thread(target=client, args=(url, deadline, counters, lock))
        for _ in range(clients)
    ]
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    throughput = counters["done"] / duration
    latency = counters["waited"] / max(counters["done"], 1) * 1000
    return throughput, latency, counters["errors"]


def documents(url):
    with urllib.request.urlopen(url + "/api/stats", timeout=30) as response:
        return json.load(response)["documents"]


def save_csv(rows, path):
    with open(path, "w", newline="") as file:
        writer = csv.writer(file)
        writer.writerow(["clients", "throughput_rps", "avg_latency_ms", "errors"])
        for clients, throughput, latency, errors in rows:
            writer.writerow([clients, f"{throughput:.1f}", f"{latency:.1f}", errors])


def save_plot(rows, path):
    clients = [row[0] for row in rows]
    fig, (left, right) = plt.subplots(1, 2, figsize=(11, 4))
    left.plot(clients, [row[1] for row in rows], marker="o")
    left.set_xlabel("clients")
    left.set_ylabel("throughput, requests/s")
    right.plot(clients, [row[2] for row in rows], marker="o")
    right.set_xlabel("clients")
    right.set_ylabel("average latency, ms")
    for axis in (left, right):
        axis.grid(True)
    fig.savefig(path, dpi=150, bbox_inches="tight")


def main():
    parser = argparse.ArgumentParser(description="Benchmark search throughput.")
    parser.add_argument("--url", default="http://localhost:8080")
    parser.add_argument(
        "--clients", type=int, nargs="+", default=[1, 2, 4, 8, 16, 32, 64]
    )
    parser.add_argument("--duration", type=float, default=10)
    args = parser.parse_args()

    if documents(args.url) == 0:
        sys.exit("index is empty: build it first with POST /api/index")

    rows = []
    for clients in args.clients:
        throughput, latency, errors = measure(args.url, clients, args.duration)
        rows.append((clients, throughput, latency, errors))
        print(
            f"clients {clients}: {throughput:.1f} rps, {latency:.1f} ms, "
            f"{errors} errors",
            flush=True,
        )

    RESULTS_DIR.mkdir(exist_ok=True)
    save_csv(rows, RESULTS_DIR / "search_throughput.csv")
    save_plot(rows, RESULTS_DIR / "search_throughput.png")


if __name__ == "__main__":
    main()
