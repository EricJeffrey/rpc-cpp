#if !defined(UTILS_CPP)
#define UTILS_CPP

#include "utils.h"

int writen(int sd, char *buf, ssize_t numToWrite) {
    ssize_t byteNumWrite = 0, ret;
    while (byteNumWrite < numToWrite) {
        ret = write(sd, buf + byteNumWrite, numToWrite - byteNumWrite);
        const int tmpErrno = errno;
        if (ret == -1) {
            loggerInstance()->error({"write failed", strerror(tmpErrno)});
            if (tmpErrno == EBADFD)
                throw SocketError("call to write failed, EBADFD");
            throw runtime_error("call to write failed");
        } else {
            if (tmpErrno == EINTR)
                loggerInstance()->debug({"write interrputed by signal"});
            byteNumWrite += ret;
        }
    }
    return 0;
}
int readn(int sd, char *buf, ssize_t numToRead) {
    ssize_t byteNumRead = 0, ret;
    while (byteNumRead < numToRead) {
        ret = read(sd, buf + byteNumRead, numToRead - byteNumRead);
        const int tmpErrno = errno;
        if (ret == -1) {
            loggerInstance()->error({"read failed", strerror(tmpErrno)});
            if (tmpErrno == EBADFD)
                throw SocketError("call to write failed, EBADFD");
            throw runtime_error("call to read failed");
        } else if (ret > 0) {
            if (tmpErrno == EINTR)
                loggerInstance()->debug({"read interrupted by signal"});
            byteNumRead += ret;
        }
    }
    return 0;
}
json parse2json(string s) {
    loggerInstance()->debug({"parsing s:|", s, "|to json: "});
    return json::parse(s);
}
string serialize2str(json j) {
    loggerInstance()->debug({"stringify json to str"});
    return j.dump();
}

#endif // UTILS_CPP
