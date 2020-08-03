#if !defined(RPC_SERVER_H)
#define RPC_SERVER_H

#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <exception>
#include <stdexcept>
#include <error.h>
#include <vector>
#include <unordered_map>
#include <functional>

#include "lib/json.hpp"
#include "logger.h"

using nlohmann::json;
using std::function;
using std::make_shared;
using std::runtime_error;
using std::thread;
using std::unordered_map;
using std::vector;

namespace jeff_rpc {

const static int HEADER_SZ = 8;

struct RawRequest;
class RPCServer;

json parse2json(string req);
string serialize2str(json resp);

} // namespace jeff_rpc

struct jeff_rpc::RawRequest {
    int reqID, bodyLen;
    string body;

    RawRequest() {}
    RawRequest(int rid, int blen, const string &bbuffer) {
        reqID = rid, bodyLen = blen, body = bbuffer;
    }
    string toString() { return std::to_string(reqID) + body; }
};

class jeff_rpc::RPCServer {
    typedef function<json(json)> ProcType;

private:
    unordered_map<string, ProcType> registeredProcMap;
    static shared_ptr<RPCServer> rpcServerInstance;

    bool registered(const string procName);
    RawRequest readRequest(int sd) throw();
    int sendResponse(int sd, int reqID, json respJson);
    json callProcedure(const string procName, json args);
    int serveClient(int sd);
    RPCServer() {}

public:
    static auto getInstance() {
        if (!rpcServerInstance) rpcServerInstance = make_shared<RPCServer>(RPCServer());
        return rpcServerInstance;
    }
    ~RPCServer() {}

    int registerProc(const string procName, ProcType proc);
    int startServer();
};

#endif // RPC_SERVER_H
