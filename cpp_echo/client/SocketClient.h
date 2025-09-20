#ifndef SOCKETCLIENT_H
# define SOCKETCLIENT_H

#include <iostream>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class SocketClient {
    private:
        int socket_;
        int sockaddr_len_ = sizeof(struct sockaddr_in);
        struct sockaddr_in addr_server_;

    public:
        SocketClient(const char* ip_server, int port_server);
        void Connect();
        void SendText(const char* text);
        void RecvText(char* text);
};

#endif
