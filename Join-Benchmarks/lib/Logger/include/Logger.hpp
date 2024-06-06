#ifndef SGXJOINEVALUATION_LOGGER_H
#define SGXJOINEVALUATION_LOGGER_H

#include "LoggerTypes.hpp"
#include <optional>
#include <cstdint>

void initLogger(std::optional<uint64_t> start = std::nullopt);

uint64_t logger_get_start();

void logger(LEVEL level, const char *fmt, ...);

#endif //SGXJOINEVALUATION_LOGGER_H
