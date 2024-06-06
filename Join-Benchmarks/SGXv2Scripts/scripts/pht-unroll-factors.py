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

    settings = ["Plain CPU", "Plain CPU U+R", "Plain CPU U+R+M", "SGX Data in Enclave", "SGX U+R"]

    data["Throughput in $10^6$ rows/s"] = data["value"]
    data["unroll"] = data["flags"].str.contains("UNROLL")
    data["Setting"] = (data["mode"] + data["unroll"].astype(str) + data["mitigation"].astype(str)).replace({
        "nativeFalseFalse": settings[0],
        "nativeTrueFalse": settings[1],
        "nativeTrueTrue": settings[2],
        "sgxFalseFalse": settings[3],
        "sgxTrueFalse": settings[4],
    })
    data["Algorithm"] = data["alg"]

    sns.set_style("ticks")
    sns.set_context("notebook")
    sns.set_palette("deep")

    deep = sns.color_palette("deep")
    palette = [deep[0], deep[3], deep[4], deep[2], deep[5]]

    plt.figure(figsize=(6, 3.5))
    sns.barplot(data, y="Throughput in $10^6$ rows/s", hue="Setting", x="Algorithm", palette=palette[1:],
                hue_order=settings[1:], errorbar="sd")

    means = data.groupby(["mode", "unroll", "mitigation", "Algorithm"])["Throughput in $10^6$ rows/s"].mean()

    sgx_normal = means["sgx", False, False]
    sgx_opt = means["sgx", True, False]
    native_normal = means["native", False, False]
    native_opt = means["native", True, False]
    native_opt_mit = means["native", True, True]

    unroll_improvement = (sgx_opt / sgx_normal) - 1
    relative_perf_normal = sgx_normal / native_normal
    relative_perf_opt = sgx_opt / native_opt
    relative_perf_mit = sgx_opt / native_opt_mit

    print("Improvement due to unrolling")
    print(unroll_improvement.to_string())
    print("Relative performance before optimization")
    print(relative_perf_normal.to_string())
    print("Relative performance after optimization")
    print(relative_perf_opt.to_string())
    print("Performance relative to native with mitigation")
    print(relative_perf_mit.to_string())
    print("Relative PHT performance:", sgx_opt["PHT"] / sgx_opt["RHO"])

    plt.ylim(bottom=0)
    plt.grid(axis="y")
    plt.tight_layout()

    plot_filename = f"../img/paper-unroll-reorder-improvements.pdf"
    plt.savefig(plot_filename, transparent=False, bbox_inches='tight', pad_inches=0.1, dpi=300)
    # plt.show()


def main():
    experiment_name = "pht-unroll-factors"

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

    unroll_flags = [[f"PHT_UNROLL_FACTOR={unroll_factor}"] for unroll_factor in range(1, 13)]

    config = ExperimentConfig(
        ["native", "sgx"],
        unroll_flags,
        [(100 * TUPLE_PER_MB, 400 * TUPLE_PER_MB)],
        ["PHT_un"],
        [16],
        [False],
        3,
        mitigation=[False, True]
    )

    if execution_command in ["run", "both"]:
        delete_all_configurations()
        compile_and_run_simple_flags(config, filename_detail)
    if execution_command in ["plot", "both"]:
        paper_plot_unroll_reorder_improvements(filename_detail, experiment_name)


if __name__ == '__main__':
    main()
