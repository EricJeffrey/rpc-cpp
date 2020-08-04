#include <exception>

#include "rpc-server.h"

using jeff_rpc::RPCServer;

// todo error handler
// todo exception

json test(json args) { return json::parse("{\"output\": \"hello\"}"); }

const int port = 5656;

int main(int argc, char const *argv[]) {
    try {
        // auto t = new char[23];

        auto rpcServer = RPCServer::getInstance();
        rpcServer->registerProc("test", test);
        rpcServer->startServer(port);
    } catch (const std::exception &e) {
        loggerInstance()->error({"rpc server terminated:", e.what()});
    } catch (...) {
        Logger::getInstance()->error({"rpc server accidently terminated"});
    }
    return 0;
}
