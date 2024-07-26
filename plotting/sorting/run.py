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
parser.add_argument('perf_metrics', nargs='+', help='Perf metric(s) to track', default=["power/energy-pkg/"])
args = parser.parse_args()

assert (Path(args.gcc_path) / "bin").exists(), "GCC path is not set properly."
assert (Path(args.oneapi_path) / "libtbb.so").exists(), "oneAPI path is not set properly."

assert Path.cwd().name != "plotting", "Run from root."
# assert os.geteuid() == 0, "Must be run with sudo."

env_vars = {"LD_LIBRARY_PATH": f"{args.gcc_path}/lib64/:{args.oneapi_path}"}

runtime_marker = "Total duration: "

core_counts = set()
core_count = 1
while core_count <= min(10, multiprocessing.cpu_count()):
    core_counts.add(core_count)
    core_count += 1

core_count = 1
while core_count <= multiprocessing.cpu_count():
    core_counts.add(core_count)
    core_count += min(4, core_count)

core_counts = sorted(core_counts)
#core_counts = core_counts[:15]

perf_metrics = " ".join([f"-e {metric}" for metric in args.perf_metrics])

results = []
energy_results = []
for sort_name, sort_mode in [("Sequential std::sort", ""), ("Parallel std::sort", "-DPARALLEL_STD_SORT")]:
    for item_count, size_mode in [(250_000, ""), (4_000_000, "-DLARGE_DATASET")]:
    #for item_count, size_mode in [(250_000, "")]:
        # Compile with flags.
        # `-D_GLIBCXX_PARALLEL -fopenmp` is necessary with libc versions (shipped with Ubuntu 20.04).
        compile_command = f"{args.gcc_path}/bin/g++ MemorySortBenchmark.cpp -O3 -o sort__{hostname} -L{args.oneapi_path} -std=c++20 -Wall -Wextra -pedantic -ltbb -D_GLIBCXX_PARALLEL -fopenmp {sort_mode} {size_mode}"
        subprocess.run(shlex.split(compile_command), check=True)

        for stop_run in range(5):
            time.sleep(5)
            stop_command = f"perf stat {perf_metrics} ./sort__{hostname} STOP"
            start = time.time()
            result = subprocess.run(shlex.split(stop_command), capture_output=True, text=True, check=True, env=env_vars)
            end = time.time()
            assert "Joules" in result.stderr, "No energy measurement found."
            for line in result.stderr.splitlines():
                if "Joules" in line:
                    line_split = [x.strip() for x in line.split(" Joules ")]
                    #print(f'"Data Generation","{line_split[1]}",{line_split[0]},{end-start}')
                    energy_results.append({"MEASUREMENT": "Data Generation", "DATASET_SIZE": item_count, "PERF_METRIC": line_split[1], "JOULES": line_split[0], "RUNTIME_S": end-start})
            time.sleep(5)
            idle_command = f"perf stat {perf_metrics} sleep 10"
            start2 = time.time()
            result = subprocess.run(shlex.split(idle_command), capture_output=True, text=True, check=True, env=env_vars)
            end2 = time.time()
            assert "Joules" in result.stderr, "No energy measurement found."
            for line in result.stderr.splitlines():
                if "Joules" in line:
                    line_split = [x.strip() for x in line.split(" Joules ")]
                    #print(f'"Idling","{line_split[1]}",{line_split[0]},{end2-start2}')
                    energy_results.append({"MEASUREMENT": "Idling", "PERF_METRIC": line_split[1], "JOULES": line_split[0], "RUNTIME_S": end2-start2})

        for core_count in core_counts:
            time.sleep(5)
            cumu_runtime = 0.0
            for run in range(5):
                command = f"perf stat {perf_metrics} taskset -c 0-{core_count-1} ./sort__{hostname}"
                start = time.time()
                result = subprocess.run(shlex.split(command), capture_output=True, text=True, check=True, env=env_vars)
                end = time.time()
                assert runtime_marker in result.stdout, "No result found."
                assert "Joules" in result.stderr, "No energy measurement found."
                for line in result.stdout.splitlines():
                    if line.startswith(runtime_marker):
                        #print(line)
                        assert line.endswith(" s"), "Unexpected."
                        cumu_runtime += float(line[len(runtime_marker):-2])
                for line in result.stderr.splitlines():
                    if "Joules" in line:
                        line_split = [x.strip() for x in line.split(" Joules ")]
                        #print(f'"Sorting","{line_split[1]}",{line_split[0]},{end-start}')
                        energy_results.append({"MEASUREMENT": "Sorting", "CORE_COUNT": core_count, "SORT_VARIANT": sort_name, "DATASET_SIZE": item_count, "PERF_METRIC": line_split[1], "JOULES": line_split[0], "RUNTIME_S": end-start})

            print(f"{sort_name} and {item_count} ({core_count} cores) >> average runtime: {cumu_runtime / 5} s", flush=True)
            results.append({"CORE_COUNT": core_count, "SORT_VARIANT": sort_name, "DATASET_SIZE": item_count, "AVG_RUNTIME_S": cumu_runtime / 5})

            if "parallel" not in sort_name.lower():
                break

df = pd.DataFrame(results)
df_energy = pd.DataFrame(energy_results)

df.to_csv(f"plotting/sorting/results__{hostname}.csv", index=False)
df_energy.to_csv(f"plotting/sorting/results_energy__{hostname}.csv", index=False)

