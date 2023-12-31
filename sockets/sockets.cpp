﻿#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <cstring>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

constexpr int MAX_CLIENTS = 10;
constexpr int BUFFER_SIZE = 1024;

// Структура клиента
struct Client {
    int id;
    int socket;
    sockaddr_in address;

    explicit Client(int id, int socket, sockaddr_in address)
        : id(id), socket(socket), address(address) {}
};

// Сервер
class Server {
public:
    Server(int port) : port(port) {
        clients.reserve(MAX_CLIENTS);
    }

    void Start() {
        if (!Initialize()) {
            std::cout << "Error server initialization." << std::endl;
            return;
        }

        std::thread acceptThread(&Server::AcceptClients, this);
        std::thread commandThread(&Server::HandleCommands, this);

        acceptThread.join();
        commandThread.join();

        Cleanup();
    }

private:
    int port;
    int serverSocket;
    std::vector<Client> clients;
    std::mutex clientsMutex;
    std::condition_variable commandCV;
    std::string command;

    bool Initialize() {
        WSADATA wsData;
        if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0) {
            std::cout << "Не удалось инициализировать библиотеку Winsock." << std::endl;
            return false;
        }

        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == -1) {
            std::cout << "Не удалось создать сокет." << std::endl;
            return false;
        }

        sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = INADDR_ANY;
        serverAddress.sin_port = htons(port);

        if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
            std::cout << "Error to link socket to host." << std::endl;
            return false;
        }

        if (listen(serverSocket, MAX_CLIENTS) == -1) {
            std::cout << "Error to setup the socket on listening." << std::endl;
            return false;
        }

        std::cout << "Server started. Waiting for clients..." << std::endl;

        return true;
    }

    void Cleanup() {
        closesocket(serverSocket);
        WSACleanup();
    }

    void AcceptClients() {
        while (true) {
            sockaddr_in clientAddress{};
            int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, nullptr);
            if (clientSocket == -1) {
                std::cout << "Error client connection." << std::endl;
                continue;
            }

            std::lock_guard<std::mutex> lock(clientsMutex);

            int clientId = clients.size();
            clients.emplace_back(clientId, clientSocket, clientAddress);

            std::thread clientThread(&Server::HandleClient, this, clientId);
            clientThread.detach();

            std::cout << "New client. ID: " << clientId << std::endl;
        }
    }

    void HandleClient(int clientId) {
        Client& client = clients[clientId];
        char buffer[BUFFER_SIZE];

        while (true) {
            int bytesRead = recv(client.socket, buffer, BUFFER_SIZE, 0);
            if (bytesRead <= 0) {
                std::cout << "Client disconnected. ID: " << clientId << std::endl;
                break;
            }
        }

        std::lock_guard<std::mutex> lock(clientsMutex);
        closesocket(client.socket);
        clients.erase(clients.begin() + clientId);
    }

    void HandleCommands() {
        while (true) {
            std::string input;
            std::getline(std::cin, input);

            std::size_t pos = input.find(':');
            if (pos != std::string::npos && input.substr(0, pos) == "sendto") {
                int clientId = std::stoi(input.substr(pos + 1));
                std::string command = input.substr(input.find(':', pos + 1) + 1);

                std::lock_guard<std::mutex> lock(clientsMutex);
                if (clientId >= 0 && clientId < clients.size()) {
                    Client& client = clients[clientId];
                    send(client.socket, command.c_str(), command.length(), 0);
                }
                else {
                    std::cout << "Incorrect inditifier of client." << std::endl;
                }
            }
            else {
                std::cout << "Incorrect command." << std::endl;
            }
        }
    }
};

int main() {
    Server server(8000);
    server.Start();
    return 0;
}
