#include "SocketInterface.hpp"
#include "ApplicationServer.hpp"
#include "HttpContext.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

SocketInterface::SocketInterface(Config* config)
    : _config(config)
{
    _numPorts = config->getPorts().size();
    createSockets(config->getPorts());
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

std::pair<std::string, std::string> SocketInterface::parseHostAndPortFromRequest(const std::string& request) {
    std::string hostHeader = "Host: ";
    size_t start = request.find(hostHeader);
    if (start == std::string::npos) return {"", ""}; // Host header not found
    start += hostHeader.length();
    size_t end = request.find("\r\n", start);
    if (end == std::string::npos) return {"", ""}; // Malformed request

    std::string hostPortStr = request.substr(start, end - start);
    size_t colonPos = hostPortStr.find(':');
    if (colonPos != std::string::npos) {
        return {hostPortStr.substr(0, colonPos), hostPortStr.substr(colonPos + 1)};
    } else {
        return {hostPortStr, ""}; // No port specified
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

    buffer[bytesRead] = '\0';
    std::string request(buffer);

    // ホストとポートの解析
    std::pair<std::string, std::string> hostPort = parseHostAndPortFromRequest(request);

    // ここで、ホストとポートを使用して、適切なServerContextを取得
    const ServerContext& serverContext = _config->getHttpContext().getServerContext(hostPort.second, hostPort.first);

    // responseの生成
    ApplicationServer appServer;
    std::string response = appServer.processRequest(buffer, serverContext);
 
    write(clientSocket, response.c_str(), response.length()); // レスポンスの送信
    close(clientSocket); // ソケットのクローズ
}

