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

hostname = socket.gethostname()

df = pd.read_csv("results.csv")
df_energy = pd.read_csv("results_energy.csv")
timestamp = int(time.time())

df_energy_medians = df_energy.groupby(["MEASUREMENT", "DATASET_SIZE", "PERF_METRIC", "CORE_COUNT", "SORT_VARIANT"], dropna=False).median().reset_index()

idling_rows = df_energy_medians.query("MEASUREMENT == 'Idling'")
assert len(idling_rows) == 1, "More lines than expected."
idling_joules_per_s = idling_rows.iloc[0]["JOULES"] / idling_rows.iloc[0]["RUNTIME_S"]
print(f"Idling for 1 s consumes {idling_joules_per_s} Joules.", )

data_generation_rows = df_energy_medians.query("MEASUREMENT == 'Data Generation'").sort_values(by="DATASET_SIZE")
assert len(data_generation_rows) == 2, "More lines than expected."
data_generation_joules_small = data_generation_rows.iloc[0]["JOULES"]
data_generation_joules_large = data_generation_rows.iloc[1]["JOULES"]
print(f"Data generation for small dataset takes {data_generation_joules_small} Joules.")
print(f"Data generation for large dataset takes {data_generation_joules_large} Joules.")

df_data_generation_joules = pd.DataFrame([{"DATASET_SIZE": 250_000, "DG_JOULES": data_generation_joules_small},
                                          {"DATASET_SIZE": 4_000_000, "DG_JOULES": data_generation_joules_large}])

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
    fig.savefig(f"sorting__{hostname}__{name}__{timestamp}.pdf")

    plt.clf()


df_joined = pd.merge(df, df_energy_medians, on=["DATASET_SIZE", "CORE_COUNT", "SORT_VARIANT"], how="left")
df_joined = pd.merge(df_joined, df_data_generation_joules, on="DATASET_SIZE")
df_joined["JOULES_WITHOUT_DG"] = df_joined.JOULES - df_joined.DG_JOULES
df_joined["TUPLES_PER_JOULE"] = df_joined.DATASET_SIZE / df_joined.JOULES_WITHOUT_DG
df_joined = df_joined.query("SORT_VARIANT != '(linearly scaling parallel std::sort)'")

facet_plot = sns.relplot(kind='line', data=df_joined, col='DATASET_SIZE_NAME', x="CORE_COUNT", y="TUPLES_PER_JOULE",
                         style="SORT_VARIANT", hue="SORT_VARIANT", markers=True,
                         facet_kws = {'sharey': False, 'sharex': True})
facet_plot.set_axis_labels("Cores", "Tuples Sorted per Joule")
facet_plot.set_titles("{col_name}")
facet_plot._legend.set_title("Sort Variant")
fig = facet_plot.fig
fig.savefig(f"sorting_energy__{hostname}__{timestamp}.pdf") 

plt.clf()
