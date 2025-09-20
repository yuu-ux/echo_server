#include "SocketClient.h"
#include <thread>
#include <chrono>

int count = 0;

void SendTask(SocketClient* sock) {
    for (;;) {
        std::string text = "client->server" + std::to_string(count);
        count++;
        sock->SendText(text.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void RecvTask(SocketClient* sock) {
    for (;;) {
        char text[1024];
        sock->RecvText(text);
        std::cout << "Recive: " << text << std::endl;
    }
}

int main(void) {
    std::cout << "Communicate with TCP'/IP!" << std::endl;

    int port_server = 2001;
    const char* ip_server = "127.0.0.1";
    SocketClient socket_client(ip_server, port_server);
    socket_client.Connect();
    std::thread thread_send(SendTask, &socket_client);
    std::thread thread_recv(RecvTask, &socket_client);
    thread_send.join();
    thread_recv.join();
}
