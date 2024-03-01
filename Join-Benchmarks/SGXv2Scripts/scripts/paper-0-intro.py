#!/usr/bin/python3
import sys

import pandas as pd
import seaborn as sns
import seaborn.objects as so
from matplotlib import pyplot as plt
from seaborn import axes_style

from helpers.commons import TUPLE_PER_MB, delete_all_configurations
from helpers.runner import compile_and_run_simple_flags, ExperimentConfig


def paper_intro_plot(filename_detail):
    joins = ["SGXv1-optimized\nJoin", "Radix Join\nin SGX", "SGX-optimized\nRadix Join", "Radix Join\noutside Enclave"]

    data = pd.read_csv(filename_detail, header=0)
    data = data[data["measurement"] == "throughput"]
    data["Throughput in $10^6$ rows/s"] = data["value"]

    data["Join"] = (data["mode"] + data["alg"] + data["flags"].str.contains("UNROLL").astype(str)).replace({
        "nativeCRKJFalse": "",
        "nativeCRKJTrue": "",
        "sgxCRKJFalse": joins[0],
        "sgxCRKJTrue": joins[0],
        "nativeRHOFalse": "",
        "nativeRHOTrue": joins[3],
        "sgxRHOFalse": joins[1],
        "sgxRHOTrue": joins[2],
    })

    plt.xticks(rotation=90)

    plt.figure(figsize=(6, 3))
    p = sns.barplot(data,  y="Throughput in $10^6$ rows/s", x="Join", hue="Join", order=joins, hue_order=joins)
    p.set(xlabel=None)
    p.set_ylim(bottom=0, top=1500)
    means = data.groupby("Join")["Throughput in $10^6$ rows/s"].mean()

    for container in p.containers:
        p.bar_label(container, fmt="%.1f")

    #p.bar_label(p.containers[0]) #, labels=[f"{means[joins[i]]:.0f}" for i in range(len(joins))], padding=3)

    plt.grid(axis="y")
    plt.tight_layout()
    plot_filename = f"../img/paper-figure-intro-new.pdf"
    plt.savefig(plot_filename, bbox_inches='tight', pad_inches=0.1, dpi=300)
    #plt.show()


def main():
    experiment_name = "intro"

    if len(sys.argv) < 2:
        print("Please specify run/plot/both")
        exit(-1)

    execution_command = sys.argv[1]

    if len(sys.argv) < 3 and execution_command == "run":
        print(f"Experiment name defaults to {experiment_name}. Are you sure?")
        user_input = input("y/N: ")
        if user_input.lower().strip() != "y":
            exit()
    elif len(sys.argv) == 3:
        experiment_name = sys.argv[2]

    filename_detail = f"../data/{experiment_name}.csv"

    config = ExperimentConfig(
        ["native", "sgx"],
        [["UNROLL", "FORCE_2_PHASES"], ["FORCE_2_PHASES"]],
        [(100 * TUPLE_PER_MB, 400 * TUPLE_PER_MB)],
        ["RHO", "CRKJ"],
        [16],
        [False],
        10,
    )

    if execution_command in ["run", "both"]:
        delete_all_configurations()
        compile_and_run_simple_flags(config, filename_detail)
    if execution_command in ["plot", "both"]:
        sns.set_style("ticks")
        sns.set_context("notebook")
        sns.set_palette("deep")

        so.Plot.config.theme.update(axes_style("ticks"))
        paper_intro_plot(filename_detail)


if __name__ == '__main__':
    main()
