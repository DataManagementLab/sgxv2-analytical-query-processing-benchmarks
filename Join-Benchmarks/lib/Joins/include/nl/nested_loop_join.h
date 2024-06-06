#ifndef _NESTED_LOO_JOIN_H_
#define _NESTED_LOO_JOIN_H_

#include "data-types.h"

result_t* NL (const table_t* relR, const table_t* relS, const joinconfig_t *config);

result_t* INL (const table_t* relR, const table_t* relS, const joinconfig_t *config);

#endif // _NESTED_LOO_JOIN_H_
