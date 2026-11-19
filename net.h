#include <string>
#include <cstdint>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_MSG 1024

// from file transfer lab, sends every byte
ssize_t send_all(int sock, const void* buffer, size_t length) {
    const char* p = (const char*)buffer;
    size_t total = 0;
    while (total < length) {
        ssize_t n = send(sock, p + total, length - total, MSG_NOSIGNAL);
        if (n < 0) return -1;
        total += n;
    }
    return total;
}

// from file transfer lab, reads every byte
ssize_t recv_all(int sock, void* buffer, size_t length) {
    char* p = (char*)buffer;
    size_t total = 0;
    while (total < length) {
        ssize_t n = recv(sock, p + total, length - total, 0);
        if (n < 0) return -1;
        if (n == 0) return 0;
        total += n;
    }
    return total;
}

// new, sends [4 byte length][text]
bool send_msg(int sock, const std::string& s) {
    uint32_t n = htonl(s.size());
    if (send_all(sock, &n, 4) < 0) return false;
    return send_all(sock, s.data(), s.size()) >= 0;
}

// new, reads [4 byte length][text]
bool recv_msg(int sock, std::string& s) {
    uint32_t n;
    if (recv_all(sock, &n, 4) <= 0) return false;
    n = ntohl(n);
    if (n > MAX_MSG) return false;  // too big, check before reading
    s.resize(n);
    if (n == 0) return true;
    return recv_all(sock, &s[0], n) > 0;
}
