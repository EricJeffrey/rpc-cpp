""" temporary rpc client """
import socket
from socket import AF_INET, SOCK_STREAM
byteorder = "big"


def connect():
    port = 5656
    host = "localhost"
    s = socket.socket(AF_INET, SOCK_STREAM)
    s.connect((host, port))
    reqID = 2
    body = b'''{"name": "test", "args": {}}'''
    data = reqID.to_bytes(4, byteorder)
    data += len(body).to_bytes(4, byteorder)
    data += body
    s.sendall(data)
    resp = s.recv(2048)
    print(resp)
    pass


if __name__ == "__main__":
    connect()
