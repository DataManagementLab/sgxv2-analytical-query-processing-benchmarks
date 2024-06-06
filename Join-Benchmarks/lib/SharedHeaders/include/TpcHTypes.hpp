#ifndef TPCTYPES_HPP
#define TPCTYPES_HPP

#include "data-types.h"
#include <stdint.h>

//Lineitem Table
const uint8_t L_SHIPMODE_MAIL = 1;
const uint8_t L_SHIPMODE_SHIP = 2;
const uint8_t L_SHIPMODE_AIR     = 3;
const uint8_t L_SHIPMODE_AIR_REG = 4;
const uint8_t L_SHIPINSTRUCT_DELIVER_IN_PERSON = 1;
// Customer Table
const uint8_t MKT_BUILDING = 1;
//Part Table
const uint8_t P_BRAND_12 = 1;
const uint8_t P_BRAND_23 = 2;
const uint8_t P_BRAND_34 = 3;
const uint8_t P_CONTAINER_SM_CASE  = 1;
const uint8_t P_CONTAINER_SM_BOX   = 2;
const uint8_t P_CONTAINER_SM_PACK  = 3;
const uint8_t P_CONTAINER_SM_PKG   = 4;
const uint8_t P_CONTAINER_MED_BAG  = 5;
const uint8_t P_CONTAINER_MED_BOX  = 6;
const uint8_t P_CONTAINER_MED_PKG  = 7;
const uint8_t P_CONTAINER_MED_PACK = 8;
const uint8_t P_CONTAINER_LG_CASE  = 9;
const uint8_t P_CONTAINER_LG_BOX   = 10;
const uint8_t P_CONTAINER_LG_PACK  = 11;
const uint8_t P_CONTAINER_LG_PKG   = 12;

// Query 12
const uint64_t TIMESTAMP_1995_01_01_SECONDS = 788918400;

// Query 3
const uint64_t TIMESTAMP_1995_03_15_SECONDS = 795225600;
const uint64_t TIMESTAMP_1995_03_16_SECONDS = 795312000;

// Query 10
const uint64_t TIMESTAMP_1993_10_01_SECONDS = 749433600;
const uint64_t TIMESTAMP_1994_01_01_SECONDS = 757382400;
const char L_RETURNFLAG_R = 'R';

typedef struct LineItemTable LineItemTable;
typedef struct OrdersTable OrdersTable;
typedef struct CustomerTable CustomerTable;
typedef struct PartTable PartTable;
typedef struct NationTable NationTable;

struct LineItemTable {
    uint64_t numTuples;
    tuple_t *l_orderkey; // key->orderkey, value->rowID
    uint64_t *l_shipdate;
    uint64_t *l_commitdate;
    uint64_t *l_receiptdate;
    uint8_t *l_shipmode;
    type_key *l_partkey;
    float *l_quantity;
    uint8_t *l_shipinstruct;
    char *l_returnflag;
};

struct OrdersTable {
    uint64_t numTuples;
    tuple_t *o_orderkey; //key->orderkey, value->rowID
    uint64_t *o_orderdate;
    type_key *o_custkey;
};

struct CustomerTable {
    uint64_t numTuples;
    tuple_t *c_custkey; //key->custkey, value->rowID
    uint8_t *c_mktsegment;
    type_key *c_nationkey;
};

struct PartTable {
    uint64_t numTuples;
    tuple_t *p_partkey; //key->partkey, value->rowID
    uint8_t *p_brand;
    uint32_t *p_size;
    uint8_t *p_container;
};

struct NationTable {
    uint64_t numTuples;
    tuple_t *n_nationkey; //key->nationkey, value->rowID
};
#endif //TPCTYPES_HPP
