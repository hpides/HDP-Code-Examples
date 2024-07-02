import pandas as pd
import os

dataframes = []
for filename in os.listdir("."):
    if filename.endswith(".csv"):
        df = pd.read_csv(filename)
        dataframes.append(df)

df = pd.concat(dataframes, ignore_index=True)

print(df)