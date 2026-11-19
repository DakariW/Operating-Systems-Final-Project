#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <ctime>
#include <map>
#include "net.h"

// counter and its lock
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
int total_connections = 0;

// who is online, fd -> user id
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
std::map<int, int> clients;
int next_id = 101;

int delay_ms = 0;  // so i have time

// like 11/19/26 4:15 PM
std::string timestamp() {
    char buf[32];
    time_t t = time(nullptr);
    strftime(buf, sizeof(buf), "%m/%d/%y %-I:%M %p", localtime(&t));
    return buf;
}

// sends to everyone online
void broadcast(const std::string& s) {
    pthread_mutex_lock(&list_mutex);
    for (auto& c : clients)
        send_msg(c.first, s);
    pthread_mutex_unlock(&list_mutex);
}

// runs in its own thread per client
void* handle_client(void* arg) {
    int client_fd = *(int*)arg;
    free(arg);  // Free the allocated int

    usleep(delay_ms * 1000);

    pthread_mutex_lock(&list_mutex);
    int id = next_id++;
    clients[client_fd] = id;
    send_msg(client_fd, "You are user " + std::to_string(id));
    pthread_mutex_unlock(&list_mutex);

    std::string name = "(" + std::to_string(id) + ") ";
    broadcast(name + "joined " + timestamp());

    std::string msg;
    while (recv_msg(client_fd, msg)) {  // keep reading till client is done
        std::cout << "Received: " << name << msg << std::endl;
        broadcast(name + msg + " " + timestamp());
    }

    pthread_mutex_lock(&list_mutex);
    clients.erase(client_fd);
    pthread_mutex_unlock(&list_mutex);
    close(client_fd);

    broadcast(name + "left " + timestamp());
    std::cout << "User " << id << " left" << std::endl;
    return nullptr;
}

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) port = atoi(argv[1]);
    if (port < 1 || port > 65535) {  // make sure port is real
        std::cerr << "Invalid port. Use 1-65535." << std::endl;
        return 1;
    }
    if (argc > 2) delay_ms = atoi(argv[2]);
    signal(SIGPIPE, SIG_IGN);

    // same setup as part 1
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 128) < 0) {  // 5 was too small
        perror("listen");
        return 1;
    }

    std::cout << "Chat server listening on port " << port << std::endl;

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        std::cout << "Client connected from " << inet_ntoa(client_addr.sin_addr) << std::endl;

        // lock so the count stays right
        pthread_mutex_lock(&counter_mutex);
        total_connections++;
        std::cout << "Total connections: " << total_connections << std::endl;
        pthread_mutex_unlock(&counter_mutex);

        int* client_ptr = (int*)malloc(sizeof(int));
        *client_ptr = client_fd;

        pthread_t thread;
        if (pthread_create(&thread, nullptr, handle_client, client_ptr) != 0) {
            perror("pthread_create");
            close(client_fd);
            free(client_ptr);
            continue;
        }
        pthread_detach(thread);  // No need to join
    }

    close(server_fd);
    return 0;
}
