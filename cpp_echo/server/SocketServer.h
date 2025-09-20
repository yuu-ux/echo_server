#ifndef SOCKETSERVER_H
# define SOCKETSERVER_H

#include <iostream>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class SocketServer {
    private:
        int socket_;
        struct sockaddr_in addr_server_;
        struct sockaddr_in addr_client_;

    public:
        SocketServer(int port_server);
        void Connect();
        void SendText(const char* text);
        void RecvText(char *text);
};

#endif
