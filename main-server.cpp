#if !defined(MAIN_SERVER_CPP)
#define MAIN_SERVER_CPP

#include "rpc-server.h"

using jeff_rpc::RPCServer;

int work() {
    const int port = 5656;
    auto test = [](json) -> json { return json::parse("{\"output\": \"hello\"}"); };
    auto hello = [](json) -> json { return "{\"name\": \"rpc-server\"}"_json; };
    try {
        auto &rpcServer = RPCServer::getInstance();
        rpcServer.registerProc("test", test);
        rpcServer.registerProc("hello", hello);
        rpcServer.startServer(port);
    } catch (const std::exception &e) {
        loggerInstance().error({"rpc server terminated:", e.what()});
    } catch (...) {
        loggerInstance().error({"rpc server accidently terminated"});
    }
    return 0;
}

int main(int argc, char const *argv[]) {
    work();
    return 0;
}

#endif // MAIN_SERVER_CPP
