#if !defined(RPC_SERVER_H)
#define RPC_SERVER_H

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <exception>
#include <stdexcept>
#include <error.h>
#include <vector>
#include <unordered_map>
#include <functional>

#include "json.hpp"
#include "logger.h"

using nlohmann::json;
using std::function;
using std::make_shared;
using std::unordered_map;
using std::vector;

namespace jeff_rpc {

const static ssize_t HEADER_SZ = 8;
typedef function<json(json)> ProcType;

struct RawRequest;
class RPCServer;
enum RegisterRet { SUCCESS, PROC_EXIST };

} // namespace jeff_rpc

struct jeff_rpc::RawRequest {
    int reqID, bodyLen;
    string body;

    RawRequest() {}
    RawRequest(int rid, int blen, const string &bbuffer)
        : reqID(rid), bodyLen(blen), body(bbuffer) {}
    string toString() { return std::to_string(reqID) + body; }
};

class jeff_rpc::RPCServer {
private:
    unordered_map<string, ProcType> registeredProcMap;

    bool registered(const string procName);
    RawRequest readRequest(int sd);
    int sendResponse(int sd, int reqID, const string &respStr);
    json callProcedure(const string procName, json args);
    int serveClient(int sd);

    RPCServer() {}

public:
    RPCServer(RPCServer const &) = delete;
    void operator=(RPCServer const &) = delete;
    ~RPCServer() {}

    RegisterRet registerProc(const string procName, ProcType proc);
    // thread will be terminated on exiting
    int startServer(int);

    static RPCServer &getInstance() {
        static RPCServer server;
        return server;
    }
};

#endif // RPC_SERVER_H
