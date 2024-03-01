import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from pandas import DataFrame
from seaborn import color_palette


def plot_relative():
    data = pd.read_csv("data/first-experiment.csv", header=0, skipinitialspace=True)
    data = data[data["num_elements"] >= 1014]

    data = data.set_index(["num_elements"])

    user_check = data.loc[(data["sgx"] == "yes") & (data["preload"] == "no"), "cpuCycles"]
    preload = data.loc[(data["sgx"] == "yes") & (data["preload"] == "yes"), "cpuCycles"]
    native = data.loc[(data["sgx"] == "no") & (data["preload"] == "no"), "cpuCycles"]

    user_check_rel = user_check / native
    preload_rel = preload / native

    data = pd.concat([user_check_rel, preload_rel], axis=1)
    data.columns = ["user_check", "preload"]
    # data.reset_index()

    data.plot()
    plt.ylim(bottom=0)
    plt.xscale('log')
    plt.xticks([2 ** x for x in range(10, 28)],
                ([str(2 ** x) + unit for unit in ["K", "M", "G"] for x in range(0, 10)])[:18],
                rotation=90)
    plt.minorticks_off()
    plt.show()


def main():
    # unroll_comparison()

    c_palette_1 = ["#D56257", "#8e8e8e", "#29292b", "#176582"]
    c_palette_2 = ["#d56257", "#8e8e8e", "#2a9d8f", "#176582", "#264653"]

    sns.set_style("ticks")
    sns.set_context("notebook")
    sns.set_palette(sns.color_palette(c_palette_1))

    # for file in ["radix-bits"]:
    #     for measurement in ["timeMicroSec", "instructions", "L1D-misses", "L1I-misses", "LLC-misses", "branch-misses", "dTLB", "iTLB"]:
    #         plot_both(file, measurement)
    plot_relative()


if __name__ == '__main__':
    main()
