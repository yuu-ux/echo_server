#include "SocketServer.h"
#include <string>
#include <sys/socket.h>
#include <thread>
#include <chrono>

int c = 0;

void SendTask(SocketServer *sock) {
    for (;;) {
        std::string text = "server->client" + std::to_string(c);
        c++;
        sock->SendText(text.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void RecvTask(SocketServer* sock) {
    for (;;) {
        char text[1024];
        sock->RecvText(text);
        std::cout << "Recive: " << text << std::endl;
    }
}

int main(void) {
    std::cout << "Communicate with TCP/IP!" << std::endl;
    int socket_port = 2001;
    SocketServer socket_server(socket_port);
    socket_server.Connect();

    std::thread thread_send(SendTask, &socket_server);
    std::thread thread_recv(RecvTask, &socket_server);
    thread_send.join();
    thread_recv.join();
}
