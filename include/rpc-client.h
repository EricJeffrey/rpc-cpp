#if !defined(CLIENT_H)
#define CLIENT_H

#include "json.hpp"
#include <queue>
#include <unordered_map>
#include <string>
#include <functional>
#include <memory>
#include <mutex>

using nlohmann::json;
using std::function;
using std::lock_guard;
using std::make_shared;
using std::mutex;
using std::pair;
using std::queue;
using std::shared_ptr;
using std::string;
using std::unordered_map;

namespace jeff_rpc {

typedef function<void(json)> RetCallback;
typedef pair<RetCallback, RetCallback> CallBackPair;
typedef unsigned long ReqID;

// FIXME request response can be abstracted
struct Response;
struct ClientRequest;
class RPCClient;

} // namespace jeff_rpc

struct jeff_rpc::Response {
    int reqID, bodyLen;
    json retVal;
    Response(int r, int blen, const json &ret) : reqID(r), bodyLen(blen), retVal(ret) {}
    string toString();
};

struct jeff_rpc::ClientRequest {
    int reqID;
    string name;
    json para;

    ClientRequest(int rid, const string &n, const json &p) : reqID(rid), name(n), para(p) {}
    string toRequestString() const;
};

class jeff_rpc::RPCClient {
private:
    int sd;
    ReqID idCurMax;
    unordered_map<ReqID, CallBackPair> req2callback;
    queue<ClientRequest> requestQueue;
    mutex reqQueueMutex;

    ReqID newReqID() { return (idCurMax += 1); }

    bool reqQueueEmpty();
    int pushRequest(const ClientRequest &req);
    ClientRequest popRequest();

    int startJob();
    int connect(const string &host, int port);
    int writeRequest(const ClientRequest &req);
    Response readResponse();

    // singleton
    RPCClient() : sd(-1) {}

public:
    ~RPCClient() {}
    // remove copy assign, better compiler description
    RPCClient(RPCClient const &) = delete;
    void operator=(RPCClient const &) = delete;

    void closeConn() { close(sd); }
    // return false on failure
    bool startClient(const string &host, int port);
    // Call procedure of [name], callback [onReturn] on success, [onError] on network failure
    // [onReturn] para: { code: int, msg: str, ret: json }
    // [onError] para: { msg: str }
    int callProcedure(const string &name, const json &para, RetCallback onReturn,
                      RetCallback onError);

    static RPCClient &getInstance() {
        static RPCClient client;
        return client;
    }
};

#endif // CLIENT_H
