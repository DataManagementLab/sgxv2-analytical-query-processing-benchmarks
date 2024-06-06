#include <vector>
#include <atomic>
#include <iostream>
#include <memory>

#include "data-types.h"
#include "CHT.hpp"
#include "Barrier.hpp"
#include "rdtscpWrapper.h"

#include "Logger.hpp"
#include <cstring> //memset

class CHTPartitionQueue{
private:
    std::atomic<int> counter {0};
    int nPart;
public:
    explicit CHTPartitionQueue(int _nPart): nPart(_nPart) {}

    int getPartition() {
        return counter++;
    }

    bool empty() {
        return counter.load() == nPart;
    }
};


const unsigned int TUPLES_PER_CACHELINE= 64 / sizeof(tuple_t);

typedef struct {
    tuple_t tuples[TUPLES_PER_CACHELINE - 1];
    uint64_t target;
} cht_cacheline_t;

#define STREAM_UNIT 4

class CHTJoin {
private:
    int nthreads;
    int npart;
    const table_t *relR;
    const table_t *relS;
    tuple_t *partBuffer;
    std::vector<std::unique_ptr<uint64_t[]>> hist;
    std::vector<std::unique_ptr<uint64_t[]>> dst;
    CHTPartitionQueue partitions;
    CHT ht;
    const intkey_t MASK;
    const intkey_t SHIFT;
    std::vector<uint64_t> matches;
    std::vector<uint64_t> checksums;
    uint64_t time_usec {0};
    uint64_t partitionTime {0};
    timers_t timers {};
    Barrier barrier;

    void radix_partition(const tuple_t *input,
                         tuple_t *output,
                         uint64_t *histogram,
                         size_t numTuples) {

        __attribute__((aligned(64))) cht_cacheline_t buffers[npart];

        // we have to make sure that the size of each partitioning in terms of elements is a multiple of 4 (since 4 tuples fit into 32 bytes = 256 bits).
        for(int i = 0; i < npart; ++i){
            buffers[i].target = histogram[i];
        }

        __attribute__((aligned(64))) uint64_t bucket_num = 0;
        for(uint64_t j = 0; j < numTuples; ++j){
            bucket_num = (input[j].key >> SHIFT) & MASK;
            int slot = buffers[bucket_num].target & (TUPLES_PER_CACHELINE - 1);
            if(slot == TUPLES_PER_CACHELINE - 1) {
                uint64_t targetBkp=buffers[bucket_num].target- (TUPLES_PER_CACHELINE-1);
                buffers[bucket_num].tuples[slot]=input[j];
                for(uint32_t b = 0; b < TUPLES_PER_CACHELINE; b += STREAM_UNIT) {
//                    _mm256_stream_si256(reinterpret_cast<__m256i*>(output + targetBkp), _mm256_load_si256((reinterpret_cast<__m256i*>(buffers[bucket_num].tuples + b))));
                    memcpy(output + targetBkp, buffers[bucket_num].tuples + b, 32);
                    targetBkp += STREAM_UNIT;
                }
                buffers[bucket_num].target=targetBkp;
            } else {
                buffers[bucket_num].tuples[slot] = input[j];
                buffers[bucket_num].target++;
            }
        }

        barrier.wait();

        for (int i = npart - 1; i >= 0; i--) {
            uint32_t slot  = (uint32_t)buffers[i].target;
            uint32_t sz    = (slot) & (TUPLES_PER_CACHELINE - 1);
            slot          -= sz;
            uint32_t startPos = (slot < histogram[i]) ? ((uint32_t)histogram[i] - slot) : 0;
            for(uint32_t j = startPos; j < sz; j++) {
                output[slot+j]  = buffers[i].tuples[j];
            }
        }
    }

    void partition(int threadID, const table_t chunkR)
    {
        const tuple_t * tupleR = chunkR.tuples;
        uint64_t * my_hist     = hist[threadID].get();
        uint64_t * my_dst      = dst[threadID].get();
        uint64_t sum           = 0;

        for (size_t i = 0; i < chunkR.num_tuples; ++i) {
            intkey_t hk = (hashKey(tupleR[i].key) >> SHIFT) & MASK;
            my_hist[hk]++;
        }

        for (int i = 0; i < npart; ++i) {
            sum += my_hist[i];
            my_hist[i] = sum;
        }

        barrier.wait();

        for (int i = 0; i < threadID; ++i) {
            for (int j = 0; j < npart; ++j) {
                my_dst[j] += hist[i][j];
            }
        }

        for (int i = threadID; i < nthreads; ++i) {
            for (int j = 1; j < npart; ++j) {
                my_dst[j] += hist[i][j-1];
            }
        }

        radix_partition(chunkR.tuples, partBuffer, my_dst, chunkR.num_tuples);
    }

    void build(int threadID, int part)
    {
        (void) (threadID);
        tuple_t *tuples = partBuffer + dst[0][part];
        const uint64_t num_tuples = part == npart - 1 ? relR->num_tuples - dst[0][part] : dst[0][part + 1] - dst[0][part];

        for (uint64_t i = 0; i < num_tuples; ++i) {
            ht.setBit(tuples[i].key);
		}

        ht.computePartPopCount(part, (uint32_t)dst[0][part]);

        for (uint64_t i = 0; i < num_tuples; ++i)
            ht.setTuple(tuples[i]);
    }

    void probe(int threadID, const table_t chunkS)
    {
        uint64_t match = 0;
        uint64_t checksum = 0;
 		tuple_t *tupleS = chunkS.tuples;
		const size_t numS = chunkS.num_tuples;
		const size_t batchStartUpperBound = numS - PROBE_BATCH_SIZE;
        for (size_t i = 0; i <= batchStartUpperBound; i += PROBE_BATCH_SIZE)
		{
			ht.batch_probe(tupleS + i, match, checksum);
		}

		const size_t leftOver = numS % PROBE_BATCH_SIZE;
		tupleS += numS - leftOver;
        for (size_t i = 0; i < leftOver; ++i)
        {
            tuple_t *foundTuple = ht.probe(tupleS[i].key);
            if (foundTuple)
            {
                match++;
                checksum += foundTuple->payload + tupleS[i].payload;
            }
        }
        matches[threadID] = match;
        checksums[threadID] = checksum;
    }

public:

    CHTJoin(int _nthreads, int _npart, const table_t *_relR, const table_t *_relS, tuple_t * _partBuffer) : nthreads(_nthreads),
        npart(_npart), relR(_relR) , relS(_relS), partBuffer(_partBuffer),
        hist(nthreads), dst(nthreads),
        partitions(npart), ht(relR->num_tuples, nthreads, npart),
        MASK(npart-1), SHIFT(__builtin_ctz(Utils::nextPowerOfTwo(relR->num_tuples)) - __builtin_ctz(npart)),
        matches(nthreads), checksums(nthreads), barrier{static_cast<size_t>(nthreads)}
    {}

    void join(int threadID) {
        ht.init(threadID);

        hist[threadID] = std::make_unique<uint64_t []>(npart);
        dst[threadID] = std::make_unique<uint64_t []>(npart);

        barrier.wait(
                [this]() {
                    ocall_get_system_micros(&timers.start);
                    auto current_time = rdtscp_s();
                    timers.total = current_time;
                    timers.partition = current_time;
                    return true;
                });

        // partition phase
        uint64_t numRPerThread = relR->num_tuples / nthreads;
        table_t myChunkR;
        myChunkR.tuples = relR->tuples + numRPerThread * threadID;
        myChunkR.num_tuples = threadID == nthreads - 1 ? (uint32_t)(relR->num_tuples - numRPerThread * threadID) : (uint32_t)numRPerThread;

        partition(threadID, myChunkR);

        barrier.wait(
                [this]() {
                    auto current_time = rdtscp_s();
                    timers.partition = current_time - timers.partition;
                    timers.build = current_time;
                    partitionTime=timers.end - timers.start;
                    return true;
                }
                );

        // build phase
        int partID;
        while ((partID = partitions.getPartition()) < npart)
        {
            build(threadID, partID);
        }

        barrier.wait(
                [this]() {
                    auto current_time = rdtscp_s();
                    timers.build = current_time - timers.build;
                    timers.probe = rdtscp_s();
                    return true;
                });

        // probe phase
        uint64_t numSperThread = relS->num_tuples / nthreads;
        table_t myChunkS;
        myChunkS.tuples = relS->tuples + numSperThread * threadID;
        myChunkS.num_tuples = threadID == nthreads - 1 ? (uint32_t)(relS->num_tuples - numSperThread * threadID) : (uint32_t)numSperThread;
        probe(threadID, myChunkS);

        barrier.wait(
                [this]() {
                    auto current_time = rdtscp_s();
                    ocall_get_system_micros(&timers.end);
                    timers.probe = current_time - timers.probe;
                    timers.total = current_time - timers.total;
                    time_usec = timers.end - timers.start;
                    return true;
                });
    }

    join_result_t get_join_result()
    {
        uint64_t m = 0;
        uint64_t c = 0;
        for (int i = 0; i < nthreads; ++i) {
            m += matches[i];
            c += checksums[i];
        }
        join_result_t result;
        result.time_usec = time_usec;
        result.part_usec=partitionTime;
        result.join_usec=time_usec-partitionTime;
        result.matches = m;
        result.checksum = c;
        return result;
    }

    struct timers_t get_timers()
    {
        return timers;
    }
};
