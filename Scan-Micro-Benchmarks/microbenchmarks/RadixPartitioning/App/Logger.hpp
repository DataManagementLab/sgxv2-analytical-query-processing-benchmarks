#ifndef SGX_ENCRYPTEDDB_LOGGER_HPP
#define SGX_ENCRYPTEDDB_LOGGER_HPP

#include <string>

class Logger {
public:
    bool debug {false};

    Logger() = default;
    explicit Logger(bool _debug);

    void log(const std::string &str, bool error = false) const;

    void err(const std::string &str) const;

};

void logger(const std::string &str, bool error = false);

void error(const std::string &str);

#endif //SGX_ENCRYPTEDDB_LOGGER_HPP
