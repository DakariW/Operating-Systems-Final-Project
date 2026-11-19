#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "net.h"

int client_fd;

// prints everything the server sends
void* receive_loop(void*) {
    std::string msg;
    while (recv_msg(client_fd, msg))
        std::cout << msg << std::endl;
    std::cout << "Server closed the connection." << std::endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <host> <port>" << std::endl;
        return 1;
    }
    std::string host = argv[1];
    int port = atoi(argv[2]);
    if (port < 1 || port > 65535) {  // make sure port is real
        std::cerr << "Invalid port '" << argv[2] << "'. Use a number from 1 to 65535." << std::endl;
        return 1;
    }
    if (host == "localhost") host = "127.0.0.1";  // inet_pton can't read localhost

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {  // bad address
        std::cerr << "Invalid address '" << argv[1] << "'" << std::endl;
        return 1;
    }

    if (connect(client_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        std::cerr << "Nothing is listening on " << argv[1] << ":" << port << ". Is the server running?" << std::endl;
        return 1;
    }

    pthread_t t;
    pthread_create(&t, nullptr, receive_loop, nullptr);

    std::string line;
    while (std::getline(std::cin, line)) {  // send each line you type
        if (line == "/quit") break;
        if (!send_msg(client_fd, line)) break;
    }

    close(client_fd);
    return 0;
}
