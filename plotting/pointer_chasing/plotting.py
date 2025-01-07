import numpy as np
import pandas as pd
import os
import seaborn as sns
import matplotlib.pyplot as plt

dataframes = []
for filename in os.listdir("."):
    if filename.endswith(".csv"):
        df = pd.read_csv(filename)
        dataframes.append(df)

df = pd.concat(dataframes, ignore_index=True)

df["RUNTIME_NS_PER_ELEMENT"] = df.RUNTIME_NS / 100_000_000
df["SIZE_LABEL"] = np.where(df["SIZE_IN_BYTES"] < 1_000_000,
                            (df["SIZE_IN_BYTES"] / 1024).map(int).map(str) + " KiB",
                            np.where(df["SIZE_IN_BYTES"] < 1_000_000_000,
                                     (df["SIZE_IN_BYTES"] / (1024 * 1024)).map(int).map(str) + " MiB",
                                     (df["SIZE_IN_BYTES"] / (1024 * 1024 * 1024)).map(int).map(str) + " GiB"))

replacements = {"nemea": "Intel Xeon Platinum 8180",
                # "Peteretina.localdomain": "Apple M2 Max",
                "Peteretina.local": "Apple M2 Max",
                "cp01": "IBM Power9",
                "cp02": "IBM Power8",
                "cp03": "IBM Power10",
                "nx03": "Intel Xeon Gold 6240L (Cascade Lake, nx03)",
                "nx04": "Intel Xeon Gold 6240L (Cascade Lake, nx04)",
                "nx05": "Intel Xeon Platinum 8352Y (Ice Lake, nx05)",
                "nx06": "Intel Xeon Platinum 8352Y (Ice Lake, nx06)",
               }

for id in range(1, 17):
    replacements[f"cx{id:02}"] = f"Intel Xeon Gold 5220S (Cascade Lake, cx{id:02})"

for id in range(17, 33):
    replacements[f"cx{id:02}"] = f"AMD EPYC 7742 (cx{id:02})"

for id in range(1, 5):
    replacements[f"gp{id:02}"] = f"IBM Power9 (gp{id:02})"

df = df.replace(replacements, regex=True)

sns.set_style("whitegrid")

# Matches need to be distinct.
machine_grouping_keywords = ["Apple", "5220S", "IBM", "8352Y", "AMD", "6240L"]

for manufacturer_keyword in machine_grouping_keywords:
    df_copy = df.query("MACHINE.str.contains(@manufacturer_keyword)").copy()
    if len(df_copy) == 0:
        continue

    plot = sns.lineplot(df_copy, x="SIZE_LABEL", y="RUNTIME_NS_PER_ELEMENT", style="MACHINE", hue="MACHINE")

    handles, labels = plot.get_legend_handles_labels()
    # sort both labels and handles by labels
    labels, handles = zip(*sorted(zip(labels, handles), key=lambda t: t[0]))
    plot.legend(handles, labels)

    sns.move_legend(plot, "upper left", bbox_to_anchor=(1, 1))

    labels = plot.get_xticklabels() # get x labels
    for ind, label in enumerate(labels):
        if ind % 6 == 0:  # every 10th label is kept
            label.set_visible(True)
        else:
            label.set_visible(False)

    plot.set(xlabel="Data Size", ylabel="Runtime per element (ns)")

    plt.savefig(f"result_{manufacturer_keyword.replace(' ', '_')}.pdf", bbox_inches='tight' )
    plt.clf()

for manufacturer_keyword in machine_grouping_keywords:
    df_copy = df.query("MACHINE.str.contains(@manufacturer_keyword)").copy()
    if len(df_copy) == 0:
        continue

    plot = sns.lineplot(df_copy, x="SIZE_LABEL", y="RUNTIME_NS_PER_ELEMENT", style="MACHINE", hue="MACHINE")

    handles, labels = plot.get_legend_handles_labels()
    # sort both labels and handles by labels
    labels, handles = zip(*sorted(zip(labels, handles), key=lambda t: t[0]))
    plot.legend(handles, labels)

    sns.move_legend(plot, "upper left", bbox_to_anchor=(1, 1))

    labels = plot.get_xticklabels() # get x labels
    for ind, label in enumerate(labels):
        if ind % 6 == 0:  # every 10th label is kept
            label.set_visible(True)
        else:
            label.set_visible(False)

    plot.set(xlabel="Data Size", ylabel="Runtime per element (ns)")

    plt.savefig(f"result_{manufacturer_keyword.replace(' ', '_')}.pdf", bbox_inches='tight' )
    plt.clf()

    print(manufacturer_keyword)
    print(str(df_copy.groupby(["MACHINE", "ELEMENT_COUNT", "SIZE_IN_BYTES"])["RUNTIME_NS_PER_ELEMENT"].mean().reset_index().query("SIZE_IN_BYTES > 16_000_000_000").sort_values(by="RUNTIME_NS_PER_ELEMENT")))
