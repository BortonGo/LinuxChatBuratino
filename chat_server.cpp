#include <iostream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <algorithm>
#include <unistd.h>
#include <netinet/in.h>
#include <cstring>

#define PORT 12345
#define BUFFER_SIZE 1024

std::unordered_map<std::string, std::string> user_db = {
    {"Buratino", "012345"},
    {"Lisa", "qwerty12"},
    {"PapaCarlo", "passwd"}
};

std::unordered_map<std::string, int> online_users;
std::mutex users_mutex;

void send_message(int sock, const std::string& msg) {
    send(sock, msg.c_str(), msg.size(), 0);
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    std::string username;

    send_message(client_socket, "Login: \n");
    ssize_t len = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (len <= 0) return;
    buffer[len] = '\0';
    std::string login(buffer);

    send_message(client_socket, "Password: \n");
    len = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (len <= 0) return;
    buffer[len] = '\0';
    std::string password(buffer);

    if (user_db.find(login) == user_db.end() || user_db[login] != password) {
        send_message(client_socket, "Auth failed\n");
        close(client_socket);
        return;
    }

    username = login;
    {
        std::lock_guard<std::mutex> lock(users_mutex);
        online_users[username] = client_socket;
    }

    std::string welcome = "Welcome, " + username + "! You can now send messages in format @user message\n";
    send_message(client_socket, welcome);

    while (true) {
        len = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (len <= 0) break;
        buffer[len] = '\0';
        std::string input(buffer);

        if (input.rfind("@", 0) == 0) {
            auto space_pos = input.find(" ");
            if (space_pos != std::string::npos) {
                std::string target = input.substr(1, space_pos - 1);
                std::string message = input.substr(space_pos + 1);
                std::lock_guard<std::mutex> lock(users_mutex);
                if (online_users.find(target) != online_users.end()) {
                    std::string full_msg = "(Private) " + username + ": " + message + "\n";
                    send_message(online_users[target], full_msg);
                } else {
                    send_message(client_socket, "User not found or offline.\n");
                }
            } else {
                send_message(client_socket, "Invalid format. Use @username message\n");
            }
        } else {
            send_message(client_socket, "Use format: @username message\n");
        }
    }

    {
        std::lock_guard<std::mutex> lock(users_mutex);
        online_users.erase(username);
    }

    close(client_socket);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);
    std::cout << "Private Message Chat Server started on port " << PORT << ".\n";

    while ((client_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) >= 0) {
        std::thread(handle_client, client_socket).detach();
    }

    close(server_fd);
    return 0;
}
