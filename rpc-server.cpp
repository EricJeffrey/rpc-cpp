#if !defined(RPC_SERVER_CPP)
#define RPC_SERVER_CPP

#include <sstream>
#include <memory>
#include <thread>
#include "utils.h"
#include "rpc-server.h"

using std::copy;
using std::ostringstream;
using std::thread;
using std::to_string;
using std::unique_ptr;

namespace jeff_rpc {

int RPCServer::startServer(int port) {
    int ret = 0;
    loggerInstance()->debug({"creating socket"});
    // create
    int listenSd = socket(AF_INET, SOCK_STREAM, 6); // /etc/protocols or protocols(5)
    if (listenSd == -1) {
        loggerInstance()->error({"create socket failed", strerror(errno)});
        throw runtime_error("call to socket failed");
    }
    int optVal = 1;
    // set reuse
    loggerInstance()->debug({"set socket resuable"});
    ret = setsockopt(listenSd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int));
    if (ret == -1) {
        loggerInstance()->error({"setsockopt failed, closing", strerror(errno)});
        if (close(listenSd) == -1)
            loggerInstance()->error({"close socket failed"});
        throw runtime_error("call to setsockopt failed");
    }
    // address
    sockaddr_in addr;
    addr.sin_port = htons(port), addr.sin_family = AF_INET, addr.sin_addr.s_addr = INADDR_ANY;
    // bind
    loggerInstance()->debug({"binding socket"});
    ret = bind(listenSd, (sockaddr *)&addr, sizeof addr);
    if (ret == -1) {
        loggerInstance()->error({"bind socket failed", strerror(errno)});
        if (close(listenSd) == -1)
            loggerInstance()->error({"close socket failed", strerror(errno)});
        throw runtime_error("call to bind failed");
    }
    loggerInstance()->debug({"listening on socket"});
    // linsten
    ret = listen(listenSd, 1024);
    if (ret == -1) {
        loggerInstance()->error({"listen failed"});
        if (close(listenSd) == -1)
            loggerInstance()->error({"clase socket failed", strerror(errno)});
        throw runtime_error("call to listen failed");
    }
    int clientSd;
    sockaddr clientAddr;
    socklen_t clientAddrLen = sizeof(sockaddr);
    // accept
    loggerInstance()->debug({"accepting client"});
    while ((clientSd = accept(listenSd, &clientAddr, &clientAddrLen)) != -1) {
        thread([clientSd](RPCServer *server) -> void { server->serveClient(clientSd); }, this)
            .detach();
    }
    loggerInstance()->error({"accept failed", strerror(errno)});
    if (close(listenSd) == -1)
        loggerInstance()->error({"even close failed", strerror(errno)});
    throw runtime_error("call to accept failed");
}

RegisterRet RPCServer::registerProc(const string name, ProcType proc) {
    if (registered(name)) {
        loggerInstance()->error({"proc:", name, "already registered"});
        return RegisterRet::PROC_EXIST;
    }
    registeredProcMap[name] = proc;
    loggerInstance()->debug({"registered proc:", name});
    return RegisterRet::SUCCESS;
}

bool RPCServer::registered(const string procName) {
    return registeredProcMap.find(procName) != registeredProcMap.end();
}

RawRequest RPCServer::readRequest(int sd) {
    char headerBuf[HEADER_SZ];
    loggerInstance()->debug({"reading request header"});
    try {
        readn(sd, headerBuf, HEADER_SZ);
        size_t reqID;
        ssize_t bodyLen;
        reqID = bodyLen = 0;
        for (int i = 0; i < 4; i++) {
            reqID <<= 8, reqID += headerBuf[i];
            bodyLen <<= 8, bodyLen += headerBuf[i + 4];
        }
        loggerInstance()->debug({"reading request body"});
        unique_ptr<char[]> bodyBufPtr(new char[bodyLen + 1]);
        readn(sd, bodyBufPtr.get(), bodyLen);
        RawRequest req(reqID, bodyLen, string(bodyBufPtr.get(), bodyLen));
        vector<char> s;
        return req;
    } catch (...) {
        throw;
    }
    return RawRequest();
}

int RPCServer::sendResponse(int sd, int reqID, const string &respStr) {
    loggerInstance()->debug({"serializing response"});
    const ssize_t bodyLen = respStr.size();
    unique_ptr<char[]> respBufferPtr(new char[HEADER_SZ + bodyLen]);
    for (uint32_t i = 0, t = 0xff; i < 4; i++, t <<= 8) {
        respBufferPtr[i] = (reqID & t) >> (8 * i);
        respBufferPtr[i + 4] = (bodyLen & t) >> (8 * i);
    }
    copy(respStr.begin(), respStr.end(), respBufferPtr.get() + HEADER_SZ);
    try {
        loggerInstance()->debug({"sending response"});
        writen(sd, respBufferPtr.get(), HEADER_SZ + bodyLen);
        return 0;
    } catch (...) {
        throw;
    }
    return 0;
}

json RPCServer::callProcedure(const string procName, json args) {
    loggerInstance()->debug({"calling proc:", procName});
    return registeredProcMap[procName](args);
}

int RPCServer::serveClient(int sd) {
    auto wrapResp = [](int code, const json &resp = "{}"_json) -> string {
        string msg;
        switch (code) {
            case 200:
                msg = "OK";
                break;
            case 404:
                msg = "NOT FOUND";
                break;
            case 500:
                msg = "INTERNAL ERROR";
                break;
            default:
                msg = "ERROR";
        }
        ostringstream ss;
        ss << "{code:" << code << ",msg:" << msg << ",ret:" << resp.dump() << "}";
        return ss.str();
    };
    RawRequest req;
    try {
        int opt = 1;
        if (setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)) == -1) {
            loggerInstance()->error({"set socket option - keep alive failed", strerror(errno)});
            throw SocketError("call setsockopt failed");
        }
        while (true) {
            req = readRequest(sd);
            loggerInstance()->info({"request read:", req.toString()});
            json reqBody = parse2json(req.body);
            loggerInstance()->info({"request body parsed to json"});
            if (!reqBody.contains("name") || !registered(reqBody["name"])) {
                loggerInstance()->info({"requested proc not found, sending 404"});
                sendResponse(sd, req.reqID, wrapResp(404));
            } else {
                loggerInstance()->info({"calling requested proc"});
                json args;
                if (reqBody.contains("args"))
                    args = reqBody["args"];
                json retJson = callProcedure(reqBody["name"], args);
                loggerInstance()->debug({"requested proc returned, sending back"});
                sendResponse(sd, req.reqID, wrapResp(200, retJson));
            }
        }
    } catch (const SocketError &e) {
        // socket(connection) is not valid anymore
        loggerInstance()->error({"serve client failed", e.what()});
        close(sd);
    } catch (const std::exception &e) {
        loggerInstance()->error({"serve client failed", e.what()});
        try {
            sendResponse(sd, req.reqID, wrapResp(500));
        } catch (...) {
            loggerInstance()->error({"send response 500 failed"});
        }
        close(sd);
        return 0;
    }
    return 0;
}

shared_ptr<RPCServer> RPCServer::rpcServerInstance = nullptr;

} // namespace jeff_rpc
#endif // RPC_SERVER_CPP
