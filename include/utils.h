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

json parse2json(string req);
string serialize2str(json resp);

// read numToRead bytes from sd and store into buf
// throw error when read return -1
int readn(int sd, char *buf, ssize_t numToRead);

// write numToRead bytes to sd from buf
// throw error when write return -1
int writen(int sd, char *buf, ssize_t numToWrite);

#endif // UTILS_H
