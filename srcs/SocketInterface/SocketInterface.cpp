#include "SocketInterface.hpp"
#include "ApplicationServer.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

SocketInterface::SocketInterface(const std::vector<std::string>& ports)
    : _numPorts(ports.size())
{
    createSockets(ports);
    setupPoll();
}

SocketInterface::~SocketInterface()
{
    for (size_t i = 0; i < _sockets.size(); ++i) {
        int socket = _sockets[i];
        close(socket);
    }
    delete[] _pollfds;
}

void SocketInterface::createSockets(const std::vector<std::string>& ports)
{
    for (std::vector<std::string>::const_iterator it = ports.begin(); it != ports.end(); ++it) 
    {
        const std::string& port = *it;
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(std::stoi(port));
        addr.sin_addr.s_addr = INADDR_ANY;
        
        bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
        ::listen(sockfd,4); // Allow up to 5 pending connections

        _sockets.push_back(sockfd);
    }
}

void SocketInterface::setupPoll()
{
    _pollfds = new struct pollfd[_numPorts];
    for (int i = 0; i < _numPorts; ++i)
    {
        _pollfds[i].fd = _sockets[i];
        _pollfds[i].events = POLLIN;
    }
}

void SocketInterface::listen()
{
    while (true)
    {
        int ret = poll(_pollfds, _numPorts, -1);
        if (ret > 0)
        {
            for (int i = 0; i < _numPorts; i++)
            {
                if (_pollfds[i].revents & POLLIN)
                {
                    acceptConnection();
                }
            }
        }
        // エラー処理
    }
}

void SocketInterface::acceptConnection()
{
    for (int i = 0; i < _numPorts; i++)
    {
        if (_pollfds[i].revents & POLLIN)
        {
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);
            int clientFd = accept(_pollfds[i].fd, (struct sockaddr*)&clientAddr, &clientAddrLen);

            if (clientFd >= 0)
            {
                handleClient(clientFd);
            }
            // エラー処理
        }
    }
}

void SocketInterface::handleClient(int clientSocket)
{
    char buffer[1024];
    ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
    
    if (bytesRead < 0) {
        // エラー処理
        return;
    }

    buffer[bytesRead] = '\0'; // NULL終端

    ApplicationServer appServer;
    std::string response = appServer.processRequest(buffer);
    
    write(clientSocket, response.c_str(), response.length()); // レスポンスの送信
    close(clientSocket); // ソケットのクローズ
}
