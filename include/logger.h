#if !defined(LOGGER_H)
#define LOGGER_H

#include <initializer_list>
#include <cstring>
#include <ctime>
#include <string>
#include <iostream>
#include <memory>

using std::cerr;
using std::cout;
using std::endl;
using std::initializer_list;
using std::make_shared;
using std::shared_ptr;
using std::string;

string curTime();

#define CUR_TIME (curTime())

enum LoggerLevel {
    NONE = -1,
    ALL = 5,
    INFO = 1,
    DEBUG_INFO = 2,
    WARN_DEBUG_INFO = 3,
    ERROR_WARN_DEBUG_INFO = 4
};

class Logger {
private:
    static shared_ptr<Logger> logInstancePtr;
    LoggerLevel logLevel;
    Logger() { logLevel = ALL; };

public:
    ~Logger() {}

    shared_ptr<Logger> static getInstance() {
        if (!logInstancePtr) {
            logInstancePtr = make_shared<Logger>(Logger());
        }
        return logInstancePtr;
    }

    void setLogLevel(LoggerLevel level) { logLevel = level; }

    void info(initializer_list<string> list) {
        if (logLevel >= DEBUG_INFO) {
            cerr << CUR_TIME << " INFO ";
            for (auto &&s : list)
                cerr << s << ", ";
            cerr << endl;
        }
    }

    void debug(initializer_list<string> list) {
        if (logLevel >= DEBUG_INFO) {
            cerr << CUR_TIME << " DEBUG ";
            for (auto &&s : list)
                cerr << s << ", ";
            cerr << endl;
        }
    }

    void warn(initializer_list<string> list) {
        if (logLevel >= WARN_DEBUG_INFO) {
            cerr << CUR_TIME << " WARN ";
            for (auto &&s : list)
                cerr << s << ", ";
            cerr << endl;
        }
    }

    void error(initializer_list<string> list) {
        if (logLevel >= ERROR_WARN_DEBUG_INFO) {
            cerr << CUR_TIME << " ERROR ";
            for (auto &&s : list)
                cerr << s << " ";
            cerr << endl;
        }
    }
};

shared_ptr<Logger> loggerInstance();

#endif // LOGGER_H
