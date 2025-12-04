import pandas as pd
import matplotlib.pyplot as plt
import argparse
import time

def load_data(path):
    df = pd.read_csv(path, header=None, names=["timestamp", "key", "value"])
    df["timestamp"] = df["timestamp"] / 1000.0  # ms → seconds
    return df

def summarize(df):
    print("\n=== SUMMARY ===")
    for key in df["key"].unique():
        values = df[df["key"] == key]["value"]
        print(f"{key}: mean={values.mean():.4f}, median={values.median():.4f}, max={values.max():.4f}, min={values.min():.4f}")

def plot(df):
    keys = df["key"].unique()
    for key in keys:
        d = df[df["key"] == key]
        plt.figure(figsize=(10,4))
        plt.plot(d["timestamp"], d["value"])
        plt.title(f"{key} over time")
        plt.xlabel("Time (s)")
        plt.ylabel(key)
        plt.grid(True)
        plt.tight_layout()
        plt.show()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("csv_path")
    parser.add_argument("--plot", action="store_true")
    args = parser.parse_args()

    df = load_data(args.csv_path)
    summarize(df)

    if args.plot:
        plot(df)

if __name__ == "__main__":
    main()
