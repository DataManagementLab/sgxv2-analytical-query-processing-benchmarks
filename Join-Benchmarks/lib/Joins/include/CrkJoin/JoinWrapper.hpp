#ifndef JOINWRAPPER_HPP
#define JOINWRAPPER_HPP

#include "data-types.h"

/**
 * Default CRKJ implementation that uses threaded partitioning and joining but no fused partitioning and joining
 * @param relR
 * @param relS
 * @param config
 * @return
 */
result_t *CRKJ(const relation_t *relR, const relation_t *relS, const joinconfig_t *config);

/**
 * This is a single-threaded version of CRKJ
 */
result_t *CRKJ_st(const relation_t *relR, const relation_t *relS, const joinconfig_t *config);

/**
 * This is a partition-only version of CRKJ. this is just for an experiment.
 * It does not do a join.
 */
result_t *CRKJ_partition_only(const relation_t *relR, const relation_t *relS, const joinconfig_t *config);

/** CRKJ Simple - partition everything at once and then join */
result_t *CRKJS(const relation_t *relR, const relation_t *relS, const joinconfig_t *config);

/** CRKJ Simple Single Thread - partition everything at once and then join */
result_t *CRKJS_st(const relation_t *relR, const relation_t *relS, const joinconfig_t *config);

/** CRKJ Fusion - build and probe while cracking - fuse cracking and joining */
result_t *CRKJF(const relation_t *relR, const relation_t *relS, const joinconfig_t *config);

/** CRKJ Fusion Single Thread - build and probe while cracking - fuse cracking and joining */
result_t *CRKJF_st(const relation_t *relR, const relation_t *relS, const joinconfig_t *config);

#endif //JOINWRAPPER_HPP
