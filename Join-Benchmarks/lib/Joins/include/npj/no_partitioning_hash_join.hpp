#ifndef NO_PARTITIONING_JOIN_H_
#define NO_PARTITIONING_JOIN_H_

#include "data-types.h"

result_t*
PHT (const table_t *relR, const table_t *relS, const joinconfig_t *config);

result_t *
PHT_no(const table_t *relR, const table_t *relS, const joinconfig_t *config);

result_t *
PHT_un(const table_t *relR, const table_t *relS, const joinconfig_t *config);

#endif // NO_PARTITIONING_JOIN_H_
