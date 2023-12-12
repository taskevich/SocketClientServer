#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include <Ws2tcpip.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

constexpr int BUFFER_SIZE = 1024;
constexpr int RECONNECT_INTERVAL = 10; // Период повторного подключения (в секундах)

class Client {
public:
    Client(const std::string& serverIP, int serverPort)
        : serverIP(serverIP), serverPort(serverPort), clientSocket(-1) {}

    bool Connect() {
        WSADATA wsData;
        if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0) {
            std::cout << "Error initializing WinSock." << std::endl;
            return false;
        }

        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == -1) {
            std::cout << "Error creating socket." << std::endl;
            return false;
        }

        sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(serverPort);
        inet_pton(AF_INET, serverIP.c_str(), &(serverAddress.sin_addr));

        if (connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
            std::cout << "Error connecting to server." << std::endl;
            return false;
        }

        std::cout << "Connection to the server was successfull." << std::endl;
        return true;
    }

    void Receive() {
        while (true) {
            if (clientSocket != -1) {
                char buffer[BUFFER_SIZE];
                memset(buffer, 0, sizeof(buffer));
                int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
                if (bytesRead > 0) {
                    std::cout << "New message from server: " << buffer << std::endl;
                }
                else {
                    std::cout << "Connection was interrut. Reconnect via "
                        << RECONNECT_INTERVAL << " seconds." << std::endl;
                    Reconnect();
                }
            }
            else {
                std::cout << "Connection error. Reconnect via "
                    << RECONNECT_INTERVAL << " seconds." << std::endl;
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
            closesocket(clientSocket);
            WSACleanup();
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
        std::cout << "Error connection to server. Reconnect via " << RECONNECT_INTERVAL << " seconds." << std::endl;
        client.Reconnect();
    }

    while (true) {
        client.Receive();
    }

    client.Disconnect();

    return 0;
}
