#ifndef SGX_ENCRYPTEDDB_ENCLAVEINIT_HPP
#define SGX_ENCRYPTEDDB_ENCLAVEINIT_HPP


#include "sgx_error.h"
#include "Enclave_u.h"
#include "Logger.hpp"

extern Logger l;

int initialize_enclave(const char *enclave_file, sgx_enclave_id_t &enclave_id) {
    sgx_status_t ret;

    l.log("Initializing enclave.");

    /* Call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    ret = sgx_create_enclave(enclave_file, 1, nullptr, nullptr, &enclave_id, nullptr);
    if (ret != SGX_SUCCESS) {
        l.err("Enclave initialization failed!");
        print_error_message(ret);
        return -1;
    }
    ret = ecall_noop(enclave_id);
    if (ret != SGX_SUCCESS) {
        l.err("First call into enclave failed!");
        print_error_message(ret);
        return -1;
    }
    l.log("Done.");
    return 0;
}

int destroy_enclave(sgx_enclave_id_t enclave_id) {
    sgx_status_t ret;

    l.log("Destroying enclave.");

    /* Call sgx_destroy_enclave to destroy  the enclave instance */
    ret = sgx_destroy_enclave(enclave_id);
    if (ret != SGX_SUCCESS) {
        l.err("Destroying the enclave failed!");
        print_error_message(ret);
        return -1;
    }
    l.log("Done.");
    return 0;
}

#endif//SGX_ENCRYPTEDDB_ENCLAVEINIT_HPP
