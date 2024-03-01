#include "Logger.hpp"
#include "TpcHCommons.hpp"
#include "tpch.hpp"
#include "Barrier.hpp"
#include <thread>
#include <vector>

char experiment_filename[512];
int write_to_file = 0;

int
main(int argc, char *argv[]) {
    initLogger();
    joinconfig_t joinconfig;
    logger(INFO, "************* TPC-H APP *************");
    // 1. Parse args
    tcph_args_t params;
    tpch_parse_args(argc, argv, &params);
    uint8_t query = params.query;
    joinconfig.NTHREADS = (int) params.nthreads;
    joinconfig.RADIXBITS = -1;
    logger(INFO, "Run Q%d (scale %d) with join algorithm %s (%d threads)", query, params.scale, params.algorithm_name,
           joinconfig.NTHREADS);

    // 2. load required TPC-H tables
    LineItemTable l;
    OrdersTable o;
    CustomerTable c;
    PartTable p;
    NationTable n;

    constexpr uint64_t num_threads = 34;

    logger(INFO, "Hogging memory");
    Barrier b {num_threads};
    auto memory_hog = [&b]() {
        b.wait();
        std::vector<char> huge_area(1ULL << 30);
        std::fill(huge_area.begin(), huge_area.end(), 42);
    };

    std::vector<std::thread> memory_hog_threads {};
    for (uint64_t i = 0; i < num_threads - 1; ++i) {
        memory_hog_threads.emplace_back(memory_hog);
    }
    memory_hog();

    for (std::thread &thread : memory_hog_threads) {
        thread.join();
    }
    logger(INFO, "Done.");

    load_orders_binary(&o, query, params.scale);
    load_customers_binary(&c, query, params.scale);
    load_parts_binary(&p, query, params.scale);
    load_nations_binary(&n, query, params.scale);
    load_lineitems_binary(&l, query, params.scale);

    // 4. execute specified query
    result_t result{};
    switch (query) {
        case 3:
            tpch_q3(&result, &c, &o, &l, params.algorithm_name, &joinconfig);
            break;
        case 10:
            tpch_q10(&result, &c, &o, &l, &n, params.algorithm_name, &joinconfig);
            break;
        case 12:
            tpch_q12(&result, &l, &o, params.algorithm_name, &joinconfig);
            break;
        case 19:
            tpch_q19(&result, &l, &p, params.algorithm_name, &joinconfig);
            break;
        default:
            logger(ERROR, "TPC-H Q%d is not supported", query);
    }
    // 4. report and clean up
    logger(INFO, "Query completed");

    free_orders(&o, query);
    free_part(&p, query);
    free_customer(&c, query);
    free_lineitem(&l, query);
    free_nation(&n, query);
}
