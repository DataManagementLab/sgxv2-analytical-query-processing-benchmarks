import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from pandas import DataFrame
from seaborn import color_palette


def plot_both(file, measurement):
    data = pd.read_csv(f"data/{file}.csv", header=0, skipinitialspace=True)

    data["setting"] = "sgx: " + data["sgx"] + " preload: " + data["preload"]

    plot = sns.relplot(data, x="num_bins", y=measurement, hue="setting",
                       markers=True, col="workload", kind="line", facet_kws={'sharey': True, 'sharex': True})

    plt.xscale('log')
    plt.xlim([1, 2 ** 20])
    plt.ylim(bottom=0)
    plot.axes[0, 0].set_xticks([2 ** x for x in range(0, 20)],
                               [str(2 ** x) + unit for unit in ["", "ki"] for x in range(0, 10)],
                               rotation=90)
    plot.axes[0, 1].set_xticks([2 ** x for x in range(0, 20)],
                               [str(2 ** x) + unit for unit in ["", "ki"] for x in range(0, 10)],
                               rotation=90)
    # ax2 = plot.axes[0, 0].twinx()
    # ax3 = plot.axes[0, 1].twinx()
    # sns.lineplot(data[data["workload"] == "hist"], x="num_keys", y="L1D-misses", hue="sgx", markers=True,
    #             palette=color_palette()[2:], ax=ax2)
    # sns.lineplot(data[data["workload"] == "copy"], x="num_keys", y="L1D-misses", hue="sgx", markers=True,
    #             palette=color_palette()[2:], ax=ax3)
    plt.savefig(f"img/{file}-{measurement}.png", dpi=150)
    plt.close()


def filter_workload(df: DataFrame, workload: str) -> DataFrame:
    return df[df["workload"] == workload]


def unroll_comparison():
    not_unrolled = pd.read_csv(f"data/results.csv", header=0, skipinitialspace=True)
    not_unrolled = filter_workload(not_unrolled, "copy")
    unrolled_4 = pd.read_csv(f"data/unroll-copy-4.csv", header=0, skipinitialspace=True)
    unrolled_4 = filter_workload(unrolled_4, "copy")
    unrolled_8 = pd.read_csv(f"data/unroll-copy-8.csv", header=0, skipinitialspace=True)
    unrolled_8 = filter_workload(unrolled_8, "copy")

    not_unrolled["unroll"] = 0
    unrolled_4["unroll"] = 4
    unrolled_8["unroll"] = 8

    data = pd.concat([not_unrolled, unrolled_4, unrolled_8])

    plot = sns.relplot(data, x="num_keys", y="timeMicroSec", hue="unroll", markers=True, col="sgx", kind="line",
                       facet_kws={'sharey': True, 'sharex': True}, palette=color_palette())

    plt.xscale('log')
    plt.xlim([1, 2 ** 20])
    plt.ylim(bottom=0)
    plot.axes[0, 0].set_xticks([2 ** x for x in range(0, 20)],
                               [str(2 ** x) + unit for unit in ["", "ki"] for x in range(0, 10)],
                               rotation=90)
    plot.axes[0, 1].set_xticks([2 ** x for x in range(0, 20)],
                               [str(2 ** x) + unit for unit in ["", "ki"] for x in range(0, 10)],
                               rotation=90)

    plot.tight_layout()

    plt.savefig(f"img/copy-unroll-comp.png", dpi=150)


def paper_figure_histogram(file):
    data = pd.read_csv(f"data/{file}.csv", header=0, skipinitialspace=True)
    data["Setting"] = data["sgx"] + data["preload"]
    data["Setting"].replace({"yesyes": "SGX Data in Enclave", "yesno": "SGX Data outside Enclave", "nono": "Plain CPU"},
                            inplace=True)
    data["Optimization"] = data["unroll"].replace({
        "no": "No Optimization",
        "yes": "Unrolling and Reordering"
    })
    data = data[data["workload"] == "hist"]
    data["Runtime in ms"] = data["timeMicroSec"] / 1000
    data["Number of Histogram Bins"] = data["num_bins"]

    sns.set_style("ticks")
    sns.set_context("notebook")

    plt.figure(figsize=(6, 3.75))
    plot = sns.lineplot(data, x="Number of Histogram Bins", y="Runtime in ms", hue="Setting", style="Optimization",
                        hue_order=["Plain CPU", "SGX Data outside Enclave", "SGX Data in Enclave"],
                        markers=True, errorbar="sd", palette=sns.color_palette("deep"), legend="auto")

    upper_bound = 12
    # plt.legend()

    plt.xscale('log')
    plt.xlim([1, 2 ** upper_bound])
    plt.ylim(bottom=25)

    plot.set_xticks([2 ** x for x in range(0, upper_bound + 1)],
                    [str(2 ** x) + unit for unit in ["", "ki"] for x in range(0, 10)][:upper_bound + 1],
                    rotation=90)

    sns.move_legend(plot, "center right", bbox_to_anchor=(1, 0.6))

    plt.tight_layout()
    plt.minorticks_off()
    plt.savefig(f"img/paper-figure-histogram.pdf", bbox_inches='tight', pad_inches=0.1, dpi=300)
    plt.close()


def paper_figure_partitioning(file):
    data = pd.read_csv(f"data/{file}.csv", header=0, skipinitialspace=True)
    data["Setting"] = data["sgx"] + data["preload"]
    data["Setting"].replace({"yesyes": "SGX Data in Enclave", "yesno": "SGX Data outside Enclave", "nono": "Plain CPU"},
                            inplace=True)
    data["Unrolled"] = data["unroll"].replace({
        "no": 1,
        "yes": 4
    })
    data = data[data["workload"] == "copy"]
    data["Runtime in ms"] = data["timeMicroSec"] / 1000
    data["Number of Partitions"] = data["num_bins"]

    plot = sns.lineplot(data, x="Number of Partitions", y="Runtime in ms", hue="Setting", style="Unrolled",
                        hue_order=["Plain CPU", "SGX Data outside Enclave", "SGX Data in Enclave"],
                        markers=True)

    upper_bound = 12

    plt.xscale('log')
    plt.xlim([1, 2 ** upper_bound])
    plt.ylim(bottom=0)

    plot.set_xticks([2 ** x for x in range(0, upper_bound + 1)],
                    [str(2 ** x) + unit for unit in ["", "ki"] for x in range(0, 10)][:upper_bound + 1],
                    rotation=90)

    # sns.move_legend(plot, "center right", bbox_to_anchor=(1, 0.65))

    plt.tight_layout()
    plt.minorticks_off()
    plt.savefig(f"img/paper-figure-partition-copy.pdf", dpi=300)
    plt.close()


def main():
    # unroll_comparison()

    # sns.set_palette(sns.color_palette(c_palette_1))

    # for file in ["radix-bits"]:
    #     for measurement in ["timeMicroSec", "instructions", "L1D-misses", "L1I-misses", "LLC-misses", "branch-misses", "dTLB", "iTLB"]:
    #         plot_both(file, measurement)
    paper_figure_histogram("histogram-10")
    # paper_figure_partitioning("histogram-10")


if __name__ == '__main__':
    main()
