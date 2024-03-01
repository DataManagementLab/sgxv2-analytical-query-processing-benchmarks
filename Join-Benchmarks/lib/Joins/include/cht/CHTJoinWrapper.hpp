#ifndef CHTJOINWRAPPER_HPP_
#define CHTJOINWRAPPER_HPP_

#include "data-types.h"

template<int numbits>
join_result_t CHTJ(const table_t *, const table_t *, const joinconfig_t *);

//int64_t CHT(relation_t *relR, relation_t *relS, int nthreads);

#endif
