#include "TpcHCommons.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <getopt.h>
#include <iomanip>
#include <istream>
#include <sstream>
#include <string>
#include <vector>

enum class CSVState { UnquotedField, QuotedField, QuotedQuote };

uint8_t
parsePartBrand(std::basic_string<char> &value);

uint8_t
parsePartContainer(std::basic_string<char> &value);

uint8_t
parseLineitemShipinstruct(std::basic_string<char> &value);

std::vector<std::string>
readCSVRow(const std::string &row, std::optional<size_t> columns = std::nullopt) {
    CSVState state = CSVState::UnquotedField;
    std::vector<std::string> fields{""};
    if (columns.has_value()) {
        fields.reserve(columns.value());
    }
    size_t i = 0;// index of the current field
    for (char c: row) {
        switch (state) {
            case CSVState::UnquotedField:
                switch (c) {
                    case '|':// end of field
                        fields.emplace_back();
                        i++;
                        break;
                    case '"':
                        state = CSVState::QuotedField;
                        break;
                    default:
                        fields[i].push_back(c);
                        break;
                }
                break;
            case CSVState::QuotedField:
                switch (c) {
                    case '"':
                        state = CSVState::QuotedQuote;
                        break;
                    default:
                        fields[i].push_back(c);
                        break;
                }
                break;
            case CSVState::QuotedQuote:
                switch (c) {
                    case '|':// , after closing quote
                        fields.emplace_back();
                        i++;
                        state = CSVState::UnquotedField;
                        break;
                    case '"':// "" -> "
                        fields[i].push_back('"');
                        state = CSVState::QuotedField;
                        break;
                    default:// end of quote
                        state = CSVState::UnquotedField;
                        break;
                }
                break;
        }
    }
    return fields;
}

/// Read CSV file, Excel dialect. Accept "quoted fields ""with quotes"""
std::vector<std::vector<std::string>>
readCSV(std::istream &in, std::optional<size_t> rows = std::nullopt, std::optional<size_t> columns = std::nullopt) {
    std::vector<std::vector<std::string>> table;
    if (rows.has_value()) {
        table.reserve(rows.value());
    }
    std::string row;
    while (!in.eof()) {
        std::getline(in, row);
        if (in.bad() || in.fail()) {
            break;
        }
        table.emplace_back(readCSVRow(row, columns));
    }
    return table;
}

void
tpch_parse_args(int argc, char **argv, tcph_args_t *params) {
    int c;
    while (1) {
        static struct option long_options[] = {
                //                        {"alg", required_argument, 0, 'a'},
                //                        {"query", required_argument, 0, 'q'},
                //                        {"threads", required_argument, 0, 'n'}
        };

        int option_index = 0;

        c = getopt_long(argc, argv, "a:b:m:n:q:s:", long_options, &option_index);

        if (c == -1)
            break;
        switch (c) {
            case 'a':
                strcpy(params->algorithm_name, optarg);
                break;
            case 'b':
                params->bits = (uint8_t) atoi(optarg);
                break;
            case 'n':
                params->nthreads = (uint8_t) atoi(optarg);
                break;
            case 'q':
                params->query = (uint8_t) atoi(optarg);
                break;
            case 's':
                params->scale = (uint8_t) atoi(optarg);
                break;
            default:
                break;
        }
    }

    if (optind < argc) {
        logger(INFO, "remaining arguments: ");
        while (optind < argc) {
            logger(INFO, "%s", argv[optind++]);
        }
    }
}

uint8_t
parseShipMode(const std::string &value) {
    if (value == "MAIL") {
        return L_SHIPMODE_MAIL;
    } else if (value == "SHIP") {
        return L_SHIPMODE_SHIP;
    } else if (value == "AIR") {
        return L_SHIPMODE_AIR;
    } else if (value == "AIR REG") {
        return L_SHIPMODE_AIR_REG;
    } else {
        return 0;
    }
}

uint64_t
parseDateToLong(const std::string &value) {
    std::tm tm = {};
    std::istringstream ss(value + " 00:00:00");
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    std::time_t time_stamp = mktime(&tm) - timezone;
    return static_cast<uint64_t>(time_stamp);
}

uint8_t
parseMktSegment(const std::string &value) {
    if (value == "BUILDING") {
        return MKT_BUILDING;
    } else {
        return 0;
    }
}

uint64_t
getNumberOfLines(const std::string &filename) {
    std::ifstream ifs(filename);
    auto lines = std::count(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>(), '\n');
    logger(INFO, "File %s has %lu lines", filename.c_str(), lines);
    return lines;
}

std::string
getPath(int scale, const std::string &tbl) {
    std::stringstream ss;
    ss << std::setw(3) << std::setfill('0') << scale;
    return "../data/scale" + ss.str() + "/" + tbl;
}

uint64_t
read_size_file(const std::string &path) {
    std::ifstream size_file{path + "/size"};
    uint64_t size;
    size_file >> size;
    size_file.close();
    return size;
}

void
read_binary(char *buffer, const uint64_t size, const std::string &path) {
    std::ifstream binary_file{path, std::ios::in | std::ios::binary};
    binary_file.read(buffer, size);
    binary_file.close();
}

int
load_lineitems_binary(LineItemTable *l_table, uint8_t query, uint8_t scale) {
    if (query != 3 && query != 10 && query != 12 && query != 19) {
        return 0;
    }

    const std::string PATH = getPath(scale, LINEITEM_TBL) + ".dir";
    uint64_t numTuples = read_size_file(PATH);

    l_table->numTuples = numTuples;

    int rv = 0;
    rv |= posix_memalign((void **) &(l_table->l_orderkey), 64, sizeof(tuple_t) * numTuples);
    if (query == 3) {
        rv |= posix_memalign((void **) &(l_table->l_shipdate), 64, sizeof(uint64_t) * numTuples);
    } else if (query == 10) {
        rv |= posix_memalign((void **) &(l_table->l_returnflag), 64, sizeof(char) * numTuples);
    } else if (query == 12) {
        rv |= posix_memalign((void **) &(l_table->l_shipdate), 64, sizeof(uint64_t) * numTuples);
        rv |= posix_memalign((void **) &(l_table->l_commitdate), 64, sizeof(uint64_t) * numTuples);
        rv |= posix_memalign((void **) &(l_table->l_receiptdate), 64, sizeof(uint64_t) * numTuples);
        rv |= posix_memalign((void **) &(l_table->l_shipmode), 64, sizeof(uint8_t) * numTuples);
    } else if (query == 19) {
        rv |= posix_memalign((void **) &(l_table->l_partkey), 64, sizeof(type_key) * numTuples);
        rv |= posix_memalign((void **) &(l_table->l_quantity), 64, sizeof(float) * numTuples);
        rv |= posix_memalign((void **) &(l_table->l_shipinstruct), 64, sizeof(uint8_t) * numTuples);
        rv |= posix_memalign((void **) &(l_table->l_shipmode), 64, sizeof(uint8_t) * numTuples);
    }
    if (rv != 0) {
        logger(ERROR, "memalign error");
        return 1;
    }

    read_binary(reinterpret_cast<char *>(l_table->l_orderkey), numTuples * sizeof(tuple_t),
                PATH + "/l_orderkey.bin");
    if (query == 3) {
        read_binary(reinterpret_cast<char *>(l_table->l_shipdate), numTuples * sizeof(uint64_t),
                    PATH + "/l_shipdate.bin");
    } else if (query == 10) {
        read_binary(reinterpret_cast<char *>(l_table->l_returnflag), numTuples * sizeof(char),
                    PATH + "/l_returnflag.bin");
    } else if (query == 12) {
        read_binary(reinterpret_cast<char *>(l_table->l_shipdate), numTuples * sizeof(uint64_t),
                    PATH + "/l_shipdate.bin");
        read_binary(reinterpret_cast<char *>(l_table->l_commitdate), numTuples * sizeof(uint64_t),
                    PATH + "/l_commitdate.bin");
        read_binary(reinterpret_cast<char *>(l_table->l_receiptdate), numTuples * sizeof(uint64_t),
                    PATH + "/l_receiptdate.bin");
        read_binary(reinterpret_cast<char *>(l_table->l_shipmode), numTuples * sizeof(uint8_t),
                    PATH + "/l_shipmode.bin");
    } else if (query == 19) {
        read_binary(reinterpret_cast<char *>(l_table->l_partkey), numTuples * sizeof(type_key),
                    PATH + "/l_partkey.bin");
        read_binary(reinterpret_cast<char *>(l_table->l_quantity), numTuples * sizeof(float),
                    PATH + "/l_quantity.bin");
        read_binary(reinterpret_cast<char *>(l_table->l_shipinstruct), numTuples * sizeof(uint8_t),
                    PATH + "/l_shipinstruct.bin");
        read_binary(reinterpret_cast<char *>(l_table->l_shipmode), numTuples * sizeof(uint8_t),
                    PATH + "/l_shipmode.bin");
    }

    return 0;
}

int
load_lineitem_from_file(LineItemTable *l_table, const uint8_t query, uint8_t scale) {
    if (query != 3 && query != 10 && query != 12 && query != 19) {
        return 0;
    }

    const std::string PATH = getPath(scale, LINEITEM_TBL);

    uint64_t numTuples = getNumberOfLines(PATH);

    l_table->numTuples = numTuples;
    int rv = 0;
    rv |= posix_memalign((void **) &(l_table->l_orderkey), 64, sizeof(tuple_t) * numTuples);
    if (query == 3) {
        rv |= posix_memalign((void **) &(l_table->l_shipdate), 64, sizeof(uint64_t) * numTuples);
    } else if (query == 10) {
        rv |= posix_memalign((void **) &(l_table->l_returnflag), 64, sizeof(char) * numTuples);
    } else if (query == 12) {
        rv |= posix_memalign((void **) &(l_table->l_shipdate), 64, sizeof(uint64_t) * numTuples);
        rv |= posix_memalign((void **) &(l_table->l_commitdate), 64, sizeof(uint64_t) * numTuples);
        rv |= posix_memalign((void **) &(l_table->l_receiptdate), 64, sizeof(uint64_t) * numTuples);
        rv |= posix_memalign((void **) &(l_table->l_shipmode), 64, sizeof(uint8_t) * numTuples);
    } else if (query == 19) {
        rv |= posix_memalign((void **) &(l_table->l_partkey), 64, sizeof(type_key) * numTuples);
        rv |= posix_memalign((void **) &(l_table->l_quantity), 64, sizeof(float) * numTuples);
        rv |= posix_memalign((void **) &(l_table->l_shipinstruct), 64, sizeof(uint8_t) * numTuples);
        rv |= posix_memalign((void **) &(l_table->l_shipmode), 64, sizeof(uint8_t) * numTuples);
    }
    if (rv != 0) {
        logger(ERROR, "memalign error");
        return 1;
    }

    std::ifstream data(PATH);
    auto rows = readCSV(data, numTuples, 16);
    logger(INFO, "lineitem table loaded from file");

    auto num_batches = (numTuples + 10'000 - 1) / 10'000;
    // parallelization does not help. Probably because of the linked data structure used.
    //#pragma omp parallel for num_threads(4)
    for (uint64_t batch = 0; batch < num_batches; ++batch) {
        auto batch_start = batch * 10'000;
        auto batch_end = std::min((batch + 1) * 10'000, numTuples);
        for (uint64_t i = batch_start; i < batch_end; ++i) {
            l_table->l_orderkey[i].key = (type_key) std::stoi(rows.at(i).at(0));
            l_table->l_orderkey[i].payload = (type_value) i;
            if (query == 3) {
                l_table->l_shipdate[i] = parseDateToLong(rows.at(i).at(10));
            } else if (query == 10) {
                l_table->l_returnflag[i] = rows.at(i).at(8).c_str()[0];
            } else if (query == 12) {
                l_table->l_shipdate[i] = parseDateToLong(rows.at(i).at(10));
                l_table->l_commitdate[i] = parseDateToLong(rows.at(i).at(11));
                l_table->l_receiptdate[i] = parseDateToLong(rows.at(i).at(12));
                l_table->l_shipmode[i] = parseShipMode(rows.at(i).at(14));
            } else if (query == 19) {
                l_table->l_partkey[i] = (type_key) std::stoi(rows.at(i).at(1));
                l_table->l_quantity[i] = std::stof(rows.at(i).at(4));
                l_table->l_shipinstruct[i] = parseLineitemShipinstruct(rows.at(i).at(13));
                l_table->l_shipmode[i] = parseShipMode(rows.at(i).at(14));
            }
        }
    }
    logger(INFO, "lineitem table parsed");
    return 0;
}

uint8_t
parseLineitemShipinstruct(std::basic_string<char> &value) {
    if (value == "DELIVER IN PERSON")
        return L_SHIPINSTRUCT_DELIVER_IN_PERSON;
    else
        return 0;
}

void
free_lineitem(LineItemTable *l_table, const uint8_t query) {
    if (query != 3 && query != 10 && query != 12 && query != 19) {
        return;
    }

    free(l_table->l_orderkey);
    switch (query) {
        case 3:
            free(l_table->l_shipdate);
            break;
        case 10:
            free(l_table->l_returnflag);
            break;
        case 12:
            free(l_table->l_shipdate);
            free(l_table->l_commitdate);
            free(l_table->l_receiptdate);
            free(l_table->l_shipmode);
            break;
        case 19:
            free(l_table->l_partkey);
            free(l_table->l_quantity);
            free(l_table->l_shipinstruct);
            free(l_table->l_shipmode);
            break;
        default:
            logger(ERROR, "Query %d not supported", query);
    }
}

int
load_orders_from_file(OrdersTable *o_table, const uint8_t query, uint8_t scale) {
    if (query != 3 && query != 10 && query != 12) {
        return 0;
    }

    const std::string PATH = getPath(scale, ORDERS_TBL);

    uint64_t numTuples = getNumberOfLines(PATH);

    o_table->numTuples = numTuples;
    int rv = posix_memalign((void **) &(o_table->o_orderkey), 64, sizeof(tuple_t) * numTuples);
    if (query == 3) {
        rv |= posix_memalign((void **) &(o_table->o_orderdate), 64, sizeof(uint64_t) * numTuples);
        rv |= posix_memalign((void **) &(o_table->o_custkey), 64, sizeof(type_key) * numTuples);
    } else if (query == 10) {
        rv |= posix_memalign((void **) &(o_table->o_orderdate), 64, sizeof(uint64_t) * numTuples);
        rv |= posix_memalign((void **) &(o_table->o_custkey), 64, sizeof(type_key) * numTuples);
    }
    if (rv != 0) {
        logger(ERROR, "memalign error");
        return 1;
    }
    std::ifstream data(PATH);
    auto rows = readCSV(data, numTuples, 9);
    logger(INFO, "orders table loaded from file");

    constexpr uint64_t BATCH_SIZE = 100'000;

    auto num_batches = (numTuples + BATCH_SIZE - 1) / BATCH_SIZE;
    //#pragma omp parallel for num_threads(4) shared(rows,o_table)
    for (uint64_t batch = 0; batch < num_batches; ++batch) {
        auto batch_start = batch * BATCH_SIZE;
        auto batch_end = std::min((batch + 1) * BATCH_SIZE, numTuples);
        for (uint64_t i = batch_start; i < batch_end; ++i) {
            o_table->o_orderkey[i].key = (type_key) std::stoi(rows[i][0]);
            o_table->o_orderkey[i].payload = (type_value) i;
            if (query == 3 || query == 10) {
                o_table->o_custkey[i] = (type_key) std::stoi(rows[i][1]);
                o_table->o_orderdate[i] = parseDateToLong(rows[i][4]);
            }
        }
    }
    logger(INFO, "orders table parsed");
    return 0;
}

int
load_orders_binary(OrdersTable *o_table, uint8_t query, uint8_t scale) {
    if (query != 3 && query != 10 && query != 12) {
        return 0;
    }

    const std::string PATH = getPath(scale, ORDERS_TBL) + ".dir";
    uint64_t numTuples = read_size_file(PATH);

    o_table->numTuples = numTuples;
    int rv = posix_memalign((void **) &(o_table->o_orderkey), 64, sizeof(tuple_t) * numTuples);
    if (query == 3 || query == 10) {
        rv |= posix_memalign((void **) &(o_table->o_orderdate), 64, sizeof(uint64_t) * numTuples);
        rv |= posix_memalign((void **) &(o_table->o_custkey), 64, sizeof(type_key) * numTuples);
    }
    if (rv != 0) {
        logger(ERROR, "memalign error");
        return 1;
    }

    read_binary(reinterpret_cast<char *>(o_table->o_orderkey), numTuples * sizeof(tuple_t),
                PATH + "/o_orderkey.bin");
    if (query == 3 || query == 10) {
        read_binary(reinterpret_cast<char *>(o_table->o_orderdate), numTuples * sizeof(uint64_t),
                    PATH + "/o_orderdate.bin");
        read_binary(reinterpret_cast<char *>(o_table->o_custkey), numTuples * sizeof(type_key),
                    PATH + "/o_custkey.bin");
    }

    return 0;
}

void
free_orders(OrdersTable *o_table, const uint8_t query) {
    if (query != 3 && query != 10 && query != 12) {
        return;
    }

    free(o_table->o_orderkey);
    switch (query) {
        case 3:
            free(o_table->o_custkey);
            free(o_table->o_orderdate);
            break;
        case 10:
            free(o_table->o_custkey);
            free(o_table->o_orderdate);
            break;
        default:
            break;
    }
}

int
load_customer_from_file(CustomerTable *c_table, const uint8_t query, uint8_t scale) {
    if (query != 3 && query != 10) {
        return 0;
    }
    const std::string PATH = getPath(scale, CUSTOMER_TBL);

    uint64_t numTuples = getNumberOfLines(PATH);

    c_table->numTuples = numTuples;
    int rv = posix_memalign((void **) &(c_table->c_custkey), 64, sizeof(tuple_t) * numTuples);
    if (query == 3) {
        rv |= posix_memalign((void **) &(c_table->c_mktsegment), 64, sizeof(uint8_t) * numTuples);
    } else if (query == 10) {
        rv |= posix_memalign((void **) &(c_table->c_nationkey), 64, sizeof(type_key) * numTuples);
    }

    if (rv != 0) {
        logger(ERROR, "memalign error");
        return 1;
    }
    std::ifstream data(PATH);
    auto rows = readCSV(data, numTuples, 8);
    logger(INFO, "customer table loaded from file");
    for (uint64_t i = 0; i < numTuples; i++) {
        c_table->c_custkey[i].key = (type_key) std::stoi(rows.at(i).at(0));
        c_table->c_custkey[i].payload = (type_value) i;
        if (query == 3)
            c_table->c_mktsegment[i] = parseMktSegment(rows.at(i).at(6));
        if (query == 10)
            c_table->c_nationkey[i] = (type_key) std::stoi(rows.at(i).at(3));
    }
    logger(INFO, "customer table parsed");
    return 0;
}

int
load_customers_binary(CustomerTable *c_table, uint8_t query, uint8_t scale) {
    if (query != 3 && query != 10) {
        return 0;
    }

    const std::string PATH = getPath(scale, CUSTOMER_TBL) + ".dir";
    uint64_t numTuples = read_size_file(PATH);

    c_table->numTuples = numTuples;

    int rv = posix_memalign((void **) &(c_table->c_custkey), 64, sizeof(tuple_t) * numTuples);
    if (query == 3) {
        rv |= posix_memalign((void **) &(c_table->c_mktsegment), 64, sizeof(uint8_t) * numTuples);
    } else if (query == 10) {
        rv |= posix_memalign((void **) &(c_table->c_nationkey), 64, sizeof(type_key) * numTuples);
    }
    if (rv != 0) {
        logger(ERROR, "memalign error");
        return 1;
    }

    read_binary(reinterpret_cast<char *>(c_table->c_custkey), numTuples * sizeof(tuple_t),
                PATH + "/c_custkey.bin");
    if (query == 3) {
        read_binary(reinterpret_cast<char *>(c_table->c_mktsegment), numTuples * sizeof(uint8_t),
                    PATH + "/c_mktsegment.bin");
    } else if (query == 10) {
        read_binary(reinterpret_cast<char *>(c_table->c_nationkey), numTuples * sizeof(type_key),
                    PATH + "/c_nationkey.bin");
    }

    return 0;
}

void
free_customer(CustomerTable *c_table, const uint8_t query) {
    if (query != 3 && query != 10) {
        return;
    }

    free(c_table->c_custkey);
    if (query == 3) {
        free(c_table->c_mktsegment);
    } else if (query == 10) {
        free(c_table->c_nationkey);
    }
}

int
load_part_from_file(PartTable *p, const uint8_t query, uint8_t scale) {
    if (query != 19) {
        return 0;
    }

    const std::string PATH = getPath(scale, PART_TBL);

    uint64_t numTuples = getNumberOfLines(PATH);

    p->numTuples = numTuples;
    int rv = posix_memalign((void **) &(p->p_partkey), 64, sizeof(tuple_t) * numTuples);
    if (query == 19) {
        rv |= posix_memalign((void **) &(p->p_brand), 64, sizeof(uint8_t) * numTuples);
        rv |= posix_memalign((void **) &(p->p_size), 64, sizeof(uint32_t) * numTuples);
        rv |= posix_memalign((void **) &(p->p_container), 64, sizeof(uint8_t) * numTuples);
    }
    if (rv != 0) {
        logger(ERROR, "memalign error");
        return 1;
    }
    std::ifstream data(PATH);
    auto rows = readCSV(data, numTuples, 9);
    logger(INFO, "part table loaded from file");
    for (uint64_t i = 0; i < numTuples; i++) {
        p->p_partkey[i].key = (type_key) std::stoi(rows.at(i).at(0));
        p->p_partkey[i].payload = (type_value) i;
        if (query == 19) {
            p->p_brand[i] = parsePartBrand(rows.at(i).at(3));
            p->p_size[i] = (uint32_t) std::stoi(rows.at(i).at(5));
            p->p_container[i] = parsePartContainer(rows.at(i).at(6));
        }
    }
    logger(INFO, "part table parsed");
    return 0;
}

int
load_parts_binary(PartTable *p, uint8_t query, uint8_t scale) {
    if (query != 19) {
        return 0;
    }

    const std::string PATH = getPath(scale, PART_TBL) + ".dir";
    uint64_t numTuples = read_size_file(PATH);

    p->numTuples = numTuples;

    int rv = posix_memalign((void **) &(p->p_partkey), 64, sizeof(tuple_t) * numTuples);
    if (query == 19) {
        rv |= posix_memalign((void **) &(p->p_brand), 64, sizeof(uint8_t) * numTuples);
        rv |= posix_memalign((void **) &(p->p_container), 64, sizeof(uint8_t) * numTuples);
        rv |= posix_memalign((void **) &(p->p_size), 64, sizeof(uint32_t) * numTuples);
    }
    if (rv != 0) {
        logger(ERROR, "memalign error");
        return 1;
    }

    read_binary(reinterpret_cast<char *>(p->p_partkey), numTuples * sizeof(tuple_t),
                PATH + "/p_partkey.bin");
    if (query == 19) {
        read_binary(reinterpret_cast<char *>(p->p_brand), numTuples * sizeof(uint8_t),
                    PATH + "/p_brand.bin");
        read_binary(reinterpret_cast<char *>(p->p_container), numTuples * sizeof(uint8_t),
                    PATH + "/p_container.bin");
        read_binary(reinterpret_cast<char *>(p->p_size), numTuples * sizeof(uint32_t),
                    PATH + "/p_size.bin");
    }

    return 0;
}

uint8_t
parsePartContainer(std::basic_string<char> &value) {
    if (value == "SM CASE")
        return P_CONTAINER_SM_CASE;
    else if (value == "SM BOX")
        return P_CONTAINER_SM_BOX;
    else if (value == "SM PACK")
        return P_CONTAINER_SM_PACK;
    else if (value == "SM PKG")
        return P_CONTAINER_SM_PKG;
    else if (value == "MED BAG")
        return P_CONTAINER_MED_BAG;
    else if (value == "MED BOX")
        return P_CONTAINER_MED_BOX;
    else if (value == "MED PKG")
        return P_CONTAINER_MED_PKG;
    else if (value == "MED PACK")
        return P_CONTAINER_MED_PACK;
    else if (value == "LG CASE")
        return P_CONTAINER_LG_CASE;
    else if (value == "LG BOX")
        return P_CONTAINER_LG_BOX;
    else if (value == "LG PACK")
        return P_CONTAINER_LG_PACK;
    else if (value == "LG PKG")
        return P_CONTAINER_LG_PKG;
    else
        return 0;
}

uint8_t
parsePartBrand(std::basic_string<char> &value) {
    if (value == "Brand#12")
        return P_BRAND_12;
    else if (value == "Brand#23")
        return P_BRAND_23;
    else if (value == "Brand#34")
        return P_BRAND_34;
    else
        return 0;
}

void
free_part(PartTable *p, const uint8_t query) {
    if (query != 19) {
        return;
    }

    free(p->p_partkey);
    switch (query) {
        case 19:
            free(p->p_brand);
            free(p->p_size);
            free(p->p_container);
            break;
        default:
            break;
    }
}

int
load_nation_from_file(NationTable *n, uint8_t query, uint8_t scale) {
    if (query != 10) {
        return 0;
    }

    const std::string PATH = getPath(scale, NATION_TBL);

    uint64_t numTuples = getNumberOfLines(PATH);

    n->numTuples = numTuples;
    int rv = posix_memalign((void **) &(n->n_nationkey), 64, sizeof(tuple_t) * numTuples);
    if (rv != 0) {
        logger(ERROR, "memalign error");
        return 1;
    }
    std::ifstream data(PATH);
    auto rows = readCSV(data, numTuples, 4);
    logger(INFO, "nation table loaded from file");
    for (uint64_t i = 0; i < numTuples; i++) {
        n->n_nationkey[i].key = (type_key) std::stoi(rows.at(i).at(0));
        n->n_nationkey[i].payload = (type_value) i;
    }
    logger(INFO, "nation table parsed");
    return 0;
}

int
load_nations_binary(NationTable *n, uint8_t query, uint8_t scale) {
    if (query != 10) {
        return 0;
    }

    const std::string PATH = getPath(scale, NATION_TBL) + ".dir";
    uint64_t numTuples = read_size_file(PATH);

    n->numTuples = numTuples;

    int rv = posix_memalign((void **) &(n->n_nationkey), 64, sizeof(tuple_t) * numTuples);
    if (rv != 0) {
        logger(ERROR, "memalign error");
        return 1;
    }

    read_binary(reinterpret_cast<char *>(n->n_nationkey), numTuples * sizeof(tuple_t),
                PATH + "/n_nationkey.bin");

    return 0;
}

void
free_nation(NationTable *n, uint8_t query) {
    if (query != 10) {
        return;
    }
    free(n->n_nationkey);
}