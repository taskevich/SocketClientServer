#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

constexpr int MAX_CLIENTS = 10;
constexpr int BUFFER_SIZE = 1024;

// Структура, представляющая клиента
struct Client {
    int id;
    int socket;
    sockaddr_in address;

    explicit Client(int id, int socket, sockaddr_in address)
        : id(id), socket(socket), address(address) {}
};

// Класс сервера
class Server {
public:
    Server(int port) : port(port) {
        clients.reserve(MAX_CLIENTS);
    }

    void Start() {
        if (!Initialize()) {
            std::cout << "Ошибка инициализации сервера." << std::endl;
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
#ifdef _WIN32
        WSADATA wsData;
        if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0) {
            std::cout << "Не удалось инициализировать библиотеку Winsock." << std::endl;
            return false;
        }
#endif

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
            std::cout << "Не удалось привязать сокет к адресу и порту." << std::endl;
            return false;
        }

        if (listen(serverSocket, MAX_CLIENTS) == -1) {
            std::cout << "Не удалось установить сокет в режим прослушивания." << std::endl;
            return false;
        }

        std::cout << "Сервер запущен. Ожидание клиентов..." << std::endl;

        return true;
    }

    void Cleanup() {
#ifdef _WIN32
        closesocket(serverSocket);
        WSACleanup();
#else
        close(serverSocket);
#endif
    }

    void AcceptClients() {
        while (true) {
            sockaddr_in clientAddress{};
            int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, nullptr);
            if (clientSocket == -1) {
                std::cout << "Ошибка при принятии клиента." << std::endl;
                continue;
            }

            std::lock_guard<std::mutex> lock(clientsMutex);

            int clientId = clients.size();
            clients.emplace_back(clientId, clientSocket, clientAddress);

            std::thread clientThread(&Server::HandleClient, this, clientId);
            clientThread.detach();

            std::cout << "Подключен новый клиент. ID: " << clientId << std::endl;
        }
    }

    void HandleClient(int clientId) {
        Client& client = clients[clientId];
        char buffer[BUFFER_SIZE];

        while (true) {
            int bytesRead = recv(client.socket, buffer, BUFFER_SIZE, 0);
            if (bytesRead <= 0) {
                std::cout << "Клиент отключен. ID: " << clientId << std::endl;
                break;
            }

            std::string message(buffer, bytesRead);
            std::cout << "Получено сообщение от клиента " << clientId << ": " << message << std::endl;
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
                    std::cout << "Неверный идентификатор клиента." << std::endl;
                }
            }
            else {
                std::cout << "Некорректная команда." << std::endl;
            }
        }
    }
};

int main() {
    Server server(8000);
    server.Start();
    return 0;
}
