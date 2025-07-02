#include <iostream>
#include <thread>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

#define PORT 12345
#define BUFFER_SIZE 1024

void receive_messages(int sock) {
    char buffer[BUFFER_SIZE];
    while (true) {
        ssize_t bytes = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) break;
        buffer[bytes] = '\0';
        std::cout << buffer << std::flush;
    }
    close(sock);
    exit(0);
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed\n";
        return 1;
    }

    std::thread(receive_messages, sock).detach();

    char buffer[BUFFER_SIZE];
    while (true) {
        std::cin.getline(buffer, BUFFER_SIZE);
        if (strcmp(buffer, "exit") == 0) break;
        send(sock, buffer, strlen(buffer), 0);
    }

    close(sock);
    return 0;
}
