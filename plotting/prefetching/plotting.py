import numpy as np
import pandas as pd
import os
import seaborn as sns
import matplotlib.pyplot as plt

dataframes = []
for filename in os.listdir("."):
    if filename.endswith(".csv.xz"):
        df = pd.read_csv(filename)
        dataframes.append(df)

df = pd.concat(dataframes, ignore_index=True)

df["RUNTIME_MS"] = df.RUNTIME_S * 1000
df["KEY"] = df.NAME + " (Locality: " + df.LOCALITY.map(str) + ")"
df["VECTOR_SIZE_STR"] = (df.VECTOR_SIZE / 1000 / 1000 / 1000).map(str) + " GB"

df = df.replace({"nemea": "Intel Xeon Platinum 8180",
                 "Peteretina.local": "Apple M2 Max",
                 "nx05": "Intel Xeon Platinum 8352Y",
                 "cx16": "Intel Xeon Gold 5220S",
                 "cp03": "IBM Power10",
                 "cx18": "AMD EPYC 7742 (16 GB)",
                 "cx27": "AMD EPYC 7742 (40 GB)"},
                 regex=True)

g = sns.FacetGrid(df, col="MACHINE", row="VECTOR_SIZE_STR", height=4, sharex=True, sharey=False, legend_out=True, margin_titles=True)


def custom_lineplot(data, **kwargs):
    sns.lineplot(data=data, x="OFFSET", y="RUNTIME_MS", style="KEY", hue="KEY", **kwargs)
    plt.xticks(data["OFFSET"].unique())  # Set x-ticks to the unique values of OFFSET
    plt.xscale("log", base=2)
    ticks = [2**i for i in range(int(np.log2(data["OFFSET"].min())), int(np.log2(data["OFFSET"].max())) + 1)]
    plt.xticks(ticks, ticks)  # Set x-ticks to powers of 2

    almost_max_y = data['RUNTIME_MS'].quantile(0.9925)  # macOS is too flaky for drawing max()
    # plt.ylim(0, almost_max_y)

g.map_dataframe(custom_lineplot)

g.set_titles(col_template="{col_name}", row_template="Data Set Size: {row_name}")
g.set_axis_labels("Prefetching Offset", "Runtime (ms)")

g.add_legend()
g.fig.subplots_adjust(top=0.85)
g.fig.suptitle("Random Position List Summation\nRuntime Comparison for Different Prefetching Offsets and Localities (third parameter)", fontsize=16)

# Adjust the layout
# g.tight_layout()

# Show the plot
plt.show()