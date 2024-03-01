#ifndef SGX_ENCRYPTEDDB_LOGGER_HPP
#define SGX_ENCRYPTEDDB_LOGGER_HPP

#include <string>

void logger(const std::string &str, bool error = false);

void error(const std::string &str);

#endif //SGX_ENCRYPTEDDB_LOGGER_HPP
