#include "SocketServer.h"

SocketServer::SocketServer(int port_server) {
    addr_server_.sin_addr.s_addr = INADDR_ANY;
    addr_server_.sin_port = htons(port_server);
    addr_server_.sin_family = AF_INET;
}

void SocketServer::Connect() {
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    bind(socket_, (sockaddr*)&addr_server_, sizeof(addr_server_));
    listen(socket_, SOMAXCONN);
    socklen_t addr_len = sizeof(addr_client_);
    socket_ = accept(socket_, (sockaddr *)&addr_client_, &addr_len);
}

void SocketServer::SendText(const char* text) {
    send(socket_, text, std::strlen(text), 0);
}

void SocketServer::RecvText(char* text) {
    int recv_size = recv(socket_, text, 1024, 0);
    text[recv_size] = '\0';
}
