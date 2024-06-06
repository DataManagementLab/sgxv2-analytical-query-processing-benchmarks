#ifndef JOIN_HPP
#define JOIN_HPP

#include "Barrier.hpp"
#include "Commons.hpp"
#include "Logger.hpp"
#include "PTreeNode.hpp"
#include "data-types.h"
#include "util.hpp"
#include <atomic>
#include <cmath>
#include <map>
#include <vector>

#ifndef ENCLAVE
#include "ocalls.hpp"
#include <cstring>
#include <cstdio>
#else
#include "ocalls_t.h"
#ifdef DEBUG
#include "mbusafecrt.h"
#endif
#endif

#ifndef HASH_BIT_MODULO
#define HASH_BIT_MODULO(K, MASK, NBITS) (((K) >> NBITS) & MASK)
#endif

#ifndef PARTITION_STATS
#define PARTITION_STATS 0
#endif

#ifndef TIMERS
#define TIMERS
#endif

#ifndef L2_CACHE_SIZE
#define L2_CACHE_SIZE 1280
#endif

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64
#endif

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
      (byte & 0x80 ? '1' : '0'), \
      (byte & 0x40 ? '1' : '0'), \
      (byte & 0x20 ? '1' : '0'), \
      (byte & 0x10 ? '1' : '0'), \
      (byte & 0x08 ? '1' : '0'), \
      (byte & 0x04 ? '1' : '0'), \
      (byte & 0x02 ? '1' : '0'), \
      (byte & 0x01 ? '1' : '0')

using std::vector;

class Join {
private:
    uint32_t nthreads;
    uint32_t npart;
    relation_t *relR;
    relation_t *relS;
    uint64_t *histR;
    uint64_t *histS;
    uint64_t **localHistR;
    PTreeNode *pTreeS, *pTreeR;
    vector<tuple_t> *partLocals;
    vector<uint32_t> *nextLocals, *bucketLocals;
    vector<uint64_t> *timerPerPartitions;
#ifdef COUNT_SCANNED_TUPLES
    pthread_mutex_t lock;
    std::map<uint32_t, uint32_t> bitMap;
#endif

    const uint32_t MASK;
    const uint32_t SHIFT;
    const uint32_t NUM_BITS;
    uint64_t *matches;
    uint64_t *timer_ht, *timer_tree, *timer_partition, *timer_probe, *timerJoin;
    uint64_t t_usec, t_partition_usec, t_join_usec, t_join_build, t_join_partition, t_join_probe;
    const int WRITETOFILE;
    const bool MATERIALIZE;

    Barrier *barrier;

    std::vector<row_t> part;
#ifdef COUNT_SCANNED_TUPLES
    std::atomic<uint64_t> scanned_tupels {0};
#endif
//    std::vector<uint32_t> next, bucket;
    PTreeNode *probeNode;

    void
    traverse_partition(PTreeNode *node, PTreeNode *root, int left) {
        if (left <= 0) {
            return;
        }
        if (!node) {
            return;
        }

        partition_both_ends(node, root, 0);
        traverse_partition(node->getBranch0(), root, left - 1);
        traverse_partition(node->getBranch1(), root, left - 1);
    }

    void
    partition_to_disjoint_parts(uint32_t parts) {
        //SGX_ASSERT(parts == nthreads, "Number of threads not equal to number of disjoint parts");
        int depth = (int) std::log2(parts);
        logger(DBG, "Partition to %d disjoint parts (%d depth)", parts, depth);
        traverse_partition(pTreeS, pTreeS, depth);
        logger(DBG, "S partitioned");
        traverse_partition(pTreeR, pTreeR, depth);
        logger(DBG, "R partitioned");
#ifdef DEBUG
        logger(DBG,"--------pTreeS-----------");
        pTreeS->traverse(pTreeS);
        logger(DBG,"-------------------------");
        logger(DBG,"--------pTreeR-----------");
        pTreeR->traverse(pTreeR);
        logger(DBG,"-------------------------");
#endif
    }

    uint64_t
    probe_node(int threadID,
               tuple_t *tuples,
               uint64_t num,
               vector<uint32_t> bucket,
               vector<uint32_t> next,
               std::vector<tuple_t> R,
               uint32_t partNum,
               uint32_t maskP,
               output_list_t **output) {
        //SGX_ASSERT(tuples != nullptr, "pNode can not be null");
        (void) output;
        tuple_t t;
        uint32_t idx;
        int hit;
        uint64_t m = 0;
        size_t bucket_size = bucket.size();
        for (uint64_t i = 0; i < num; i++) {
            t = tuples[i];
            if (HASH_BIT_MODULO(t.key, MASK, SHIFT) == partNum) {
                idx = HASH_BIT_MODULO(t.key, maskP, 0);
                if (idx > bucket_size - 1) continue;
                for (hit = bucket.at(idx); hit > 0; hit = next.at(hit - 1)) {
                    if (t.key == R.at(hit - 1).key) {
                        matches[threadID]++;
                        m++;
                        if (MATERIALIZE) {
                            insert_output(output, t.key, R.at(hit - 1).payload, t.payload);
                        }
#ifdef DEBUG
                        if (WRITETOFILE) {
                            char buf[512];
#ifndef ENCLAVE
                            sprintf(buf, "%u,%u,%u\n", R.at(hit-1), t.key, t.payload);
#else
                            sprintf_s(buf, 512, "%u,%u,%u\n", R.at(hit-1), t.key, t.payload);
#endif
                            ocall_write_to_file(buf);
                        }
#endif
                    }
                }
            }
        }
        return m;
    }


    void
    print_relation(relation_t *rel, uint64_t num, uint64_t offset) {
        logger(DBG, "****************** Relation sample ******************");
        if (num == 0) {
            num = rel->num_tuples;
        }
        for (uint64_t i = offset; i < rel->num_tuples && i < num + offset; i++) {
            logger(DBG, "%u -> %u", rel->tuples[i].key, rel->tuples[i].payload);
        }
        logger(DBG, "******************************************************");
    }

public:
    Join(uint32_t _nthreads, uint32_t _numbits, relation_t *_relR, relation_t *_relS, int _writeOutputToFile,
         bool _materialize) :
            nthreads(_nthreads), npart(1 << _numbits), relR(_relR), relS(_relS), MASK(npart - 1),
            SHIFT((uint32_t) (__builtin_ctzl(Commons::nextPowerOfTwo(relR->num_tuples)) - __builtin_ctz(npart))),
            NUM_BITS(_numbits), WRITETOFILE(_writeOutputToFile), MATERIALIZE(_materialize) {
        //SGX_ASSERT(SHIFT < 33, "Can not shift by more than 32 bits");
        try {
            histR = new uint64_t[npart]();
            histS = new uint64_t[npart]();
            localHistR = new uint64_t *[nthreads]();
            for (uint32_t i = 0; i < nthreads; i++) {
                localHistR[i] = new uint64_t[npart]();
            }
            pTreeS = new PTreeNode(relS, NUM_BITS);
            pTreeR = new PTreeNode(relR, NUM_BITS);
            matches = new uint64_t[nthreads]();
            timer_ht = new uint64_t[nthreads]();
            timer_tree = new uint64_t[nthreads]();
            timer_partition = new uint64_t[nthreads]();
            timer_probe = new uint64_t[nthreads]();
            timerPerPartitions = new vector<uint64_t>[nthreads];
            timerJoin = new uint64_t[nthreads]();

            barrier = new Barrier(nthreads);
            partLocals = new vector<tuple_t>[nthreads];
            nextLocals = new vector<uint32_t>[nthreads];
            bucketLocals = new vector<uint32_t>[nthreads];
        } catch (std::bad_alloc &ex) {
            logger(ERROR, "Failed to allocate memory for CRKJ");
            ocall_exit(-1);
        }
#ifdef COUNT_SCANNED_TUPLES
        if (pthread_mutex_init(&lock, nullptr) != 0) {
            logger(ERROR,"mutex init has failed");
            ocall_exit(-1);
        }
#endif

        logger(INFO, "CrkJoin threads=%d, npart=%d", nthreads, npart);
        if (WRITETOFILE) {
            logger(DBG, "Log output to file enabled");
        }
//        print_relation(relR, relR->num_tuples, 0);
//        print_relation(relS, relS->num_tuples, 0);
    }

    /**
     * Partition everything under this node in dfs style
     * @param node
     * @param root
     */
    void crack_dfs(PTreeNode *node, PTreeNode *root) const {
        if (node == nullptr) {
            return;
        }
//        logger(INFO, "crack_dfs: %c%c%c%c%c%c%c%c", BYTE_TO_BINARY(node->getMask()));
        if (node->getBits() == node->getTotalBits()) {
            return;
        }
        partition_both_ends(node, root, 0);
        crack_dfs(node->getBranch0(), root);
        crack_dfs(node->getBranch1(), root);
    }

    /**
     * Partition everything under this node up to a depth threshold. Does not create threads. Uses partition_both_ends
     * for partitioning.
     * @param node
     * @param root
     * @param threshold absolute depth threshold
     */
    void crack_dfs(PTreeNode *node, PTreeNode *root, int threshold) const {
        if (node == nullptr) {
            return;
        }
        if (node->getBits() >= node->getTotalBits() - threshold) {
            logger(DBG, "Cut off cracking with %d bits", node->getBits());
            return;
        }
        partition_both_ends(node, root, 0);
        crack_dfs(node->getBranch0(), root, threshold);
        crack_dfs(node->getBranch1(), root, threshold);
    }

    /**
     * Partition a table using the pointer and exchange mechanism described in the paper.
     * @param pNode Node that describes what should be partitioned
     * @param root Root node of the tree describing the partitioning
     * @param partition The child node that fits this combination of radix bits is returned after partitioning
     * @return
     */
    PTreeNode *
    partition_both_ends(PTreeNode *pNode, PTreeNode *root, uint32_t partition) const {
        uint64_t pNum = pNode->getNum();
        // stop partitioning if there is one element left
        if (pNum <= 1) {
            return pNode;
        }
        // stop partitioning when the number of bits reaches the total number of bits
        if (pNode->getBits() >= pNode->getTotalBits()) {
            return pNode;
        }
        PTreeNode *n;
        tuple_t *tuples = pNode->getStart();
        uint64_t i0 = 0;
        uint64_t i1 = pNum - 1;
        uint32_t current_bit = SHIFT + pNode->getTotalBits() - pNode->getBits() - 1;
        while (i0 < i1) {
            /// break if the pointers switched places. Partition is now split by the bit and the break is between
            /// i1 and i0. If pointers are still on their side, go on moving them towards each other.

            /// Move left pointer forwards until a value that must be swapped is found.
            while (!Commons::check_bit(tuples[i0].key, current_bit) && i0 < pNum) {
                i0++;
            }
            /// Move the right pointer backwards until a value that must be swapped is found.
            while (Commons::check_bit(tuples[i1].key, current_bit) && i1 > 0) {
                i1--;
            }
            /// Swap if necessary
            if (i0 < i1) {
                std::swap(tuples[i0], tuples[i1]); //                i0++; i1--;
            }
        }

        root->insert(pNode->getStart(), i0, pNode->getStart() + i0, pNum - i0, pNode->getMask(), pNode->getBits());
        if (Commons::check_bit(partition, pNode->getTotalBits() - pNode->getBits() - 1)) {
            n = pNode->getBranch1();
        } else {
            n = pNode->getBranch0();
        }
        //SGX_ASSERT(n != nullptr, "partition should not return a nullptr");
        // keep partitioning if the level of partitioning is very small
        /* if (n->getBits() < 3) { */
        /*     n = partition_both_ends(n, partition); */
        /* } */
        return n;
    }

    /**
     * Checks if t belongs to partition. If yes, adds t to the hash table of the thread.
     * @param threadID
     * @param t
     * @param partition
     * @param ht_counter
     * @param maskP
     */
    void
    check_add_to_ht(int threadID, tuple_t *t, uint32_t partition, uint32_t *ht_counter, uint32_t maskP) {
        uint32_t hk = HASH_BIT_MODULO(t->key, MASK, SHIFT);
        if (hk == partition) {
            // do some j >= numP validation
            vector<tuple_t> &partLocal = partLocals[threadID];
            vector<uint32_t> &nextLocal = nextLocals[threadID];
            vector<uint32_t> &bucketLocal = bucketLocals[threadID];
            tuple_t tt;
            tt.key = t->key;
            tt.payload = t->payload;
            partLocal.at(*ht_counter) = tt;
            uint32_t idx = HASH_BIT_MODULO(t->key, maskP, 0);
            nextLocal.at(*ht_counter) = bucketLocal.at(idx);
            bucketLocal.at(idx) = ++(*ht_counter);
        }
    }

    /**
     * Guess: fuses partitioning and building for the R table in fused CrkJoin.
     * @param threadID
     * @param root
     * @param partition
     * @param maskP
     */
    void
    partition_and_build(int threadID, PTreeNode *root, uint32_t partition, uint32_t maskP) {
        PTreeNode *pNode = root->getSlice(root, partition, (int) NUM_BITS);
        uint64_t pNum = pNode->getNum();
        uint32_t ht_counter = 0;
        tuple_t *tuples = pNode->getStart();
        // stop partitioning when the number of bits reaches the total number of bits
        if (pNode->getBits() >= pNode->getTotalBits() || pNum <= 1) {
            // still build the HT
            for (uint32_t i = 0; i < pNode->getNum(); i++) {
                check_add_to_ht(threadID, &tuples[i], partition, &ht_counter, maskP);
            }
            return;
        }

        uint64_t i0 = 0, i1 = pNum - 1;
        uint32_t current_bit = SHIFT + pNode->getTotalBits() - pNode->getBits() - 1;
        while (i0 < i1) {
            while (!Commons::check_bit(tuples[i0].key, current_bit) && i0 < pNum) {
                check_add_to_ht(threadID, &tuples[i0], partition, &ht_counter, maskP);
                i0++;
            }
            while (Commons::check_bit(tuples[i1].key, current_bit) && i1 > 0) {
                check_add_to_ht(threadID, &tuples[i1], partition, &ht_counter, maskP);
                i1--;
            }
            if (i0 < i1) {
                std::swap(tuples[i0], tuples[i1]); //                i0++; i1--;
            }
        }

        root->insert(pNode->getStart(), i0, pNode->getStart() + i0, pNum - i0, pNode->getMask(), pNode->getBits());
    }

    /**
     * Probes the hash table with t, counts all matches and adds all matches to the output if materialize is enabled
     * @param threadID
     * @param t
     * @param partNum
     * @param maskP
     * @param output
     */
    void
    probe_ht(int threadID, tuple_t *t, uint32_t partNum, uint32_t maskP, output_list_t **output) {
        if (HASH_BIT_MODULO(t->key, MASK, SHIFT) == partNum) {
            uint32_t idx = HASH_BIT_MODULO(t->key, maskP, 0);
            vector<tuple_t> &R = partLocals[threadID];
            vector<uint32_t> &next = nextLocals[threadID];
            vector<uint32_t> &bucket = bucketLocals[threadID];
            size_t bucket_size = bucket.size();
            if (idx > bucket_size - 1) return;
            for (int hit = bucket.at(idx); hit > 0; hit = next.at(hit - 1)) {
                if (t->key == R.at(hit - 1).key) {
                    matches[threadID]++;
                    if (MATERIALIZE) {
                        insert_output(output, t->key, R.at(hit - 1).payload, t->payload);
                    }
                }
            }
        }
    }

    /**
     * Guess: fuses partitioning and probing for the S table in fused CrkJoin.
     * @param threadID
     * @param root
     * @param partition
     * @param maskP
     * @param output
     */
    void
    partition_and_probe(int threadID, PTreeNode *root, uint32_t partition,
                        uint32_t maskP, output_list_t **output) {
        PTreeNode *pNode = root->getSlice(root, partition, (int) NUM_BITS);
        uint64_t pNum = pNode->getNum();

        tuple_t *tuples = pNode->getStart();
        // stop partitioning when the number of bits reaches the total number of bits
        if (pNode->getBits() >= pNode->getTotalBits() || pNum <= 1) {
            // still probe the HT
            for (uint32_t i = 0; i < pNode->getNum(); i++) {
                probe_ht(threadID, &tuples[i], partition, maskP, output);
            }
            return;
        }

        uint64_t i0 = 0, i1 = pNum - 1;
        uint32_t current_bit = SHIFT + pNode->getTotalBits() - pNode->getBits() - 1;
        while (i0 < i1) {
            while (!Commons::check_bit(tuples[i0].key, current_bit) && i0 < pNum) {
                probe_ht(threadID, &tuples[i0], partition, maskP, output);
                i0++;
            }
            while (Commons::check_bit(tuples[i1].key, current_bit) && i1 > 0) {
                probe_ht(threadID, &tuples[i1], partition, maskP, output);
                i1--;
            }
            if (i0 < i1) {
                std::swap(tuples[i0], tuples[i1]); //                i0++; i1--;
            }
        }

        root->insert(pNode->getStart(), i0, pNode->getStart() + i0, pNum - i0, pNode->getMask(), pNode->getBits());

    }


    /**
     * Partitioning only for an extra expriment
     */
    void partition_only() {
        volatile uint64_t dummySum = 0;
        PTreeNode *probeNodeLocal = nullptr;
        uint64_t alg_start, alg_stop;
#ifdef TIMERS
//        uint64_t cyclesPartition=0, cyclesJoin=0;
        uint64_t tmpStart, tmpStop;
#endif
        ocall_get_system_micros(&alg_start);

        for (uint32_t p = 0; p < npart; p++) {
            ocall_get_system_micros(&tmpStart);
            // identify the slice of S to work on
//            probeNodeLocal = nullptr;
            auto slice = pTreeS->getSlice(pTreeS, p, (int) NUM_BITS);
            // partition the slice of S and probe the join candidates
            if (slice->getNum() > 0) {
                probeNodeLocal = partition_both_ends(slice, pTreeS, p);
            }

            dummySum += probeNodeLocal->getNum();
            ocall_get_system_micros(&tmpStop);
            timerPerPartitions[0][p] = (tmpStop - tmpStart);
        }
        ocall_get_system_micros(&alg_stop);
        logger(INFO, "Dummy sum = %lu", dummySum);
    }

    /**
     * Default CrkJoin with 3-phase approach.
     *
     * Phase 1: reorganize S in parallel so it forms independent fragments
     * Phase 2: assign partition ranges to threads
     * Phase 3: each thread independently joins its partitions
     *
     * @param threadID
     * @param threadresult
     * @param crackFlag ignored
     */
    void
    join(const int threadID, threadresult_t *threadresult, bool crackFlag) {
        (void) (crackFlag);
        if (threadID == 0) {
            logger(INFO, "Welcome from the classic CRKJ!");
        }
        uint64_t alg_start, alg_stop, timer_start, timer_stop, partTimerStart, partTimerEnd,
                threadStart, threadEnd;
        ocall_get_system_micros(&threadStart);
        volatile uint64_t dummySum = 0;
#ifdef TIMERS
        uint64_t cyclesPartition = 0, cyclesJoin = 0;
        uint64_t joinBuild = 0, joinPartition = 0, joinProbe = 0, tmpStart, tmpStop;
#endif
        uint32_t maskP;
        tuple_t *tupleR = relR->tuples;
        uint64_t numR = relR->num_tuples;
        uint32_t j;
        PTreeNode *probeNodeLocal, *buildNodeLocal;
        PTreeNode *slice;
        vector<tuple_t> &partLocal = partLocals[threadID];
        vector<uint32_t> &nextLocal = nextLocals[threadID];
        vector<uint32_t> &bucketLocal = bucketLocals[threadID];
//        vector<uint32_t>& timerPerPartition = timerPerPartitions[threadID];
        output_list_t *output;

        if ((nthreads & (nthreads - 1)) != 0) {
            logger(ERROR, "Number of threads must be power of 2");
            ocall_exit(-1);
        }

        barrier->wait();

        if (threadID == 0) {
            ocall_get_system_micros(&alg_start);
#ifdef TIMERS
            timer_start = alg_start;
            ocall_startTimer(&cyclesPartition);
#endif
        }


        if (threadID == 0) {
            logger(DBG, "Build histogram on R");
        }

        uint64_t numPerThread = numR / nthreads;
        uint64_t numThread = (uint64_t) ((threadID == int(nthreads - 1)) ?
                                         (numR - threadID * numPerThread) : numPerThread);
        uint64_t startTuple = threadID * numPerThread;
        for (uint64_t i = 0; i < numThread; ++i) {
            uint32_t hk = HASH_BIT_MODULO(tupleR[startTuple + i].key, MASK, SHIFT);
            localHistR[threadID][hk]++;
        }

        barrier->wait();

        if (threadID == 0) {
            //build the global histogram of R
            for (uint32_t i = 0; i < npart; i++) {
                for (uint32_t k = 0; k < nthreads; k++) {
                    histR[i] += localHistR[k][i];
                }
            }

#ifdef DEBUG //validate the histogram computation
            uint64_t *histRDbg = new uint64_t [npart]();

            for (uint64_t i = 0; i < numR; ++i) {
                uint32_t hk = HASH_BIT_MODULO(tupleR[i].key, MASK, SHIFT);
                histRDbg[hk]++;
            }
            for (uint32_t i = 0; i < npart; i++) {
                //SGX_ASSERT(histR[i] == histRDbg[i], "Histogram not calculated correctly");
            }
            delete [] histRDbg;
            logger(DBG, "histR OK");
#endif
        }

        barrier->wait();

#ifdef TIMERS
        if (threadID == 0) {
            ocall_stopTimer(&cyclesPartition);
            ocall_startTimer(&cyclesJoin);
            ocall_get_system_micros(&timer_stop);
            t_partition_usec = timer_stop - timer_start;
            timer_start = timer_stop;
        }
#endif

        /* Phase 2 - assign partition ranges to threads */
        uint32_t partPerThread = npart / nthreads;
        uint32_t partStart = partPerThread * threadID;
        logger(DBG, "[thread-%d] partStart=%d, partPerThread=%d",
               threadID, partStart, partPerThread);
        uint64_t tmp = pTreeS->getSlice(pTreeS, partStart, (int(NUM_BITS)))->getNum();
        logger(INFO, "[thread-%d] threadTuples : %lu", threadID, tmp);
        timerPerPartitions[threadID].resize(partPerThread);
        std::fill(timerPerPartitions[threadID].begin(), timerPerPartitions[threadID].end(), 0);

        /* Phase 3 - join its own partitions */
        for (uint32_t p = partStart; p < partStart + partPerThread; ++p) {
#ifdef MEASURE_PARTITIONS
            ocall_get_system_micros(&partTimerStart);
#endif
            uint64_t numP = histR[p];
            if (!numP) {
                continue;
            }

#ifdef TIMERS
            if (threadID == 0) {
                ocall_get_system_micros(&tmpStart);
            }
#endif
            // identify the slice of S to work on
            probeNodeLocal = nullptr;
            slice = pTreeS->getSlice(pTreeS, p, (int) NUM_BITS);
            // partition the slice of S and probe the join candidates
            if (slice->getNum() > 0) {
                probeNodeLocal = partition_both_ends(slice, pTreeS, p);
#ifdef COUNT_SCANNED_TUPLES
                scanned_tupels += slice->getNum();
                pthread_mutex_lock(&lock);
                uint32_t sliceBits = slice->getBits();
                if ( !bitMap.insert( std::make_pair( sliceBits, 1 ) ).second ) {
                    bitMap[sliceBits]++;
                }
                pthread_mutex_unlock(&lock);
#endif
            }

            if (probeNodeLocal == nullptr) {
                continue;
            }

#ifdef TIMERS
            if (threadID == 0) {
                ocall_get_system_micros(&tmpStop);
                joinPartition += (tmpStop - tmpStart);
                tmpStart = tmpStop;
            }
#endif

            uint64_t NP = Commons::nextPowerOfTwo(numP + 1);
            partLocal.resize(numP);
            nextLocal.resize(numP);
            std::fill(nextLocal.begin(), nextLocal.end(), 0);
            bucketLocal.resize(NP == 1 ? 2 : NP);
            std::fill(bucketLocal.begin(), bucketLocal.end(), 0);
            maskP = (uint32_t) (NP - 1);

            //identify the slice of R to work on
            buildNodeLocal = nullptr;
            slice = pTreeR->getSlice(pTreeR, p, (int) NUM_BITS);
            if (slice->getNum() > 0) {
                buildNodeLocal = partition_both_ends(slice, pTreeR, p);
            }
            if (buildNodeLocal == nullptr) {
                continue;
            }
            j = 0;
            //build the hash table
            for (uint64_t i = 0; i < buildNodeLocal->getNum(); i++) {
                type_key key = buildNodeLocal->getStart()[i].key;
                uint32_t hk = HASH_BIT_MODULO(key, MASK, SHIFT);
                if (hk == p) {
//                    logger(DBG, "HT add key %u", tupleR[i].key);
                    if (j >= numP) {
                        logger(ERROR, "[thread-%d] j >= numP %d > %d", threadID, j, numP);
                        ocall_exit(-1);
                    }
                    partLocal.at(j) = buildNodeLocal->getStart()[i];
                    uint32_t idx = HASH_BIT_MODULO(key, maskP, 0);
                    nextLocal.at(j) = bucketLocal.at(idx);
                    bucketLocal.at(idx) = ++j;
                }
            }

#ifdef TIMERS
            if (threadID == 0) {
                ocall_get_system_micros(&tmpStop);
                joinBuild += (tmpStop - tmpStart);
                tmpStart = tmpStop;
            }
#endif
//
//            //probe the hash table
            probe_node(threadID, probeNodeLocal->getStart(), probeNodeLocal->getNum(),
                       bucketLocal, nextLocal, partLocal, p, maskP, &output);
//
#ifdef TIMERS
            if (threadID == 0) {
                ocall_get_system_micros(&tmpStop);
                joinProbe += (tmpStop - tmpStart);
            }
#endif
#ifdef MEASURE_PARTITIONS
            ocall_get_system_micros(&partTimerEnd);
            dummySum += probeNodeLocal->getNum();
            timerPerPartitions[threadID].at(p-partStart) = (partTimerEnd-partTimerStart);
#endif
        }
        if (MATERIALIZE) {
            threadresult->results = output;
            threadresult->nresults = (int64_t) matches[threadID];
            threadresult->threadid = (uint32_t) threadID;
        }

        if (threadID == 0) {
            ocall_get_system_micros(&alg_stop);
            t_usec = alg_stop - alg_start;
#ifdef TIMERS
            ocall_stopTimer(&cyclesJoin);
            ocall_get_system_micros(&timer_stop);
            t_join_usec = timer_stop - timer_start;
            t_join_build = joinBuild;
            t_join_partition = joinPartition;
            t_join_probe = joinProbe;
#endif
        }
        ocall_get_system_micros(&threadEnd);
        logger(INFO, "[thread-%d] Micros : %lu",
               threadID, (threadEnd - threadStart));
    }

    /**
     * This may be the "naive baseline" they mentioned in the paper. That's why it is also called simple.
     * @param threadID
     * @param threadresult
     * @param crackFlag In the multi-threaded case, partitioning by cracking is done before. Therefore, this value is
     * false. In the single-threaded case, partitioning with cracking is done inside this function. Threrfore, the value
     * is true.
     */
    void
    join_simple_dfs_st(const int threadID, threadresult_t *threadresult, bool crackFlag) {
        if (threadID == 0) {
            logger(WARN, "Welcome from CRKJS!");
        }
        uint64_t alg_start, alg_stop, timer_start, timer_stop;
#ifdef TIMERS
        uint64_t cyclesPartition = 0, cyclesJoin = 0;
        uint64_t joinBuild = 0, joinPartition = 0, joinProbe = 0, tmpStart = 0, tmpStop = 0;
#endif
        uint32_t maskP;
        tuple_t *tupleR = relR->tuples;
        uint64_t numR = relR->num_tuples;
        PTreeNode *probeNodeLocal, *buildNodeLocal;
        std::vector<tuple_t> partLocal;
        std::vector<uint32_t> nextLocal, bucketLocal;
        output_list_t *output;

        if ((nthreads & (nthreads - 1)) != 0) {
            logger(ERROR, "Number of threads must be power of 2");
            ocall_exit(-1);
        }

        barrier->wait();
        if (threadID == 0) {
            ocall_get_system_micros(&alg_start);
#ifdef TIMERS
            timer_start = alg_start;
            ocall_startTimer(&cyclesPartition);
#endif
        }

        if (threadID == 0) {
            logger(DBG, "Build histogram on R");
        }

        uint64_t numPerThread = numR / nthreads;
        uint64_t numThread = (uint64_t) ((threadID == int(nthreads - 1)) ?
                                         (numR - threadID * numPerThread) : numPerThread);
        uint64_t startTuple = threadID * numPerThread;
        for (uint64_t i = 0; i < numThread; ++i) {
            uint32_t hk = HASH_BIT_MODULO(tupleR[startTuple + i].key, MASK, SHIFT);
            localHistR[threadID][hk]++;
        }

        barrier->wait();

        if (threadID == 0) {
            //build the global histogram of R
            for (uint32_t i = 0; i < npart; i++) {
                for (uint32_t k = 0; k < nthreads; k++) {
                    histR[i] += localHistR[k][i];
                }
            }

#ifdef DEBUG //validate the histogram computation
            uint64_t *histRDbg = new uint64_t [npart]();

            for (uint64_t i = 0; i < numR; ++i) {
                uint32_t hk = HASH_BIT_MODULO(tupleR[i].key, MASK, SHIFT);
                histRDbg[hk]++;
            }
            for (uint32_t i = 0; i < npart; i++) {
                //SGX_ASSERT(histR[i] == histRDbg[i], "Histogram not calculated correctly");
            }
            delete [] histRDbg;
            logger(DBG, "histR OK");
#endif
        }


        barrier->wait();

        // crackFlag is true if single threaded
        if (crackFlag) {
            // 1. partition R
            crack_dfs(pTreeR, pTreeR);
            // 2. partition S
            crack_dfs(pTreeS, pTreeS);
        }

        // 3. join partitions
        /// This is an implementation of the bucket chaining join with some C++ data structures and without the work
        /// queue. Each thread gets assigned a number of partitions.
        uint32_t partPerThread = npart / nthreads;
        uint32_t partStart = partPerThread * threadID;
        logger(DBG, "[thread-%d] partStart=%d, partPerThread=%d",
               threadID, partStart, partPerThread);
        for (uint32_t p = partStart; p < partStart + partPerThread; p++) {
            uint64_t numP = histR[p];
            if (!numP) {
                continue;
            }
            probeNodeLocal = pTreeS->getSlice(pTreeS, p, (int) NUM_BITS);
            if (probeNodeLocal == nullptr) {
                continue;
            }
            uint64_t NP = Commons::nextPowerOfTwo(numP + 1);
            partLocal.resize(numP);
            nextLocal.resize(numP);
            std::fill(nextLocal.begin(), nextLocal.end(), 0);
            bucketLocal.resize(NP == 1 ? 2 : NP);
            std::fill(bucketLocal.begin(), bucketLocal.end(), 0);
            maskP = (uint32_t) (NP - 1);
            buildNodeLocal = pTreeR->getSlice(pTreeR, p, (int) NUM_BITS);
            if (buildNodeLocal == nullptr) {
                continue;
            }
            uint32_t j = 0;
            //build the hash table
            logger(DBG, "[thread-%d] Build hash table with %d elements (should be %d)",
                   threadID, buildNodeLocal->getNum(), numP);
            for (uint64_t i = 0; i < buildNodeLocal->getNum(); i++) {
                type_key key = buildNodeLocal->getStart()[i].key;
//                    logger(DBG, "HT add key %u", tupleR[i].key);
                if (j >= numP) {
                    logger(ERROR, "[thread-%d] j >= numP: %d >= %d. buildNodeLocal size: %d",
                           threadID, j, numP, buildNodeLocal->getNum());
                    ocall_exit(-1);
                }
                partLocal.at(j) = buildNodeLocal->getStart()[i];
                uint32_t idx = HASH_BIT_MODULO(key, maskP, 0);
                nextLocal.at(j) = bucketLocal.at(idx);
                bucketLocal.at(idx) = ++j;
            }
#ifdef TIMERS
            if (threadID == 0) {
                ocall_get_system_micros(&tmpStop);
                joinBuild += (tmpStop - tmpStart);
                tmpStart = tmpStop;
            }
#endif
            // probe the hash table
//            probe_node(threadID, probeNodeLocal->getStart(), probeNodeLocal->getNum(),
//                       bucketLocal, nextLocal, partLocal, p, maskP, &output);
            size_t bucket_size = bucketLocal.size();
            for (uint64_t i = 0; i < probeNodeLocal->getNum(); i++) {
                tuple_t t = probeNodeLocal->getStart()[i];
                uint32_t idx = HASH_BIT_MODULO(t.key, maskP, 0);
                if (idx > bucket_size - 1) continue;
                for (int hit = bucketLocal.at(idx); hit > 0; hit = nextLocal.at(hit - 1)) {
                    if (t.key == partLocal.at(hit - 1).key) {
                        matches[threadID]++;
                        if (MATERIALIZE) {
                            insert_output(&output, t.key, partLocal.at(hit - 1).payload, t.payload);
                        }
                    }
                }
            }
#ifdef TIMERS
            if (threadID == 0) {
                ocall_get_system_micros(&tmpStop);
                joinProbe += (tmpStop - tmpStart);
            }
#endif
        }
        if (MATERIALIZE) {
            threadresult->results = output;
            threadresult->nresults = (int64_t) matches[threadID];
            threadresult->threadid = (uint32_t) threadID;
        }

        if (threadID == 0) {
            ocall_get_system_micros(&alg_stop);
            t_usec = alg_stop - alg_start;
#ifdef TIMERS
            ocall_stopTimer(&cyclesJoin);
            ocall_get_system_micros(&timer_stop);
            t_join_usec = timer_stop - timer_start;
            t_join_build = joinBuild;
            t_join_partition = joinPartition;
            t_join_probe = joinProbe;
#endif
        }
    }

    /**
     * This join fusions cracking and joining phases into one
     * @param threadID
     * @param threadresult
     * @param crackFlag ignored.
     */
    void
    joinFusion(const int threadID, threadresult_t *threadresult, bool crackFlag) {
        (void) (crackFlag);
        if (threadID == 0) {
            logger(INFO, "Welcome from CRKJF!");
        }
        uint64_t alg_start, alg_stop, timer_start, timer_stop;
#ifdef TIMERS
        uint64_t cyclesPartition = 0;
        uint64_t joinPartition = 0;//, tmpStart, tmpStop;
#endif
        uint32_t maskP;
        tuple_t *tupleR = relR->tuples;
        uint64_t numR = relR->num_tuples;
        vector<tuple_t> &partLocal = partLocals[threadID];
        vector<uint32_t> &nextLocal = nextLocals[threadID];
        vector<uint32_t> &bucketLocal = bucketLocals[threadID];
        output_list_t *output;

        if ((nthreads & (nthreads - 1)) != 0) {
            logger(ERROR, "Number of threads must be power of 2");
            ocall_exit(-1);
        }

        barrier->wait();

        if (threadID == 0) {
            ocall_get_system_micros(&alg_start);
#ifdef TIMERS
            timer_start = alg_start;
            ocall_startTimer(&cyclesPartition);
#endif
        }


        if (threadID == 0) {
            logger(DBG, "Build histogram on R");
        }

        uint64_t numPerThread = numR / nthreads;
        uint64_t numThread = (uint64_t) ((threadID == int(nthreads - 1)) ?
                                         (numR - threadID * numPerThread) : numPerThread);
        uint64_t startTuple = threadID * numPerThread;
        for (uint64_t i = 0; i < numThread; ++i) {
            uint32_t hk = HASH_BIT_MODULO(tupleR[startTuple + i].key, MASK, SHIFT);
            localHistR[threadID][hk]++;
        }

        barrier->wait();

        if (threadID == 0) {
            //build the global histogram of R
            for (uint32_t i = 0; i < npart; i++) {
                for (uint32_t k = 0; k < nthreads; k++) {
                    histR[i] += localHistR[k][i];
                }
            }

#ifdef DEBUG //validate the histogram computation
            uint64_t *histRDbg = new uint64_t [npart]();

            for (uint64_t i = 0; i < numR; ++i) {
                uint32_t hk = HASH_BIT_MODULO(tupleR[i].key, MASK, SHIFT);
                histRDbg[hk]++;
            }
            for (uint32_t i = 0; i < npart; i++) {
                //SGX_ASSERT(histR[i] == histRDbg[i], "Histogram not calculated correctly");
            }
            delete [] histRDbg;
            logger(DBG, "histR OK");
#endif
        }

        barrier->wait();

#ifdef TIMERS
        if (threadID == 0) {
            ocall_stopTimer(&cyclesPartition);
            ocall_get_system_micros(&timer_stop);
            t_partition_usec = timer_stop - timer_start;
            timer_start = timer_stop;
        }
#endif

        /* Phase 2 - assign partition ranges to threads */
        uint32_t partPerThread = npart / nthreads;
        uint32_t partStart = partPerThread * threadID;
        logger(DBG, "[thread-%d] partStart=%d, partPerThread=%d",
               threadID, partStart, partPerThread);

        /* Phase 3 - join its own partitions */
        for (uint32_t p = partStart; p < partStart + partPerThread; ++p) {
            uint64_t numP = histR[p];
            if (!numP) {
                continue;
            }

#ifdef TIMERS
//            if (threadID == 0) {
//                ocall_get_system_micros(&tmpStart);
//            }
#endif
            // 1. crack R and build HT at the same time
            uint64_t NP = Commons::nextPowerOfTwo(numP + 1);
            partLocal.clear();
            partLocal.resize(numP);
            nextLocal.resize(numP);
            std::fill(nextLocal.begin(), nextLocal.end(), 0);
            bucketLocal.resize(NP == 1 ? 2 : NP);
            std::fill(bucketLocal.begin(), bucketLocal.end(), 0);
            maskP = (uint32_t) (NP - 1);
            partition_and_build(threadID, pTreeR, p, maskP);

            // 2. partition the slice of S and probe the join candidates
            partition_and_probe(threadID, pTreeS, p, maskP, &output);

#ifdef TIMERS
//            if (threadID == 0) {
//                ocall_get_system_micros(&tmpStop);
//                joinPartition += (tmpStop - tmpStart);
//                tmpStart = tmpStop;
//            }
#endif


        }

        if (MATERIALIZE) {
            threadresult->results = output;
            threadresult->nresults = (int64_t) matches[threadID];
            threadresult->threadid = (uint32_t) threadID;
        }

        if (threadID == 0) {
            ocall_get_system_micros(&alg_stop);
            t_usec = alg_stop - alg_start;
#ifdef TIMERS
            ocall_get_system_micros(&timer_stop);
            t_join_usec = timer_stop - timer_start;
            t_join_partition = joinPartition;
#endif
        }
    }

    join_result_t getJoinResult() {
        uint64_t m = 0, t_ht = 0, t_tree = 0, t_part = 0, t_probe = 0;
        for (uint32_t i = 0; i < nthreads; ++i) {
            m += matches[i];
            t_ht += timer_ht[i];
            t_tree += timer_tree[i];
            t_part += timer_partition[i];
            t_probe += timer_probe[i];
        }

        join_result_t result;
        result.matches = m;
        result.time_usec = t_usec;
        result.part_usec = t_partition_usec;
        result.join_usec = t_join_usec;
        result.join_build_usec = t_join_build;
        result.join_partition_usec = t_join_partition;
        result.join_probe_usec = t_join_probe;
        result.timer_ht = t_ht;
        result.timer_tree = t_tree;
        result.timer_part = t_part;
        result.timer_probe = t_probe;

#ifdef MEASURE_PARTITIONS
        result.timer_per_partition = new uint64_t*[nthreads];
        for (int i = 0; i < nthreads; i++) {
            result.timer_per_partition[i] = new uint64_t[(npart/nthreads)];
            std::copy(timerPerPartitions[i].begin(), timerPerPartitions[i].end(), result.timer_per_partition[i]);
        }
//        memcpy(result.timer_per_partition, timerPerPartition, npart * sizeof(uint64_t));
#endif
#ifdef COUNT_SCANNED_TUPLES
        result.scanned_tuples = scanned_tupels;
#endif
        return result;
    }

    PTreeNode *getTreeRootR() {
        return pTreeR;
    }

    PTreeNode *getTreeRootS() {
        return pTreeS;
    }

    virtual ~Join() {
        delete pTreeS;
        delete pTreeR;
        delete[] histR;
        delete[] histS;
        delete[] matches;
        delete[] timer_ht;
        delete[] timer_tree;
        delete[] timer_partition;
        delete[] timer_probe;
        delete[] timerPerPartitions;
        delete[] timerJoin;

        delete[] partLocals;
        delete[] nextLocals;
        delete[] bucketLocals;

        delete barrier;
#ifdef COUNT_SCANNED_TUPLES
        pthread_mutex_destroy(&lock);
#endif

        for (uint32_t i = 0; i < nthreads; i++) {
            delete[] localHistR[i];
        }
        delete[] localHistR;
    }

#ifdef COUNT_SCANNED_TUPLES
    void logBitMap()
    {
        pthread_mutex_lock(&lock);
        logger(INFO, "***** BIT MAP *****");
        for (auto const &pair: bitMap) {
            logger(INFO, "%2d -> %d", pair.first, pair.second);
        }
        logger(INFO, "***** BIT MAP *****");
        pthread_mutex_unlock(&lock);
    }
#endif
};

#endif //JOIN_HPP
