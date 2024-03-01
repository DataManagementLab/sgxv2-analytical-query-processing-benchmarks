import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

L1 = 2 ** 14 * 3
L2 = 2 ** 18 * 5
L2_COMBINED = 2 ** 22 * 5
L3 = 2 ** 23 * 3

def plot_scans(native_file, sgx_file, scans):
    df_native = pd.read_csv(native_file, header=0)
    df_sgx = pd.read_csv(sgx_file, header=0)

    df = df_native[['testsize', 'nthreads', 'funcname']].copy()
    df['slowdown'] = df_sgx['bandwidth'].astype(float) / df_native['bandwidth'].astype(float)
    df = df[df['nthreads'] == 16]
    df = df[df['funcname'].isin(scans)]
    df = df[df['funcname'].str.contains("Read")]
    df = df[~df['funcname'].str.contains("32")]

    c_palette_1 = ["#D56257", "#8e8e8e", "#29292b", "#176582"]
    sns.set_style("ticks")
    sns.set_context("notebook")
    sns.set_palette(sns.color_palette(c_palette_1))

    plot = sns.lineplot(df, x='testsize', y='slowdown', hue='funcname', marker='o')

    plt.xscale('log')
    plt.xlim([2 ** 10, 2 ** 33])
    plt.ylim(bottom=0)

    plot.set_xticks([2 ** x for x in range(10, 34)],
                    ([str(2 ** x) + unit for unit in ["KB", "MB", "GB"] for x in range(0, 10)])[:24],
                    rotation=90)

    plt.axvline(x=2 ** 14 * 3, linestyle="--", color="k", label="L1 Cache Size")
    plt.axvline(x=2 ** 18 * 5, linestyle="--", color="r", label="L2 Cache Size")
    plt.axvline(x=2 ** 23 * 3, linestyle="--", color="b", label="L3 Cache Size")
    plot.legend()
    plt.tight_layout()
    plt.minorticks_off()
    # plt.savefig('results/rand_seq_access.pdf')
    plt.show()


def paper_figure_liner_read_write_max(native_file, sgx_file):
    df_native = pd.read_csv(native_file, header=0)
    df_sgx = pd.read_csv(sgx_file, header=0)

    df = df_native[['testsize', 'nthreads', 'funcname']].copy()
    df['Performance Relative to Plain CPU'] = df_sgx['bandwidth'].astype(float) / df_native['bandwidth'].astype(float)
    df = df[df['nthreads'] == 16]
    df = df[df['funcname'].isin(["ScanRead512PtrSimpleLoop", "ScanRead64PtrSimpleLoop",
                                 "ScanWrite512PtrSimpleLoop", "ScanWrite64PtrSimpleLoop"])]
    df["Data Size"] = df['testsize']
    df["Scan Type"] = df['funcname'].replace({"ScanRead512PtrSimpleLoop": "AVX 512 Read",
                                              "ScanRead64PtrSimpleLoop": "64bit Read",
                                              "ScanWrite512PtrSimpleLoop": "AVX 512 Write",
                                              "ScanWrite64PtrSimpleLoop": "64bit Write"})

    c_palette_1 = ["#D56257", "#8e8e8e", "#29292b", "#176582"]
    sns.set_style("ticks")
    sns.set_context("notebook")
    sns.set_palette(sns.color_palette(c_palette_1))

    plt.figure(figsize=(6, 4))
    plot = sns.lineplot(df, x='Data Size', y='Performance Relative to Plain CPU', hue='Scan Type',
                        hue_order=["64bit Read", "64bit Write", "AVX 512 Read", "AVX 512 Write"],
                        marker='o')

    plt.xscale('log')
    plt.xlim([2 ** 10, 2 ** 33])
    plt.ylim(bottom=0)

    plot.set_xticks([2 ** x for x in range(10, 34)],
                    ([str(2 ** x) + unit for unit in ["KB", "MB", "GB"] for x in range(0, 10)])[:24],
                    rotation=90)

    plt.axvline(x=2 ** 14 * 3, linestyle="--", color="k", label="L1 Cache Size")
    plt.axvline(x=2 ** 18 * 5, linestyle="--", color="r", label="L2 Cache Size")
    plt.axvline(x=2 ** 23 * 3, linestyle="--", color="b", label="L3 Cache Size")
    plot.legend()
    plt.tight_layout()
    plt.minorticks_off()
    plt.savefig('results/paper-figure-pmbw-linear.pdf')

    percentages = df.loc[df["Data Size"] == 2 ** 33, ["Scan Type", 'Performance Relative to Plain CPU']]
    print(percentages.to_string())
    plt.show()


def paper_figure_linear_read_write_max_3_part(native_files, sgx_files):
    df_native = pd.concat([pd.read_csv(native_file, header=0) for native_file in native_files])
    df_sgx = pd.concat([pd.read_csv(sgx_file, header=0) for sgx_file in sgx_files])

    df = df_native[['testsize', 'nthreads', 'funcname']].copy()
    df['Performance Relative to Plain CPU'] = df_sgx['bandwidth'].astype(float) / df_native['bandwidth'].astype(float)
    df = df[df['nthreads'] == 16]
    df = df[df["testsize"] >= 2 ** 12]
    df = df[df['funcname'].isin(["ScanRead512PtrSimpleLoop", "ScanRead64PtrSimpleLoop",
                                 "ScanWrite512PtrSimpleLoop", "ScanWrite64PtrSimpleLoop"])]
    df["Data Size"] = df['testsize']
    df["Scan Type"] = df['funcname'].replace({"ScanRead512PtrSimpleLoop": "AVX 512 Read",
                                              "ScanRead64PtrSimpleLoop": "64bit Read",
                                              "ScanWrite512PtrSimpleLoop": "AVX 512 Write",
                                              "ScanWrite64PtrSimpleLoop": "64bit Write"})

    c_palette_1 = ["#D56257", "#8e8e8e", "#29292b", "#176582"]
    sns.set_style("ticks")
    sns.set_context("notebook")
    sns.set_palette(sns.color_palette(c_palette_1))

    fig, (ax1, ax2, ax3) = plt.subplots(ncols=3, squeeze=True, figsize=(12, 3.5), width_ratios=(12, 5, 9))
    ax1 = sns.lineplot(df[df["Data Size"] <= 2 ** 23], x='Data Size', y='Performance Relative to Plain CPU',
                       hue='Scan Type', hue_order=["64bit Read", "64bit Write", "AVX 512 Read", "AVX 512 Write"],
                       marker='o', ax=ax1, legend=False)
    ax1.set_xlim((2**12, 2**23))
    ax1.set_xscale('log')
    ax1.set_xticks([2 ** x for x in range(12, 24)],
                   ([str(2 ** x) + unit for unit in ["KB", "MB", "GB"] for x in range(0, 10)])[2:14],
                   rotation=90)
    ax1.axvline(x=2 ** 18 * 3, linestyle="--", color="grey")
    ax1.text(0.69, 0.99, "Combined L1", rotation=90, ha='right', va='top', color="grey", transform=ax1.transAxes)
    ax1.axhline(1, linestyle="-", color="grey", alpha=0.5)

    ax2 = sns.lineplot(df[(df["Data Size"] >= 2 ** 23) & (df["Data Size"] <= 2 ** 27)], x='Data Size',
                       y='Performance Relative to Plain CPU',
                       hue='Scan Type', hue_order=["64bit Read", "64bit Write", "AVX 512 Read", "AVX 512 Write"],
                       marker='o', ax=ax2, legend=False)
    ax2.set_xlim((2 ** 23, 2 ** 27))
    ax2.set_xscale('log')
    ax2.set_xticks([2 ** x for x in range(23, 28)],
                   ([str(2 ** x) + unit for unit in ["KB", "MB", "GB"] for x in range(0, 10)])[13:18],
                   rotation=90)
    ax2.axvline(x=2 ** 22 * 5, linestyle="--", color="grey")
    ax2.axvline(x=2 ** 23 * 3, linestyle="--", color="grey")
    ax2.text(0.33, 0.99, "L2", rotation=90, ha='right', va='top', color="grey", transform=ax2.transAxes)
    ax2.text(0.41, 0.99, "L3", rotation=90, ha='left', va='top', color="grey", transform=ax2.transAxes)
    ax2.set_ylabel("")
    ax2.axhline(1, linestyle="-", color="grey", alpha=0.5)

    ax3 = sns.lineplot(df[df["Data Size"] >= 2 ** 27], x='Data Size', y='Performance Relative to Plain CPU',
                       hue='Scan Type', hue_order=["64bit Read", "64bit Write", "AVX 512 Read", "AVX 512 Write"],
                       marker='o', ax=ax3)
    ax3.set_xlim((2 ** 27, 2 ** 35))
    ax3.set_xscale('log')
    ax3.set_xticks([2 ** x for x in range(27, 36)],
                   ([str(2 ** x) + unit for unit in ["KB", "MB", "GB"] for x in range(0, 10)])[17:26],
                   rotation=90)
    # sns.move_legend(ax3, "center right", bbox_to_anchor=(1, 0.6))
    ax3.set_ylabel("")

    for ax in (ax1, ax2, ax3):
        ax.minorticks_off()
        ax.set(xlabel=None)

    plt.tight_layout(w_pad=0)
    plt.savefig('img/paper-figure-pmbw-linear-3-part.png')
    # plt.show()

    percentages = df.loc[df["Data Size"] == 2 ** 33, ["Scan Type", 'Performance Relative to Plain CPU']]
    print(percentages.to_string())
    # plt.show()


def paper_figure_linear_read_write_max_2_part(native_files, sgx_files):
    df_native = pd.concat([pd.read_csv(native_file, header=0) for native_file in native_files])
    df_sgx = pd.concat([pd.read_csv(sgx_file, header=0) for sgx_file in sgx_files])

    funcnames = ["ScanRead512PtrSimpleLoop", "ScanRead64PtrSimpleLoop",
                 "ScanWrite512PtrSimpleLoop", "ScanWrite64PtrSimpleLoop"]

    df_native = df_native[(df_native["nthreads"] == 16)
                          & (df_native["testsize"] >= 2 ** 12)
                          & (df_native["funcname"].isin(funcnames))]
    df_native["mode"] = "native"

    df_sgx = df_sgx[(df_sgx["nthreads"] == 16)
                    & (df_sgx["testsize"] >= 2 ** 12)
                    & (df_sgx["funcname"].isin(funcnames))]
    df_sgx["mode"] = "sgx"

    df = pd.concat([df_native, df_sgx])

    df["Data Size"] = df['testsize']
    df["Scan Type"] = df['funcname'].replace({"ScanRead512PtrSimpleLoop": "AVX 512 Read",
                                              "ScanRead64PtrSimpleLoop": "64bit Read",
                                              "ScanWrite512PtrSimpleLoop": "AVX 512 Write",
                                              "ScanWrite64PtrSimpleLoop": "64bit Write"})

    means = df.groupby(["mode", "Scan Type", "Data Size"])["bandwidth"].mean()
    relative = means["sgx"] / means["native"]
    relative.name = "Rel. Performance"
    relative = relative.reset_index()

    sns.set_style("ticks")
    sns.set_context("notebook")

    fig, (ax1, ax2) = plt.subplots(ncols=2, squeeze=True, figsize=(6, 2.75), width_ratios=(17, 9))
    ax1 = sns.lineplot(relative[relative["Data Size"] <= 2 ** 27], x='Data Size', y='Rel. Performance',
                       hue='Scan Type', hue_order=["64bit Read", "64bit Write", "AVX 512 Read", "AVX 512 Write"],
                       marker='o', ax=ax1, legend=True, errorbar="sd", palette=sns.color_palette("deep"))
    ax1.set_xlim((2**12, 2**27))
    ax1.set_xscale('log')
    ax1.set_xticks([2 ** x for x in range(12, 28)],
                   ([str(2 ** x) + unit for unit in ["KB", "MB", "GB"] for x in range(0, 10)])[2:18],
                   rotation=90)
    ax1.axvline(x=2 ** 18 * 3, linestyle="--", color="grey")
    ax1.axvline(x=2 ** 22 * 5, linestyle="--", color="grey")
    ax1.axvline(x=2 ** 23 * 3, linestyle="--", color="grey")
    ax1.text(0.505, 0.01, "L1", rotation=90, ha='right', va='bottom', color="grey", transform=ax1.transAxes)
    ax1.text(0.82, 0.01, "L2", rotation=90, ha='right', va='bottom', color="grey", transform=ax1.transAxes)
    ax1.text(0.85, 0.01, "L3", rotation=90, ha='left', va='bottom', color="grey", transform=ax1.transAxes)
    ax1.grid(axis="y")
    ax1.set_ylim(bottom=0.6)
    ax1.axhline(1, linestyle="-", color="grey", alpha=0.5, linewidth=1)

    ax2 = sns.lineplot(relative[relative["Data Size"] >= 2 ** 27], x='Data Size', y='Rel. Performance',
                       hue='Scan Type', hue_order=["64bit Read", "64bit Write", "AVX 512 Read", "AVX 512 Write"],
                       marker='o', ax=ax2, legend=False, errorbar="sd", palette=sns.color_palette("deep"))
    ax2.set_xlim((2 ** 27, 2 ** 35))
    ax2.set_xscale('log')
    ax2.set_xticks([2 ** x for x in range(27, 36)],
                   ([str(2 ** x) + unit for unit in ["KB", "MB", "GB"] for x in range(0, 10)])[17:26],
                   rotation=90)
    # sns.move_legend(ax3, "center right", bbox_to_anchor=(1, 0.6))
    ax2.grid(axis="y")
    ax2.axhline(1, linestyle="-", color="grey", alpha=0.5, linewidth=1)

    for ax in (ax1, ax2):
        ax.minorticks_off()
        ax.set(xlabel=None)

    fig.text(0.5, 0, 'Data Size', ha='center')

    plt.tight_layout(w_pad=0)
    plt.savefig('img/paper-figure-pmbw-linear-2-part.pdf', bbox_inches='tight', pad_inches=0.1, dpi=300)
    # plt.show()

    print(relative[relative["Data Size"] == 2 ** 35].to_string())
    # plt.show()


def paper_figure_random_access(native_files, sgx_files, write_file):
    # prepare random read data
    df_native = pd.concat([pd.read_csv(native_file, header=0) for native_file in native_files])
    df_sgx = pd.concat([pd.read_csv(sgx_file, header=0) for sgx_file in sgx_files])

    df = df_native[['testsize', 'nthreads', 'funcname']].copy()
    df["Rel. Performance"] = df_sgx['bandwidth'].astype(float) / df_native['bandwidth'].astype(float)
    df = df[df['funcname'] == "PermRead64SimpleLoop"]
    df["Data Size"] = df['testsize']
    df["Access Type"] = "Random Read"

    # prepare random write data
    df_write = pd.read_csv(f"data/{write_file}.csv", header=0, skipinitialspace=True)
    df_write["Runtime in s"] = df_write["timeMicroSec"] / 1000 / 1000
    df_write["Data Size"] = df_write["totalDataSize"]
    native = df_write.loc[df_write["sgx"] == "no", :]
    sgx = df_write.loc[(df_write["sgx"] == "yes") & (df_write["preload"] == "yes"), :]

    native.reset_index(inplace=True, drop=True)
    sgx.reset_index(inplace=True, drop=True)

    sgx["Rel. Performance"] = native["Runtime in s"] / sgx["Runtime in s"]
    sgx["Access Type"] = "Random Write"

    combined_df = pd.concat([sgx[["Access Type", "Data Size", "Rel. Performance"]],
                             df[["Access Type", "Data Size", "Rel. Performance"]]])

    sns.set_style("ticks")
    sns.set_context("notebook")

    order = ["Random Read", "Random Write"]
    plt.figure(figsize=(6, 3.25))
    plot = sns.lineplot(combined_df, x='Data Size', y='Rel. Performance', hue='Access Type', style='Access Type',
                        hue_order=order, style_order=order, errorbar="sd", palette=sns.color_palette("deep"))

    plt.xscale('log')
    plt.xlim([2 ** 10, 2 ** 35])
    plt.ylim(bottom=0)

    plot.set_xticks([2 ** x for x in range(10, 36)],
                    ([str(2 ** x) + unit for unit in ["KB", "MB", "GB"] for x in range(0, 10)])[:26],
                    rotation=90)

    plt.axvline(x=2 ** 14 * 3, linestyle="--", color="grey")
    plt.axvline(x=2 ** 18 * 5, linestyle="--", color="grey")
    plt.axvline(x=2 ** 23 * 3, linestyle="--", color="grey")
    plt.text(2 ** 14 * 3, 0.02, "L1", rotation=90, ha='right', va='bottom', color="grey")
    plt.text(2 ** 18 * 5, 0.02, "L2", rotation=90, ha='right', va='bottom', color="grey")
    plt.text(2 ** 23 * 3 + 2 ** 20, 0.02, "L3", rotation=90, ha='left', va='bottom', color="grey")
    sns.move_legend(plot, "center left", bbox_to_anchor=(0, 0.3))
    plot.yaxis.set_label_coords(-0.1, 0.4)
    plot.axhline(1, linestyle="-", color="grey", alpha=0.5, linewidth=1)

    plt.grid(axis="y")
    plt.tight_layout()
    plt.minorticks_off()
    plt.savefig('img/paper-figure-random.pdf', bbox_inches='tight', pad_inches=0.1, dpi=300)

    percentages = combined_df.loc[combined_df["Data Size"] >= 2 ** 33, :]
    mean_percentages = percentages.groupby(["Access Type", "Data Size"]).agg(["mean", "median", "min", "max"])
    print(mean_percentages.to_string())


def paper_figure_random_access_mean(native_files, sgx_files, write_file):
    # prepare random read data
    df_native = pd.concat([pd.read_csv(native_file, header=0) for native_file in native_files])
    df_sgx = pd.concat([pd.read_csv(sgx_file, header=0) for sgx_file in sgx_files])

    df_native = df_native[df_native['funcname'] == 'PermRead64SimpleLoop']
    df_sgx = df_sgx[df_sgx['funcname'] == 'PermRead64SimpleLoop']

    df_sgx = df_sgx.groupby('testsize')['bandwidth'].mean()
    df_native = df_native.groupby('testsize')['bandwidth'].mean()

    df = df_sgx.astype(float) / df_native.astype(float)
    df.name = 'Rel. Performance'
    df = df.reset_index()
    df["Data Size"] = df['testsize']
    df["Access Type"] = "Random Read"

    # prepare random write data
    df_write = pd.read_csv(f"data/{write_file}.csv", header=0, skipinitialspace=True)
    df_write = df_write[~((df_write["sgx"] == "yes") & (df_write["preload"] == "no"))]
    df_write["Runtime in s"] = df_write["timeMicroSec"] / 1000 / 1000
    df_write["Data Size"] = df_write["totalDataSize"]

    mean_runtime = df_write.groupby(["sgx", "Data Size"])["Runtime in s"].mean()

    df_write = mean_runtime.reset_index()

    native = df_write.loc[df_write["sgx"] == "no", :]
    sgx = df_write.loc[df_write["sgx"] == "yes", :]

    native.reset_index(inplace=True, drop=True)
    sgx.reset_index(inplace=True, drop=True)

    sgx["Rel. Performance"] = native["Runtime in s"] / sgx["Runtime in s"]
    sgx["Access Type"] = "Random Write"

    combined_df = pd.concat([sgx[["Access Type", "Data Size", "Rel. Performance"]],
                             df[["Access Type", "Data Size", "Rel. Performance"]]])

    sns.set_style("ticks")
    sns.set_context("notebook")

    order = ["Random Read", "Random Write"]
    plt.figure(figsize=(6, 2.75))
    plot = sns.lineplot(combined_df, x='Data Size', y='Rel. Performance', hue='Access Type', style='Access Type',
                        hue_order=order, style_order=order, errorbar="sd", palette=sns.color_palette("deep"))

    plt.xscale('log')
    plt.xlim([2 ** 10, 2 ** 35])
    plt.ylim(bottom=0)

    plot.set_xticks([2 ** x for x in range(10, 36)],
                    ([str(2 ** x) + unit for unit in ["KB", "MB", "GB"] for x in range(0, 10)])[:26],
                    rotation=90)

    plot.set_yticks([0, 0.25, 0.5, 0.75, 1, 1.25])

    plt.axvline(x=2 ** 14 * 3, linestyle="--", color="grey")
    plt.axvline(x=2 ** 18 * 5, linestyle="--", color="grey")
    plt.axvline(x=2 ** 23 * 3, linestyle="--", color="grey")
    plt.text(2 ** 14 * 3, 0.02, "L1", rotation=90, ha='right', va='bottom', color="grey")
    plt.text(2 ** 18 * 5, 0.02, "L2", rotation=90, ha='right', va='bottom', color="grey")
    plt.text(2 ** 23 * 3 + 2 ** 20, 0.02, "L3", rotation=90, ha='left', va='bottom', color="grey")
    sns.move_legend(plot, "center left", bbox_to_anchor=(0, 0.45))
    #plot.yaxis.set_label_coords(-0.1, 0.4)
    plot.axhline(1, linestyle="-", color="grey", alpha=0.5, linewidth=1)

    plt.grid(axis="y")
    plt.tight_layout()
    plt.minorticks_off()
    plt.savefig('img/paper-figure-random.pdf', bbox_inches='tight', pad_inches=0.1, dpi=300)

    percentages = combined_df.loc[combined_df["Data Size"] >= 2 ** 33, :]
    mean_percentages = percentages.groupby(["Access Type", "Data Size"]).agg(["mean", "median", "min", "max"])
    print(mean_percentages.to_string())


def paper_figure_random_read(native_files, sgx_files):
    df_native = pd.concat([pd.read_csv(native_file, header=0) for native_file in native_files])
    df_sgx = pd.concat([pd.read_csv(sgx_file, header=0) for sgx_file in sgx_files])

    df = df_native[['testsize', 'nthreads', 'funcname']].copy()
    df['Performance Rel. to Plain CPU'] = df_sgx['bandwidth'].astype(float) / df_native['bandwidth'].astype(float)
    # df = df[df['nthreads'] == 1]
    df = df[df['funcname'].isin(["PermRead64UnrollLoop", "PermRead64SimpleLoop"])]
    df["Data Size"] = df['testsize']
    df["Scan Type"] = df['funcname'].replace({"PermRead64SimpleLoop": "Pointer Chasing Simple Loop",
                                              "PermRead64UnrollLoop": "Pointer Chasing Unrolled Loop"})

    c_palette_1 = ["#D56257", "#8e8e8e", "#29292b", "#176582"]
    sns.set_style("ticks")
    sns.set_context("notebook")
    sns.set_palette(sns.color_palette(c_palette_1))

    plt.figure(figsize=(6, 3.25))
    plot = sns.lineplot(df, x='Data Size', y='Performance Rel. to Plain CPU', hue='Scan Type',
                        hue_order=["Pointer Chasing Simple Loop", "Pointer Chasing Unrolled Loop"],
                        marker='o', errorbar="sd", palette=sns.color_palette("deep"))

    plt.xscale('log')
    plt.xlim([2 ** 10, 2 ** 35])
    plt.ylim(bottom=0)

    plot.set_xticks([2 ** x for x in range(10, 36)],
                    ([str(2 ** x) + unit for unit in ["KB", "MB", "GB"] for x in range(0, 10)])[:26],
                    rotation=90)

    plt.axvline(x=L1, linestyle="--", color="grey")
    plt.axvline(x=L2, linestyle="--", color="grey")
    plt.axvline(x=L3, linestyle="--", color="grey")
    plt.text(L1, 0.02, "L1", rotation=90, ha='right', va='bottom', color="grey")
    plt.text(L2, 0.02, "L2", rotation=90, ha='right', va='bottom', color="grey")
    plt.text(L3, 0.02, "L3", rotation=90, ha='left', va='bottom', color="grey")
    sns.move_legend(plot, "center left", bbox_to_anchor=(0, 0.3))
    plot.yaxis.set_label_coords(-0.1, 0.4)
    plot.axhline(1, linestyle="-", color="grey", alpha=0.5, linewidth=1)

    plt.tight_layout()
    plt.minorticks_off()
    plt.savefig('img/paper-figure-pmbw-random.pdf')

    percentages = df.loc[df["Data Size"] >= 2 ** 33, ["Scan Type", "Data Size", 'Performance Rel. to Plain CPU']]
    mean_percentages = percentages.groupby(["Scan Type", "Data Size"]).agg(["mean", "median", "min", "max"])
    print(mean_percentages.to_string())
    # plt.show()


if __name__ == '__main__':
    # plot_scans('results/pmbw_native_stats_fixed.csv', 'results/pmbw_enclave_stats_fixed.csv',["ScanRead64PtrSimpleLoop"])

    native_files = [f"data/pmbw_native_stats_large_{n}.csv" for n in range(1, 11)]
    sgx_files = [f"data/pmbw_enclave_stats_large_{n}.csv" for n in range(1, 11)]
    paper_figure_linear_read_write_max_2_part(native_files, sgx_files)  # figure 6

    native_files_pc = [f"data/pmbw_native_pointer_chasing_{n}.csv" for n in range(1, 11)]
    sgx_files_pc = [f"data/pmbw_enclave_pointer_chasing_{n}.csv" for n in range(1, 11)]
    # paper_figure_random_read(native_files, sgx_files)  # figure 8

    paper_figure_random_access_mean(native_files_pc, sgx_files_pc, "write_area_increase")
