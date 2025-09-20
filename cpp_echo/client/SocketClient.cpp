#include "SocketClient.h"

SocketClient::SocketClient(const char* ip_server, int port_server) {
    addr_server_.sin_addr.s_addr = inet_addr(ip_server);
    addr_server_.sin_port = htons(port_server);
    addr_server_.sin_family = AF_INET;
}

void SocketClient::Connect() {
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    connect(socket_, (sockaddr *)&addr_server_, sockaddr_len_);
}

void SocketClient::SendText(const char* text) {
    send(socket_, text, std::strlen(text), 0);
}

void SocketClient::RecvText(char* text) {
    ssize_t recv_size = recv(socket_, text, 1024, 0);
    text[recv_size] = '\0';
}
