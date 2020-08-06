#if !defined(CLIENT_H)
#define CLIENT_H

#include "json.hpp"
#include <string>
#include <functional>
#include <memory>

using nlohmann::json;
using std::function;
using std::make_shared;
using std::shared_ptr;
using std::string;

namespace jeff_rpc {

typedef function<void(json)> RetCallback;

class RPCClient {
private:
    int sd;
    static shared_ptr<RPCClient> client;
    RPCClient() : sd(-1) {}

public:
    ~RPCClient() {}
    int connect(const string &host, int port);
    int callProcedure(const string &name, const json &para, RetCallback onReturn);

    static shared_ptr<RPCClient> getInstance() {
        if (!client)
            client = make_shared<RPCClient>(RPCClient());
        return client;
    }
};

} // namespace jeff_rpc

#endif // CLIENT_H
