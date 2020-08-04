#if !defined(RPC_SERVER_CPP)
#define RPC_SERVER_CPP

#include "rpc-server.h"

using std::copy;
using std::to_string;

namespace jeff_rpc {

int writen(int sd, char *buf, ssize_t numToWrite) {
    ssize_t byteNumWrite = 0, ret;
    while (byteNumWrite < numToWrite) {
        ret = write(sd, buf + byteNumWrite, numToWrite - byteNumWrite);
        const int tmpErrno = errno;
        if (ret == -1) {
            loggerInstance()->error({"write failed", strerror(tmpErrno)});
            throw runtime_error("write failed");
        } else {
            if (tmpErrno == EINTR)
                loggerInstance()->debug({"write interrputed by signal"});
            byteNumWrite += ret;
        }
    }
    return 0;
}
int readn(int sd, char *buf, ssize_t numToRead) {
    ssize_t byteNumRead = 0, ret;
    while (byteNumRead < numToRead) {
        ret = read(sd, buf + byteNumRead, numToRead - byteNumRead);
        const int tmpErrno = errno;
        if (ret == -1) {
            loggerInstance()->error({"read failed", strerror(tmpErrno)});
            throw runtime_error("read failed");
        } else if (ret > 0) {
            if (tmpErrno == EINTR)
                loggerInstance()->debug({"read interrupted by signal"});
            byteNumRead += ret;
        }
    }
    return 0;
}
json parse2json(string s) {
    loggerInstance()->debug({"parsing s:|", s, "|to json: "});
    return json::parse(s);
}
string serialize2str(json j) {
    loggerInstance()->debug({"stringify json to str"});
    return j.dump();
}

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

// todo return value -> already registered (enum + struct?)
int RPCServer::registerProc(const string name, ProcType proc) {
    if (registered(name)) {
        loggerInstance()->error({"proc:", name, "already registered"});
        return -1;
    }
    registeredProcMap[name] = proc;
    loggerInstance()->debug({"registered proc:", name});
    return 0;
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
        char *bodyBuf = new char[bodyLen + 1];
        readn(sd, bodyBuf, bodyLen);
        RawRequest req(reqID, bodyLen, string(bodyBuf, bodyLen));
        delete[] bodyBuf;
        vector<char> s;
        return req;
    } catch (...) {
        throw;
    }
    return RawRequest();
}

int RPCServer::sendResponse(int sd, int reqID, json respJson) {
    loggerInstance()->debug({"serializing response"});
    string respStr = serialize2str(respJson);
    const ssize_t bodyLen = respStr.size();
    char *respBuffer = new char[HEADER_SZ + bodyLen];
    for (uint32_t i = 0, t = 0xff; i < 4; i++, t <<= 8) {
        respBuffer[i] = (reqID & t) >> (8 * i);
        respBuffer[i + 4] = (bodyLen & t) >> (8 * i);
    }
    copy(respStr.begin(), respStr.end(), respBuffer + HEADER_SZ);
    try {
        loggerInstance()->debug({"sending response"});
        writen(sd, respBuffer, HEADER_SZ + bodyLen);
        delete[] respBuffer;
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
    RawRequest req;
    try {
        req = readRequest(sd);
        loggerInstance()->info({"request read:", req.toString()});
        json reqBody = parse2json(req.body);
        loggerInstance()->info({"request body parsed to json"});
        if (!reqBody.contains("name") || !registered(reqBody["name"])) {
            loggerInstance()->info({"requested proc not found, sending 404"});
            sendResponse(sd, req.reqID, "{code: 404, msg: \"NOT FOUND\", ret: {}}"_json);
        } else {
            loggerInstance()->info({"calling requested proc"});
            json args;
            if (reqBody.contains("args"))
                args = reqBody["args"];
            json retJson = callProcedure(reqBody["name"], args);
            loggerInstance()->info(
                {"requested proc returned with:", retJson.dump(), "sending back"});
            sendResponse(sd, req.reqID, retJson);
        }
    } catch (const std::exception &e) {
        loggerInstance()->error({"serve client failed", e.what()});
        sendResponse(sd, req.reqID, "{code: 500, msg:\"INTERNAL ERROR\", ret: {}}"_json);
        return 0;
    } catch (...) {
        loggerInstance()->error({"serve client failed"});
        sendResponse(sd, req.reqID, "{code: 500, msg:\"INTERNAL ERROR\", ret: {}}"_json);
    }
    return 0;
}

shared_ptr<RPCServer> RPCServer::rpcServerInstance = nullptr;

} // namespace jeff_rpc
#endif // RPC_SERVER_CPP
