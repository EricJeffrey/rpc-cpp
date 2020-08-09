#if !defined(RPC_CLIENT_CPP)
#define RPC_CLIENT_CPP

#include "utils.h"
#include "rpc-client.h"
#include "logger.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <exception>
#include <thread>
#include <sstream>

using std::make_pair;
using std::runtime_error;
using std::stringstream;
using std::thread;
using std::to_string;

namespace jeff_rpc {

string Response::toString() {
    stringstream ss;
    ss << reqID << ", " << bodyLen << "\n--------" << retVal;
    return ss.str();
}

string ClientRequest::toRequestString() const {
    stringstream ss;
    const string respBody = json({{"name", name}, {"args", para}}).dump();
    ss << int2str(reqID) << int2str(respBody.size()) << respBody;
    return ss.str();
}

bool RPCClient::reqQueueEmpty() {
    const lock_guard<mutex> guard(reqQueueMutex);
    return requestQueue.empty();
}
const ClientRequest &RPCClient::topRequest() {
    const lock_guard<mutex> guard(reqQueueMutex);
    const ClientRequest &req = requestQueue.front();
    return req;
}
int RPCClient::popRequest() {
    const lock_guard<mutex> guard(reqQueueMutex);
    requestQueue.pop();
    return 0;
}

int RPCClient::pushRequest(const ClientRequest &req) {
    const lock_guard<mutex> guard(reqQueueMutex);
    requestQueue.push(req);
    return 0;
}

int RPCClient::writeRequest(const ClientRequest &req) {
    const string resp = req.toRequestString();
    loggerInstance().debug({"sending to server"});
    writen(sd, resp.c_str(), resp.size());
    return 0;
}

Response RPCClient::readResponse() {
    loggerInstance().debug({"reading response"});
    // FIXME should be sepcified on higher level
    const ssize_t HEADER_SZ = 8;
    char headerBuf[HEADER_SZ] = {};
    try {
        loggerInstance().debug({"reading header"});
        readn(sd, headerBuf, HEADER_SZ);
        ReqID reqId = str2int(headerBuf), bodyLen = str2int(headerBuf + 4);
        loggerInstance().debug({"header reqID, bodyLen: ", to_string(reqId), to_string(bodyLen)});
        char *bodyBufPtr = new char[bodyLen + 1];
        readn(sd, bodyBufPtr, bodyLen);
        loggerInstance().debug({"response read, body: ", bodyBufPtr});
        Response res = Response(reqId, bodyLen, json::parse(string(bodyBufPtr, bodyLen)));
        delete[] bodyBufPtr;
        return res;
    } catch (const SocketError &e) {
        loggerInstance().error({"socket error, ", e.what()});
        throw;
    } catch (...) {
        loggerInstance().error({"read response failed"});
        throw;
    }
    // won't reach
}

int RPCClient::startJob() {
    const useconds_t uSecToSleep = 10000;
    loggerInstance().debug({"background thread job started, sleep usec:", to_string(uSecToSleep)});
    while (true) {
        if (!reqQueueEmpty()) {
            loggerInstance().debug({"request not empty, poping request"});
            ReqID tempReqID = -1;
            try {
                const ClientRequest &req = topRequest();
                tempReqID = req.reqID;
                loggerInstance().debug({"request got:", req.toRequestString()});
                writeRequest(req);
                loggerInstance().debug({"request sent to server"});
                Response resp = readResponse();
                loggerInstance().debug({"response got, resp: ", resp.toString()});
                loggerInstance().debug({"invoke callback"});
                req2callback[resp.reqID].first(resp.retVal);
                req2callback.erase(resp.reqID);
                popRequest();
            } catch (const SocketError &e) {
                loggerInstance().error({"socket error, closing"});
                if (tempReqID != -1 && req2callback[tempReqID].second != nullptr)
                    req2callback[tempReqID].second(
                        "{\"msg\": \"socket error, connection may close\"}"_json);
                close(sd);
                return -1;
            } catch (const std::exception &e) {
                loggerInstance().error({"error when call remote proc", e.what()});
                if (tempReqID != -1 && req2callback[tempReqID].second != nullptr) {
                    req2callback[tempReqID].second(
                        "{\"msg\": \"error on read, write or parse\"}"_json);
                    req2callback.erase(tempReqID);
                }
            }
        }
        usleep(uSecToSleep);
    }
    return 0;
}
int RPCClient::connect(const string &host, int port) {
    loggerInstance().debug({"connect"});
    int ret = 0;
    // create socket
    loggerInstance().debug({"creating socket"});
    ret = socket(AF_INET, SOCK_STREAM, 0);
    if (ret == -1) {
        loggerInstance().error(
            {"create socket to", host, ":", to_string(port), "failed", strerror(errno)});
        throw runtime_error("call to socket failed");
    }
    this->sd = ret;
    // set keep-alive
    loggerInstance().debug({"setting socket option - keep alive"});
    int opt = 1;
    ret = setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
    if (ret == -1) {
        loggerInstance().error({"set socket option - keepalive failed", strerror(errno)});
        close(sd);
        throw runtime_error("call to setsockopt failed");
    }
    // connect
    loggerInstance().debug({"connecting to server"});
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_aton(host.c_str(), &addr.sin_addr);
    loggerInstance().debug({"converted addr, raw port:", to_string(port), "converted port: ",
                            to_string(addr.sin_port), ", addr: ", inet_ntoa(addr.sin_addr)});
    ret = ::connect(sd, (sockaddr *)&addr, sizeof(addr));
    if (ret == -1) {
        loggerInstance().error({"connect to host failed", strerror(errno)});
        close(sd);
        throw runtime_error("call to connect failed");
    }
    loggerInstance().debug({"connection established"});
    return 0;
}

bool RPCClient::startClient(const string &host, int port) {
    try {
        loggerInstance().debug({"connecting to server"});
        connect(host, port);
        loggerInstance().debug({"connected, starting background thread"});
        thread([&]() -> void { startJob(); }).detach();
    } catch (const std::exception &e) {
        loggerInstance().error({"client init failed"});
        return false;
    }
    return true;
}
int RPCClient::callProcedure(const string &name, const json &para, RetCallback onReturn,
                             RetCallback onError) {
    ReqID reqID = newReqID();
    pushRequest(ClientRequest(reqID, name, para));
    req2callback[reqID] = make_pair(onReturn, onError);
    return 0;
}
} // namespace jeff_rpc

#endif // RPC_CLIENT_CPP
