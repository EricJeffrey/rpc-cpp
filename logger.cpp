#if !defined(LOGGER_CPP)
#define LOGGER_CPP

#include "logger.h"

string curTime() {
    time_t t = time(NULL);
    struct tm ttm = *localtime(&t);
    char buf[22] = {};
    snprintf(buf, 22, "%d-%02d-%02d %02d:%02d:%02d", ttm.tm_year + 1900, ttm.tm_mon + 1,
             ttm.tm_mday, ttm.tm_hour, ttm.tm_min, ttm.tm_sec);
    return buf;
}

shared_ptr<Logger> loggerInstance() { return Logger::getInstance(); }
shared_ptr<Logger> Logger::logInstancePtr = nullptr;

#endif // LOGGER_CPP
