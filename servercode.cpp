#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 12345

struct Client {
    int socket;
    std::string name;
};

std::vector<Client> clients;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast(const std::string& message, int sender_fd) {
    pthread_mutex_lock(&clients_mutex);
    for (const auto& client : clients) {
        if (client.socket != sender_fd) {
            send(client.socket, message.c_str(), message.length(), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void* handle_client(void* arg) {
    int client_fd = *((int*)arg);
    delete (int*)arg;

    char buffer[1024];
    std::string name;

    // Ask for name
    const char* prompt = "Enter your name: ";
    send(client_fd, prompt, strlen(prompt), 0);

    memset(buffer, 0, sizeof(buffer));
    int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
        close(client_fd);
        return nullptr;
    }

    name = std::string(buffer, bytes_received);
    name.erase(name.find_last_not_of(" \n\r\t") + 1); 

    pthread_mutex_lock(&clients_mutex);
    clients.push_back({client_fd, name});
    pthread_mutex_unlock(&clients_mutex);

    std::string join_msg = "[" + name + "] joined the chat.\n";
    broadcast(join_msg, client_fd);

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) break;

        std::string message = "[" + name + "]: " + std::string(buffer, bytes_received);
        broadcast(message, client_fd);
    }

    close(client_fd);
    pthread_mutex_lock(&clients_mutex);
    clients.erase(std::remove_if(clients.begin(), clients.end(),
        [client_fd](const Client& c) { return c.socket == client_fd; }), clients.end());
    pthread_mutex_unlock(&clients_mutex);

    std::string leave_msg = "[" + name + "] left the chat.\n";
    broadcast(leave_msg, -1);

    return nullptr;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr{}, client_addr{};
    socklen_t addr_len = sizeof(client_addr);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, 10);

    std::cout << "Server running on port " << PORT << "...\n";

    while (true) {
        int* client_fd = new int;
        *client_fd = accept(server_fd, (sockaddr*)&client_addr, &addr_len);
        if (*client_fd < 0) {
            delete client_fd;
            continue;
        }

        pthread_t thread;
        pthread_create(&thread, nullptr, handle_client, client_fd);
        pthread_detach(thread);
    }

    close(server_fd);
    return 0;
}

