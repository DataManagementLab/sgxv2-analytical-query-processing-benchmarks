#!/usr/bin/python3
import sys

import pandas as pd
import seaborn as sns
import seaborn.objects as so
from matplotlib import pyplot as plt
from seaborn import axes_style

from helpers.commons import TUPLE_PER_MB, delete_all_configurations
from helpers.runner import compile_and_run_simple_flags, ExperimentConfig


def paper_plot_throughput(filename_detail):
    data = pd.read_csv(filename_detail, header=0)

    data = data[(data["measurement"] == "throughput") & (data["mode"] == "sgx")]

    data["Throughput in $10^6$ rows/s"] = data["value"]
    data["Enclave Size"] = data["dynamic_enclave"].replace({True: "Dynamic Enclave Size", False: "Static Enclave Size"})

    plt.figure(figsize=(6, 3))
    sns.barplot(data, y="Throughput in $10^6$ rows/s", x="Enclave Size", color=sns.color_palette("deep")[2],
                errorbar="sd")

    plt.xlabel(None)
    plt.ylim(bottom=0, top=800)
    plt.grid(axis="y")
    plt.tight_layout()

    plt.tight_layout()
    plot_filename = f"../img/paper-figure-slow-malloc.pdf"
    plt.savefig(plot_filename, transparent=False, bbox_inches='tight', pad_inches=0.1, dpi=300)
    # plt.show()


def main():
    experiment_name = "slow-malloc"

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
        ["sgx"],
        [["UNROLL", "FORCE_2_PHASES", "CHUNKED_TABLE"]],
        [(100 * TUPLE_PER_MB, 400 * TUPLE_PER_MB)],
        ["RHO"],
        [16],
        [True],
        10,
        dynamic_enclave=[False, True]
    )

    if execution_command in ["run", "both"]:
        delete_all_configurations()
        compile_and_run_simple_flags(config, filename_detail)
    if execution_command in ["plot", "both"]:
        sns.set_style("ticks")
        sns.set_context("notebook")
        sns.set_palette("deep")

        so.Plot.config.theme.update(axes_style("ticks"))
        paper_plot_throughput(filename_detail)


if __name__ == '__main__':
    main()
