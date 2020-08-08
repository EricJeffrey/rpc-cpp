#include <exception>

#include "rpc-server.h"
#include "rpc-client.h"

using jeff_rpc::RPCClient;
using jeff_rpc::RPCServer;

int serverTester() {
    const int port = 5656;
    auto test = [](json) -> json { return json::parse("{\"output\": \"hello\"}"); };
    try {
        auto &rpcServer = RPCServer::getInstance();
        rpcServer.registerProc("test", test);
        rpcServer.startServer(port);
    } catch (const std::exception &e) {
        loggerInstance().error({"rpc server terminated:", e.what()});
    } catch (...) {
        loggerInstance().error({"rpc server accidently terminated"});
    }
    return 0;
}

int clientTester() {
    jeff_rpc::ClientRequest req(2, "halo", "{\"para\": 666}"_json);
    cout << req.toRequestString() << endl;
    return 0;
}

int main(int argc, char const *argv[]) {
    // serverTester();
    clientTester();
    return 0;
}
