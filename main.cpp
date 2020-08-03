#include <exception>

#include "rpc-server.h"

using jeff_rpc::RPCServer;

shared_ptr<Logger> Logger::logInstancePtr = nullptr;

// todo error handler
// todo exception

json test(json args) { return json::parse("{\"output\": \"hello\"}"); }

int main(int argc, char const *argv[]) {
    try {
        auto rpcServer = RPCServer::getInstance();
        rpcServer->registerProc("test", test);
        rpcServer->startServer();
    } catch (const std::exception &e) {
        Logger::getInstance()->error({e.what()});
    }
    return 0;
}
