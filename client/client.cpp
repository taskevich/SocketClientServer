#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include <Ws2tcpip.h>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

constexpr int BUFFER_SIZE = 1024;
constexpr int RECONNECT_INTERVAL = 10; // Период повторного подключения (в секундах)

class Client {
public:
    Client(const std::string& serverIP, int serverPort)
        : serverIP(serverIP), serverPort(serverPort), clientSocket(-1) {}

    bool Connect() {
#ifdef _WIN32
        WSADATA wsData;
        if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0) {
            std::cout << "Не удалось инициализировать библиотеку Winsock." << std::endl;
            return false;
        }
#endif

        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == -1) {
            std::cout << "Не удалось создать сокет." << std::endl;
            return false;
        }

        sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(serverPort);
        inet_pton(AF_INET, serverIP.c_str(), &(serverAddress.sin_addr));

        if (connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
            std::cout << "Не удалось подключиться к серверу." << std::endl;
            return false;
        }

        std::cout << "Успешное подключение к серверу." << std::endl;
        return true;
    }

    void ReceiveAndPrint() {
        while (true) {
            if (clientSocket != -1) {
                char buffer[BUFFER_SIZE];
                memset(buffer, 0, sizeof(buffer));
                int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
                if (bytesRead > 0) {
                    std::cout << "Получено сообщение от сервера: " << buffer << std::endl;
                }
                else {
                    std::cout << "Соединение с сервером разорвано. Повторная попытка подключения будет выполнена через "
                        << RECONNECT_INTERVAL << " секунд." << std::endl;
                    Reconnect();
                }
            }
            else {
                std::cout << "Не удалось установить соединение с сервером. Повторная попытка подключения будет выполнена через "
                    << RECONNECT_INTERVAL << " секунд." << std::endl;
                Reconnect();
            }
        }
    }


    void Reconnect() {
        while (true) {
            Disconnect();
            std::this_thread::sleep_for(std::chrono::seconds(RECONNECT_INTERVAL));
            if (Connect()) {
                break;
            }
        }
    }

    void Disconnect() {
        if (clientSocket != -1) {
#ifdef _WIN32
            closesocket(clientSocket);
            WSACleanup();
#else
            close(clientSocket);
#endif
        }
    }

private:
    std::string serverIP;
    int serverPort;
    int clientSocket;
};

int main() {
    std::string serverIP = "127.0.0.1";
    int serverPort = 8000;

    Client client(serverIP, serverPort);

    if (!client.Connect()) {
        std::cout << "Не удалось подключиться к серверу. Повторная попытка подключения будет выполнена каждые " << RECONNECT_INTERVAL << " секунд." << std::endl;
        client.Reconnect();
    }

    while (true) {
        client.ReceiveAndPrint();
    }

    client.Disconnect();

    return 0;
}
