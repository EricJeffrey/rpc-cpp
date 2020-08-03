#if !defined(RPC_SERVER_CPP)
#define RPC_SERVER_CPP

#include "rpc-server.h"

using std::to_string;

namespace jeff_rpc {

json parse2json(string s) {
    loggerInstance()->debug({"parsing s to json: ", s});
    return json::parse(s);
}
string serialize2str(json j) {
    loggerInstance()->debug({"stringify json", j.dump(), "to str"});
    return j.dump();
}

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
    int optVal = 1;
    ret = setsockopt(listenSd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int));
    if (ret == -1) {
        Logger::getInstance()->error({"setsockopt failed"});
        close(listenSd);
        return 0;
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
        return 0;
    }
    loggerInstance()->debug({"listening on socket"});
    // linsten
    ret = listen(listenSd, 1024);
    if (ret == -1) {
        loggerInstance()->error({"listen failed"});
    }
    int clientSd;
    sockaddr clientAddr;
    socklen_t clientAddrLen = sizeof(sockaddr);
    loggerInstance()->debug({"accepting client"});
    // accept
    while ((clientSd = accept(listenSd, &clientAddr, &clientAddrLen)) != -1) {
        new thread([clientSd](RPCServer *server) -> void { server->serveClient(clientSd); }, this);
    }
    loggerInstance()->error({"accept failed", strerror(errno)});
    // clsoe
    if (close(listenSd) == -1) loggerInstance()->error({"even close failed", strerror(errno)});
    throw runtime_error("start server failed on accept");
}

int RPCServer::registerProc(const string name, ProcType proc) {
    loggerInstance()->debug({"registerProc, name:", name});
    if (registered(name)) {
        loggerInstance()->error({"proc:", name, "already registered"});
        return -1;
    }
    loggerInstance()->debug({"registering proc:", name});
    registeredProcMap[name] = proc;
    return 0;
}

bool RPCServer::registered(const string procName) {
    return registeredProcMap.find(procName) != registeredProcMap.end();
}

RawRequest RPCServer::readRequest(int sd) throw() {
    char headerBuf[HEADER_SZ];
    ssize_t ret = 0;
    ssize_t byteNumRead = 0;
    loggerInstance()->debug({"reading request header"});
    while (byteNumRead < HEADER_SZ) {
        ret = read(sd, headerBuf + byteNumRead, HEADER_SZ - byteNumRead);
        const int tmpErrno = errno;
        if (ret == -1 && tmpErrno != EINTR) {
            loggerInstance()->error({"read failed", strerror(tmpErrno)});
            throw runtime_error("read failed");
        } else {
            if (tmpErrno == EINTR) loggerInstance()->debug({"read interrupted by signal"});
            byteNumRead += ret;
            ret = 0;
        }
    }
    size_t reqID, bodyLen;
    reqID = bodyLen = 0;
    for (int i = 0; i < 4; i++) {
        reqID <<= 8, reqID += headerBuf[i];
        bodyLen <<= 8, bodyLen += headerBuf[i + 4];
    }
    loggerInstance()->debug(
        {"reading request body, length: ", to_string(bodyLen), " reqID:", to_string(reqID)});
    char *bodyBuf = new char[bodyLen];
    std::fill(bodyBuf, bodyBuf + bodyLen, '\0');
    byteNumRead = 0;
    while ((size_t)byteNumRead < bodyLen) {
        ret = read(sd, bodyBuf + byteNumRead, bodyLen - byteNumRead);
        const int tmpErrno = errno;
        if (ret == -1 && tmpErrno != EINTR) {
            loggerInstance()->error({"read failed", strerror(tmpErrno)});
            throw runtime_error("read failed");
        } else {
            if (tmpErrno == EINTR) loggerInstance()->debug({"read interrupted by signal"});
            byteNumRead += ret;
        }
    }
    RawRequest req(reqID, bodyLen, string(bodyBuf, bodyLen));
    delete[] bodyBuf;
    vector<char> s;
    return req;
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
        loggerInstance()->info({"request read:", req.toString()});
        json reqBody = parse2json(req.body);
        loggerInstance()->info({"request body parsed to json"});
        if (!reqBody.contains("name") || !registered(reqBody["name"])) {
            loggerInstance()->info({"requested proc not found, sending 404"});
            sendResponse(sd, req.reqID, "{code: 404, msg: \"NOT FOUND\", ret: {}}"_json);
        } else {
            loggerInstance()->info({"calling requested proc"});
            json args;
            if (reqBody.contains("args")) args = reqBody["args"];
            json retJson = callProcedure(reqBody["name"], args);
            loggerInstance()->info(
                {"requested proc returned with:", retJson.dump(), "sending back"});
            sendResponse(sd, req.reqID, retJson);
        }
    } catch (const std::exception &e) {
        loggerInstance()->error({"serve client failed", e.what()});
        sendResponse(sd, req.reqID, "{code: 500, msg:\"INTERNAL ERROR\", ret: {}}"_json);
        return 0;
    }
    return 0;
}

shared_ptr<RPCServer> RPCServer::rpcServerInstance = nullptr;

} // namespace jeff_rpc
#endif // RPC_SERVER_CPP
