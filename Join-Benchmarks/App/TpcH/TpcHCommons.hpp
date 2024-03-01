#ifndef TPCH_COMMONS_HPP
#define TPCH_COMMONS_HPP

#include "TpcHTypes.hpp"
#include "data-types.h"
#include <string>

const std::string LINEITEM_TBL = "lineitem.tbl";
const std::string ORDERS_TBL = "orders.tbl";
const std::string CUSTOMER_TBL = "customer.tbl";
const std::string PART_TBL = "part.tbl";
const std::string NATION_TBL = "nation.tbl";

struct tcph_args_t {
    //    algorithm_t *algorithm;
    char algorithm_name[128]{"CrkJoin"};
    uint8_t query{12};
    uint8_t nthreads{4};
    uint8_t bits{13};
    uint8_t scale{1};

    tcph_args_t() = default;
};

std::string
getPath(int scale, const std::string &tbl);

void
tpch_parse_args(int argc, char **argv, tcph_args_t *params);

int
load_lineitems_binary(LineItemTable *l_table, uint8_t query, uint8_t scale);
int
load_lineitem_from_file(LineItemTable *l_table, uint8_t query, uint8_t scale);
void
free_lineitem(LineItemTable *l_table, uint8_t query);

int
load_orders_binary(OrdersTable *o_table, uint8_t query, uint8_t scale);
int
load_orders_from_file(OrdersTable *o_table, uint8_t query, uint8_t scale);
void
free_orders(OrdersTable *o_table, uint8_t query);

int
load_customers_binary(CustomerTable *c_table, uint8_t query, uint8_t scale);
int
load_customer_from_file(CustomerTable *c_table, uint8_t query, uint8_t scale);
void
free_customer(CustomerTable *c_table, uint8_t query);

int
load_parts_binary(PartTable *p, uint8_t query, uint8_t scale);
int
load_part_from_file(PartTable *p, uint8_t query, uint8_t scale);
void
free_part(PartTable *p, uint8_t query);

int
load_nations_binary(NationTable *n_table, uint8_t query, uint8_t scale);
int
load_nation_from_file(NationTable *n, uint8_t query, uint8_t scale);
void
free_nation(NationTable *n, uint8_t query);

void
print_query_results(uint64_t totalTime, uint64_t filterTime, uint64_t joinTime);
#endif//TPCH_COMMONS_HPP
