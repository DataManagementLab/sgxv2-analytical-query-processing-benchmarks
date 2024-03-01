#ifndef SGX_ENCRYPTEDDB_MULTIREADCONFIG_H
#define SGX_ENCRYPTEDDB_MULTIREADCONFIG_H

/**
 * Describes how often the same data is read. If unique is set to true, data is only read once.
 * If unique is set to false, num_warmup_runs are done to get the data into cache if possible.
 * Afterwards the same data is read num_runs times.
 */
struct MultiReadConfiguration {
    int unique;
    uint8_t num_warmup_runs;
    uint32_t num_runs;
};

#endif //SGX_ENCRYPTEDDB_MULTIREADCONFIG_H
