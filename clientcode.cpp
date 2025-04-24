#include <iostream>
#include <thread>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 12345

void receive_messages(int sock) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            std::cout << "Disconnected from server.\n";
            break;
        }
        std::cout << buffer << std::flush;
    }
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Error creating socket.\n";
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address.\n";
        return 1;
    }

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed.\n";
        return 1;
    }

    char prompt[1024];
    memset(prompt, 0, sizeof(prompt));
    recv(sock, prompt, sizeof(prompt), 0);
    std::cout << prompt << std::flush;

    std::string name;
    std::getline(std::cin, name);
    send(sock, name.c_str(), name.length(), 0);

    std::thread receiver(receive_messages, sock);

    std::string message;
    while (std::getline(std::cin, message)) {
        if (message == "/exit") break;
	message+="\n";
        send(sock, message.c_str(), message.length(), 0);
    }

    close(sock);
    receiver.detach(); 
    return 0;
}

