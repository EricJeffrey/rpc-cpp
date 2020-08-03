
# RPC框架模型

远程过程调用框架，使用TCP传输过程调用请求，使用一连接一线程模式响应客户端的RPC请求。

## 方法注册

方法注册主要由RPC服务端程序执行，程序调用RPC服务实例注册可调用过程。


## 连接建立与通信

RPC服务端使用传统的`bind+listen+accept`方式等待客户端的请求。客户端可以有多个，客户端启动时使用`connect`与服务端建立连接。之后，客户端与服务端使用建立的连接传输数据。客户端与服务端采用约定的数据格式传输，由首部Header确定RPC请求与响应的大小。

### RPC服务端

- 总服务程序
  1. `bind+listen+accept`初始化
  2. 建立**新线程**处理请求，标识连接ConnID
- 单个客户端服务程序
  1. 接收客户端数据
  2. 根据首部得到数据长度，读取数据
  3. 建立**新线程**处理，设置成功/失败的回调callback
     1. 解析请求的过程ID，参数以及本次请求标识ReqID
     2. 若过程ID已注册，执行待调用过程；否则，执行错误处理，返回错误码
     3. 将返回值或错误码序列化为Resp，回调（成功或失败）
  4. 回调：确定Resp长度，将ConnID+ReqID+Resp发送到客户端

### RPC客户端

1. 初始化，`connect`建立连接，保持连接(keep-alive)
2. 向服务器获取连接标识ConnId(特殊请求)
3. 阻塞，直到客户端发起RPC请求(消息队列)
4. 后台处理请求，唯一标识请求ID，序列化参数，调用过程ID
5. 通过建立的连接发送请求，等待响应
6. 解析响应体，若成功，则回调成功，否则回调失败


### 通信数据格式

`4 4 json` 适用于请求和响应。`reqID`与`bodyLength`都是无符号整数。

- 请求:
  ```
  reqID(4) bodyLength(4)
  {
      name: procName,
      args: {...}
  }
  ```
- 响应:
  ```
  reqID(4) bodyLength(4)
  {
      code: 0,
      msg: "",
      ret: {...}
  }
  ```
  响应码: 200(ok), 404(not found), 500(internal error)




## 用例
```
- 客户端 - 获取所有容器信息
    1. RPC连接初始化
    2. 客户端说明 被调用过程名+参数
    3. RPC客户端处理
        1. 解析被调用过程ID，序列化参数Para
        2. 封装ID+Para+ConnID，通过连接发送到RPC服务端
    4. RPC服务的处理
        1. 收到RPC请求，启动新线程处理请求
            1. 接收RPC客户端数据
            2. 解析被调用过程ID，反序列化参数Para，ConnID
            3. 被调用过程尚未注册？
                - 将错误码ErrCode+ConnID封装为响应体，到步骤5
            4. 否则
                - 开启新线程调用该过程
                - 过程结束后序列化返回值RetVal，封装ConnID+ID+RetVal为响应体
            5. 将响应体返回给客户端
```

## 类设计

1. RPCServer
   - data
     - map: name->function
   - method
     - startServer - `listen` and `accept`
     - call - call procedure
     - isProcRegistered
     - registerProc
     - readReq
     - writeReq
     - parse&serialize
2. RPCClient
   - init - `connect`
   - callProcedure - request
   - parse&serialize
3. util
   - parseJson
   - sirializeObject
4. logger & errorHandler


## 实现

使用[nlohmann/json](https://github.com/nlohmann/json)作为参数。
