#!/usr/bin/env python

import matplotlib.pyplot as plt
import multiprocessing
import os
import pandas as pd
import seaborn as sns
import shlex
import socket
import subprocess
import time

from pathlib import Path

assert Path.cwd().name != "plotting", "Run from root."
assert os.geteuid() == 0, "Must be run with sudo."

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

results = []
energy_results = []
for sort_name, sort_mode in [("Sequential std::sort", ""), ("Parallel std::sort", "-DPARALLEL_STD_SORT")]:
    # for item_count, size_mode in [(250_000, ""), (4_000_000, "-DLARGE_DATASET")]:
    for item_count, size_mode in [(250_000, "")]:
        # Compile with flags.
        compile_command = f"g++ MemorySortBenchmark.cpp -O3 -o sort -std=c++20 -Wall -Wextra -pedantic -ltbb {sort_mode} {size_mode}"
        subprocess.run(shlex.split(compile_command), check=True)

        for stop_run in range(5):
            time.sleep(5)
            stop_command = "perf stat -e power/energy-pkg/ ./sort STOP"
            start = time.time()
            result = subprocess.run(shlex.split(stop_command), capture_output=True, text=True, check=True)
            end = time.time()
            assert "Joules" in result.stderr, "No energy measurement found."
            for line in result.stderr.splitlines():
                if "Joules" in line:
                    line_split = [x.strip() for x in line.split(" Joules ")]
                    print(f'"Data Generation","{line_split[1]}",{line_split[0]},{end-start}')
            time.sleep(5)
            idle_command = "perf stat -e power/energy-pkg/ sleep 10"
            start2 = time.time()
            result = subprocess.run(shlex.split(idle_command), capture_output=True, text=True, check=True)
            end2 = time.time()
            assert "Joules" in result.stderr, "No energy measurement found."
            for line in result.stderr.splitlines():
                if "Joules" in line:
                    line_split = [x.strip() for x in line.split(" Joules ")]
                    print(f'"Idling","{line_split[1]}",{line_split[0]},{end2-start2}')

        for core_count in core_counts:
            time.sleep(5)
            cumu_runtime = 0.0
            for run in range(5):
                command = f"perf stat -e power/energy-pkg/ taskset -c 0-{core_count-1} ./sort"
                start = time.time()
                result = subprocess.run(shlex.split(command), capture_output=True, text=True, check=True)
                end = time.time()
                assert runtime_marker in result.stdout, "No result found."
                assert "Joules" in result.stderr, "No energy measurement found."
                for line in result.stdout.splitlines():
                    if line.startswith(runtime_marker):
                        print(line)
                        assert line.endswith(" s"), "Unexpected."
                        cumu_runtime += float(line[len(runtime_marker):-2])
                for line in result.stderr.splitlines():
                    if "Joules" in line:
                        line_split = [x.strip() for x in line.split(" Joules ")]
                        print(f'"Sorting","{line_split[1]}",{line_split[0]},{end-start}')

            print(f"{sort_name} and {item_count} ({core_count} cores) >> average runtime: {cumu_runtime / 5} s", flush=True)
            results.append({"CORE_COUNT": core_count, "SORT_VARIANT": sort_name, "DATASET_SIZE": item_count, "AVG_RUNTIME_S": cumu_runtime / 5})

            if "parallel" not in sort_name.lower():
                break

timestamp = int(time.time())
df = pd.DataFrame(results)

stretched_sequentials = []
# Simpler than fiddling with horizontals in mpl/seaborn.
for row in df.query("SORT_VARIANT == 'Sequential std::sort'").itertuples():
    for core_count in pd.unique(df.CORE_COUNT):
         if core_count == 1:
             continue
         stretched_sequentials.append({"CORE_COUNT": core_count, "SORT_VARIANT": row.SORT_VARIANT,
                                       "DATASET_SIZE": row.DATASET_SIZE, "AVG_RUNTIME_S": row.AVG_RUNTIME_S})

extrapolations = []
for row in df.query("SORT_VARIANT == 'Parallel std::sort' and CORE_COUNT == 1").itertuples():
    for core_count in pd.unique(df.CORE_COUNT):
        extrapolations.append({"CORE_COUNT": core_count, "SORT_VARIANT": "(linearly scaling parallel std::sort)",
                               "DATASET_SIZE": row.DATASET_SIZE, "AVG_RUNTIME_S": row.AVG_RUNTIME_S / core_count})

df = pd.concat([df, pd.DataFrame(stretched_sequentials)])
df = pd.concat([df, pd.DataFrame(extrapolations)])

df["SORTED_TUPLES_PER_S"] = df.DATASET_SIZE / df.AVG_RUNTIME_S
df["MILLION_SORTED_TUPLES_PER_S"] = df.SORTED_TUPLES_PER_S / 1_000_000
df["DATASET_SIZE_NAME"] = (df.DATASET_SIZE / 1_000_000).astype(str) + " Million Items (" + (df.DATASET_SIZE / 250_000).astype(str) + " GB)"

for size in pd.unique(df.DATASET_SIZE):
    df_plot = df.query("DATASET_SIZE == @size")
    plot = sns.lineplot(data=df_plot, x="CORE_COUNT", y="MILLION_SORTED_TUPLES_PER_S", style="SORT_VARIANT", hue="SORT_VARIANT", markers=True)
    fig = plot.get_figure()
    fig.savefig(f"out__{size}.pdf")

    plt.clf()

for name, core_limit in [("4cores", 4), ("64cores", 64), ("128cores", 128), ("unlimited", 2**32)]:
    df_filtered = df.query("CORE_COUNT <= @core_limit")
    facet_plot = sns.relplot(kind='line', data=df_filtered, col='DATASET_SIZE_NAME', x="CORE_COUNT", y="MILLION_SORTED_TUPLES_PER_S",
                             style="SORT_VARIANT", hue="SORT_VARIANT", markers=True,
                             facet_kws = {'sharey': False, 'sharex': True})
    facet_plot.set_axis_labels("Cores", "Million Tuples sorted per Second")
    facet_plot.set_titles('{col_name}')
    facet_plot._legend.set_title("Sort Variant")
    fig = facet_plot.fig
    fig.savefig(f"sorting__{socket.gethostname()}__{name}__{timestamp}.pdf") 

    plt.clf()
