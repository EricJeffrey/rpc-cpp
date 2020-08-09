#if !defined(UTILS_H)
#define UTILS_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <exception>
#include "logger.h"
#include "json.hpp"

using nlohmann::json;
using std::runtime_error;
using std::string;

// error that current socket is not valid
class SocketError : runtime_error {
public:
    explicit SocketError(const string &s) : runtime_error(s) {}
    const char *what() const noexcept { return runtime_error::what(); }
};

json parse2json(string req);
string serialize2str(json resp);

// read numToRead bytes from sd and store into buf
// throw error when read return -1
int readn(int sd, char *buf, ssize_t numToRead);

// write numToRead bytes to sd from buf
// throw error when write return -1
int writen(int sd, const char *buf, ssize_t numToWrite);

// convert int to 4 byte str, bigend - 3:00 00 00 03
string int2str(int x);
// convert 4 byte str to int, bigend - 00 00 00 03: 3
int str2int(const string &str);
// convert 4 byte str to int, bigend - 00 00 00 03: 3
int str2int(const char *str);

#endif // UTILS_H
