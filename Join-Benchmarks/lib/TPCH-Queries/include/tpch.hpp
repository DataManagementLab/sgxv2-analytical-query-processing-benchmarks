#ifndef SGXV2_JOIN_BENCHMARKS_TPCH_HPP
#define SGXV2_JOIN_BENCHMARKS_TPCH_HPP

#include "TpcHTypes.hpp"
#include "data-types.h"

void
tpch_q3(result_t *result, const struct CustomerTable *c, const struct OrdersTable *o,
              const struct LineItemTable *l, const char *algorithm, struct joinconfig_t *config);

void
tpch_q10(result_t *result, const CustomerTable *c, const OrdersTable *o, const LineItemTable *l,
               const NationTable *n, const char *algorithm, joinconfig_t *config);

void
tpch_q12(result_t *result, const LineItemTable *l, const OrdersTable *o, const char *algorithm,
               joinconfig_t *config);

void
tpch_q19(result_t *result, const LineItemTable *l, const PartTable *p, const char *algorithm,
               joinconfig_t *config);

#endif//SGXV2_JOIN_BENCHMARKS_TPCH_HPP
