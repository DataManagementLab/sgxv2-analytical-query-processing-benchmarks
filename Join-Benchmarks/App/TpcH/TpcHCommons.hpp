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
    bool parallel;

    tcph_args_t() = default;
};

std::string
getPath(int scale, const std::string &tbl);

void
tpch_parse_args(int argc, char **argv, tcph_args_t *params);

int
load_lineitems_from_binary(LineItemTable *l_table, uint8_t query, uint8_t scale);
int
load_lineitem_from_csv(LineItemTable *l_table, uint8_t scale);
void
free_lineitem(LineItemTable *l_table);

int
load_orders_from_binary(OrdersTable *o_table, uint8_t query, uint8_t scale);
int
load_orders_from_csv(OrdersTable *o_table, uint8_t scale);
void
free_orders(OrdersTable *o_table);

int
load_customers_from_binary(CustomerTable *c_table, uint8_t query, uint8_t scale);
int
load_customer_from_csv(CustomerTable *c_table, uint8_t scale);
void
free_customer(CustomerTable *c_table);

int
load_parts_from_binary(PartTable *p, uint8_t query, uint8_t scale);
int
load_part_from_csv(PartTable *p, uint8_t scale);
void
free_part(PartTable *p);

int
load_nations_from_binary(NationTable *n_table, uint8_t query, uint8_t scale);
int
load_nation_from_csv(NationTable *n, uint8_t scale);
void
free_nation(NationTable *n);

void
print_query_results(uint64_t totalTime, uint64_t filterTime, uint64_t joinTime);
#endif//TPCH_COMMONS_HPP
