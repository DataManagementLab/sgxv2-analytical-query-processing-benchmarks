#!/usr/bin/python3
import sys

import pandas as pd
import seaborn as sns
from matplotlib import pyplot as plt

from helpers.commons import delete_all_configurations, TUPLE_PER_MB
from helpers.runner import compile_and_run_simple_flags, ExperimentConfig


def paper_plot_unroll_reorder_improvements(filename_detail, experiment_name):
    data = pd.read_csv(filename_detail, header=0)

    data = data[(data["measurement"] == "throughput")]

    settings = ["Plain CPU", "Plain CPU Optimized", "SGX Data in Enclave", "SGX Optimized"]

    data["Throughput in $10^6$ rows/s"] = data["value"]
    data["unroll"] = data["flags"].str.contains("UNROLL")
    data["Setting"] = (data["mode"] + data["unroll"].astype(str)).replace({
        "nativeFalse": settings[0],
        "nativeTrue": settings[1],
        "sgxFalse": settings[2],
        "sgxTrue": settings[3],
    })
    data["Algorithm"] = data["alg"]

    sns.set_style("ticks")
    sns.set_context("notebook")
    sns.set_palette("deep")

    plt.figure(figsize=(6, 3.6))
    sns.barplot(data, y="Throughput in $10^6$ rows/s", hue="Setting", x="Algorithm", hue_order=settings, errorbar="sd")

    means = data.groupby(["mode", "unroll", "Algorithm"])["Throughput in $10^6$ rows/s"].mean()

    sgx_normal = means["sgx", False]
    sgx_opt = means["sgx", True]
    native_normal = means["native", False]
    native_opt = means["native", True]

    unroll_improvement = (sgx_opt / sgx_normal) - 1
    relative_perf_normal = sgx_normal / native_normal
    relative_perf_opt = sgx_opt / native_opt

    print(native_opt.to_string())
    print(sgx_normal.to_string())
    print(sgx_opt.to_string())
    print(unroll_improvement.to_string())
    print(relative_perf_normal.to_string())
    print(relative_perf_opt.to_string())
    print("Relative PHT performance:", sgx_opt["PHT"] / sgx_opt["RHO"])

    plt.ylim(bottom=0)
    plt.grid(axis="y")
    plt.tight_layout()

    plot_filename = f"../img/paper-unroll-reorder-improvements.pdf"
    plt.savefig(plot_filename, transparent=False, bbox_inches='tight', pad_inches=0.1, dpi=300)
    # plt.show()


def main():
    experiment_name = "unroll-reorder-improvements"

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
        [["FORCE_2_PHASES"], ["FORCE_2_PHASES", "UNROLL"]],
        [(100 * TUPLE_PER_MB, 400 * TUPLE_PER_MB)],
        ["RHO", "PHT"],
        [16],
        [False],
        10
    )

    if execution_command in ["run", "both"]:
        delete_all_configurations()
        compile_and_run_simple_flags(config, filename_detail)
    if execution_command in ["plot", "both"]:
        paper_plot_unroll_reorder_improvements(filename_detail, experiment_name)


if __name__ == '__main__':
    main()
