#if !defined(RPC_CLIENT_CPP)
#define RPC_CLIENT_CPP

#include "rpc-client.h"
#include "logger.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <exception>

using std::runtime_error;
using std::to_string;

namespace jeff_rpc {

int RPCClient::connect(const string &host, int port) {
    loggerInstance()->debug({"connect"});
    int ret = 0;
    // create socket
    loggerInstance()->debug({"creating socket"});
    ret = socket(AF_INET, SOCK_STREAM, 0);
    if (ret == -1) {
        loggerInstance()->error(
            {"create socket to", host, ":", to_string(port), "failed", strerror(errno)});
        throw runtime_error("call to socket failed");
    }
    this->sd = ret;
    // set keep-alive
    loggerInstance()->debug({"setting socket option - keep alive"});
    int opt = 1;
    ret = setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
    if (ret == -1) {
        loggerInstance()->error({"set socket option - keepalive failed", strerror(errno)});
        throw runtime_error("call to setsockopt failed");
    }
    // connect
    loggerInstance()->debug({"connecting to server"});
    sockaddr_in addr;
    addr.sin_family = AF_INET, addr.sin_port = htonl(port), inet_aton(host.c_str(), &addr.sin_addr);
    ret = ::connect(sd, (sockaddr *)&addr, sizeof(addr));
    if (ret == -1) {
        loggerInstance()->error({"connect to host failed", strerror(errno)});
        throw runtime_error("call to connect failed");
    }
    loggerInstance()->debug({"connection established"});
    return 0;
}

int RPCClient::callProcedure(const string &name, const json &para, RetCallback onReturn) {
    // todo serialize para and call
    try {
        const string paraStr = para.dump();

    } catch (const std::exception &e) {
        loggerInstance()->error({""});
    }
}

static shared_ptr<RPCClient> client;
} // namespace jeff_rpc

#endif // RPC_CLIENT_CPP
