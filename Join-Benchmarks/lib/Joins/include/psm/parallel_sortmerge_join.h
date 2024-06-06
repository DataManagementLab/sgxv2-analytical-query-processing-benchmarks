#ifndef PARALLEL_SORTMERGE_JOIN_H
#define PARALLEL_SORTMERGE_JOIN_H

#include "data-types.h"


/*
 * Parallel Sort Merge (PSM) Join.
 * This algorithm has two phases - sort and merge. Sorting is done using
 * a parallel quicksort algorithm originally published by Intel [1].
 * The merge phase follows an algorithm proposed in the book
 * "Database Management Systems" by Ramakrishnan and Gehrke.
 *
 * [1] https://software.intel.com/content/www/us/en/develop/articles/an-efficient-parallel-three-way-quicksort-using-intel-c-compiler-and-openmp-45-library.html
 * */
result_t* PSM (const table_t * relR, const table_t * relS, const joinconfig_t *config);
#endif //PARALLEL_SORTMERGE_JOIN_H
