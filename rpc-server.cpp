#if !defined(RPC_SERVER)
#define RPC_SERVER

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

json parse2json(string);
string serialize2str(json);

const static int HEADER_SZ = 8;

struct RawRequest {
    int reqID, bodyLen;
    char *bodyBuffer;

    RawRequest(int rid, int blen, char *bbuffer) {
        reqID = rid, bodyLen = blen;
        bodyBuffer = bbuffer;
    }
    RawRequest() { bodyBuffer = nullptr; }
    void freeBody() {
        if (bodyBuffer != nullptr) {
            delete[] bodyBuffer;
            bodyBuffer = nullptr;
        }
    }
};

class RPCServer {
    typedef function<json(json)> ProcType;

private:
    unordered_map<string, ProcType> registeredProcMap;
    static shared_ptr<RPCServer> rpcServerInstance;

    bool registered(const string procName);
    RawRequest readRequest(int sd);
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

int RPCServer::startServer() {
    const int port = 5656;
    int ret = 0;

    loggerInstance()->debug({"creating socket"});
    // create
    int listenSd = socket(AF_INET, SOCK_STREAM, 6); // /etc/protocols or protocols(5)
    if (listenSd == -1) {
        Logger::getInstance()->error({"create socket failed", strerror(errno)});
        throw runtime_error("socket");
    }
    ret = setsockopt(listenSd, SOL_SOCKET, SO_REUSEADDR, nullptr, 0);
    if (ret == -1) {
        Logger::getInstance()->error({"setsockopt failed"});
        close(listenSd);
        return;
    }
    // address
    sockaddr_in addr;
    addr.sin_port = htons(port), addr.sin_family = AF_INET, addr.sin_addr.s_addr = INADDR_ANY;
    Logger::getInstance()->debug({"binding socket to port", std::to_string(port)});
    // bind
    ret = bind(listenSd, (sockaddr *)&addr, sizeof addr);
    if (ret == -1) {
        Logger::getInstance()->error({"bind socket failed", strerror(errno)});
        if (close(listenSd) == -1)
            Logger::getInstance()->error({"close socket failed", strerror(errno)});
        return;
    }
    loggerInstance()->debug({"listening on socket"});
    // linsten
    ret = listen(listenSd, 1024);
    if (ret == -1) {
        loggerInstance()->error({"listen failed"});
    }
    int clientSd = -1;
    sockaddr clientAddr;
    socklen_t clientAddrLen = sizeof(sockaddr);
    loggerInstance()->debug({"accepting client"});
    // accept
    while ((clientSd = accept(listenSd, &clientAddr, &clientAddrLen)) != -1) {
        // todo new thread and server client
        thread t = thread(serveClient);
    }
    loggerInstance()->error({"accept failed", strerror(errno)});
    // clsoe
    if (close(listenSd) == -1) {
        loggerInstance()->error({"even close failed", strerror(errno)});
    }
}

int RPCServer::registerProc(const string name, ProcType proc) {
    loggerInstance()->debug({"registerProc, name:", name});
    if (registered(name)) {
        loggerInstance()->error({"proc:", name, "already registered"});
        return -1;
    }
    loggerInstance()->debug({"registering proc:", name});
    registeredProcMap[name] = proc;
}

bool RPCServer::registered(const string procName) {
    return registeredProcMap.find(procName) != registeredProcMap.end();
}

RawRequest RPCServer::readRequest(int sd) throw() {
    char headerBuf[HEADER_SZ];
    int ret = 0, byteNumRead = 0;
    loggerInstance()->debug({"reading request header"});
    while (byteNumRead < HEADER_SZ) {
        ret = read(sd, headerBuf + byteNumRead, HEADER_SZ - byteNumRead);
        if (ret == -1) {
            // FIXME signal interrupted
            loggerInstance()->error({"read failed", strerror(errno)});
            throw runtime_error("read failed");
        } else {
            byteNumRead += ret;
            ret = 0;
        }
    }
    int reqID, bodyLen;
    for (int i = 0; i < 4; i++) {
        reqID += headerBuf[i], reqID <<= 8;
        bodyLen += headerBuf[i + 4], bodyLen <<= 8;
    }
    loggerInstance()->debug({"reading request body"});
    char *bodyBuf = new char[bodyLen];
    byteNumRead = 0;
    while (byteNumRead < bodyLen) {
        ret = read(sd, bodyBuf + byteNumRead, bodyLen - byteNumRead);
        if (ret == -1) {
            // FIXME signal interrupted
            loggerInstance()->error({"read failed", strerror(errno)});
            throw runtime_error("read failed");
        } else {
            byteNumRead += ret;
        }
    }
    return RawRequest(reqID, bodyLen, bodyBuf);
}

int RPCServer::sendResponse(int sd, int reqID, json respJson) {
    loggerInstance()->debug({"sending response"});
    string respStr = serialize2str(respJson);
    const int bodyLen = respStr.size();
    char *respBuffer = new char[HEADER_SZ + bodyLen];
    for (uint32_t i = 0, t = 0xff; i < 4; i++, t <<= 8) {
        respBuffer[i] = (reqID & t) >> (8 * i);
        respBuffer[i + 4] = (bodyLen & t) >> (8 * i);
    }
    std::copy(respStr.begin(), respStr.end(), respBuffer + HEADER_SZ);
    int byteNumWrite = 0, ret;
    while (byteNumWrite < HEADER_SZ + bodyLen) {
        ret = write(sd, respBuffer + byteNumWrite, HEADER_SZ + bodyLen - byteNumWrite);
        if (ret == -1) {
            // FIXME signal interrupted
            loggerInstance()->error({"write failed"});
            throw runtime_error("write failed");
        } else {
            byteNumWrite += ret;
        }
    }
    delete[] respBuffer;
    return 0;
}

json RPCServer::callProcedure(const string procName, json args) {
    loggerInstance()->debug({"calling proc:", procName});
    return registeredProcMap[procName](args);
}

int RPCServer::serveClient(int sd) {
    RawRequest req;
    try {
        req = readRequest(sd);
        json reqBody = parse2json(req.bodyBuffer);
        req.freeBody();
        if (!reqBody.contains("name") || !registered(reqBody["name"])) {
            sendResponse(sd, req.reqID, "{code: 404, msg: \"NOT FOUND\", ret: {}}"_json);
        } else {
            json args;
            if (reqBody.contains("args")) args = reqBody["args"];
            json retJson = callProcedure(reqBody["name"], args);
            sendResponse(sd, req.reqID, retJson);
        }
    } catch (const std::exception &e) {
        loggerInstance()->error({"serve client failed"});
        sendResponse(sd, req.reqID, "{code: 500, msg:\"INTERNAL ERROR\", ret: {}}"_json);
        req.freeBody();
        return 0;
    }
}

json parse2json(string req) {
    loggerInstance()->debug({"parsing request"});
    json res = json::parse(req);
}
string serialize2str(json resp) {
    loggerInstance()->debug({"stringify response"});
    return resp.dump();
}

shared_ptr<RPCServer> RPCServer::rpcServerInstance = nullptr;

#endif // RPC_SERVER
