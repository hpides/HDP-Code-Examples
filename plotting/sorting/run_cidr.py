#!/usr/bin/env python

import argparse
import multiprocessing
import os
import pandas as pd
import shlex
import socket
import subprocess
import time

from pathlib import Path

hostname = socket.gethostname()

parser = argparse.ArgumentParser()
parser.add_argument('--gcc_path', required=True)
parser.add_argument('--oneapi_path', required=True)
args = parser.parse_args()

assert (Path(args.gcc_path) / "bin").exists(), "GCC path is not set properly."
assert (Path(args.oneapi_path) / "libtbb.so").exists(), "oneAPI path is not set properly."

assert Path.cwd().name != "plotting", "Run from root."
# assert os.geteuid() == 0, "Must be run with sudo."

env_vars = {"LD_LIBRARY_PATH": f"{args.gcc_path}/lib64/:{args.oneapi_path}"}
export_command = f"export LD_LIBRARY_PATH={args.gcc_path}/lib64/:{args.oneapi_path};"

runtime_marker = "Total duration: "

perf_metrics = " ".join([f"-e {metric}" for metric in args.perf_metrics])

item_count = 4_000_000_000

stop_runs = 5
stop_joules = []
stop_runtimes = []

# Compile with flags.
# `-D_GLIBCXX_PARALLEL -fopenmp` is necessary with libc versions (shipped with Ubuntu 20.04).
compile_command = f"{args.gcc_path}/bin/g++ MemorySortBenchmark.cpp -O3 -o sort__{hostname} -L{args.oneapi_path} -std=c++20 -Wall -Wextra -pedantic -ltbb -D_GLIBCXX_PARALLEL -fopenmp -DPARALLEL_STD_SORT -DLARGE_DATASET"
subprocess.run(shlex.split(compile_command), check=True)

print(" == Data Generation")
for stop_run in range(stop_runs):
    time.sleep(5)
    stop_command = f"sudo turbostat --quiet --Joules --show Pkg_J sh -c '{export_command} numactl -N 0 -m 0 ./sort__{hostname} STOP'"
    start = time.time()
    result = subprocess.run(shlex.split(stop_command), capture_output=True, text=True, check=True, env=env_vars)
    end = time.time()
    assert "Pkg_J" in result.stderr, "No energy measurement found."
    lines_split_raw = result.stderr.splitlines()
    found = False
    lines_split = []
    for line_split in lines_split_raw:
        if line_split.endswith(" sec"):
            found = True

        if found:
            lines_split.append(line_split)
        
    assert "sec" in lines_split[0]
    stop_runtimes.append(float(lines_split[0].split()[0]))
    print(f"Joule results per package ({len(lines_split[3:])} results): {' '.join(lines_split[3:])}")
    joules_result = float(lines_split[3])
    print(f"Data generation: {joules_result} Joules")
    stop_joules.append(joules_result)

    time.sleep(5)

avg_stop_joules = sum(stop_joules) / stop_runs
avg_stop_runtime = sum(stop_runtimes) / stop_runs
print(f"Avg. Joules data generation: {avg_stop_joules}")
print(f"Avg. runtime data generation: {avg_stop_runtime} s")


sorting_runs = 5
sorting_joules = []
sorting_runtimes = []

print(" == Sorting")
time.sleep(5)
cumu_runtime = 0.0
for run in range(sorting_runs):
    command = f"sudo turbostat --quiet --Joules --show Pkg_J sh -c '{export_command} numactl -m 0 -N 0 ./sort__{hostname}'"
    start = time.time()
    result = subprocess.run(shlex.split(command), capture_output=True, text=True, check=True)
    end = time.time()
    assert runtime_marker in result.stdout, "No result found."
    assert "Pkg_J" in result.stderr, "No energy measurement found."
    lines_split_raw = result.stderr.splitlines()
    found = False
    lines_split = []
    for line_split in lines_split_raw:
        if line_split.endswith(" sec"):
            found = True

        if found:
            lines_split.append(line_split)
        
    assert "sec" in lines_split[0]
    sorting_runtimes.append(float(lines_split[0].split()[0]))
    print(f"Joule results per package ({len(lines_split[3:])} results): {' '.join(lines_split[3:])}")
    joules_result = float(lines_split[3])
    print(f"Data generation: {joules_result} Joules")
    sorting_joules.append(joules_result)

    time.sleep(5)

avg_sorting_joules = sum(sorting_joules) / sorting_runs
avg_sorting_runtime = sum(sorting_runtimes) / sorting_runs
print(f"Avg. Joules sorting: {avg_sorting_joules}")
print(f"Avg. runtime sorting: {avg_sorting_runtime} s")
