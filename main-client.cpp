#if !defined(MAIN_CLIENT_CPP)
#define MAIN_CLIENT_CPP

#include "rpc-client.h"
using jeff_rpc::RPCClient;

int work() {
    const string host = "127.0.0.1";
    const int port = 5656;
    auto &client = RPCClient::getInstance();
    client.startClient(host, port);
    client.callProcedure(
        "halo", "{}",
        [](const json &ret) -> json {
            printf("call hello sucess, ret: %s\n", ret.dump().c_str());
            return "{}"_json;
        },
        [](const json &err) -> json {
            printf("call hello failed, err: %s\n", err.dump().c_str());
            return "{}"_json;
        });
    loggerInstance().debug({"sleeping now"});
    while (true)
        ;
    client.closeConn();
    return 0;
}

int main(int argc, char const *argv[]) {
    work();
    return 0;
}

#endif // MAIN_CLIENT_CPP
