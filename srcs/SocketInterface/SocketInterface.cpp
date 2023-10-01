#include "SocketInterface.hpp"
#include "CoreHandler.hpp"
#include "ServerContext.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <fcntl.h>

SocketInterface::SocketInterface(Config *config)
    : _config(config), _numClients(0)
{
    _numPorts = _config->getPorts().size();
    createSockets(_config->getPorts());
    setupPoll();
}

SocketInterface::~SocketInterface()
{
    for (size_t i = 0; i < _sockets.size(); ++i) {
        int socket = _sockets[i];
        close(socket);
    }
}

void SocketInterface::createSockets(const std::vector<std::string> &ports)
{
    for (std::vector<std::string>::const_iterator it = ports.begin(); it != ports.end(); ++it) 
    {
        const std::string& port = *it;
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            std::cerr << "socket() failed" << std::endl;
            exit(1);
        }
        int optval = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
        {
            perror("setsockopt");
            exit(1);
        }
        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(std::stoi(port));
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)))
        {
            perror("bind");
            std::cerr << "bind() failed" << std::endl;
            exit(1);
        }
        if (listen(sockfd, SOMAXCONN)) // 属性変更
        {
            std::cerr << "listen() failed" << std::endl;
            exit(1);
        }
        _sockets.push_back(sockfd);
    }
}

void SocketInterface::setupPoll()
{
    _pollfds.resize(_numPorts);
    for (int i = 0; i < _numPorts; ++i)
    {
        _pollfds[i].fd = _sockets[i];
        _pollfds[i].events = POLLIN;
    }
}

void SocketInterface::addClientsToPollfds(int clientFd)
{
    struct pollfd clientPollfd;
    clientPollfd.fd = clientFd;
    clientPollfd.events = POLLIN;
    _pollfds.push_back(clientPollfd);
}

bool SocketInterface::isListeningSocket(int fd)
{
    return std::find(_sockets.begin(), _sockets.end(), fd) != _sockets.end();
}

void SocketInterface::eventLoop()
{
    while (true)
    {
        int ret = poll(_pollfds.data(), _numPorts + _numClients, -1);
        if (ret > 0)
        {
            for (size_t i = 0; i < _pollfds.size(); i++)
            {
                if (_pollfds[i].revents & POLLIN)
                {
                    if (isListeningSocket(_pollfds[i].fd)) {
                        acceptConnection(_pollfds[i].fd);
                        _numClients++;
                    } else {
                        handleClient(_pollfds[i].fd);
                        _pollfds.erase(_pollfds.begin() + i);
                        _numClients--;
                    }
                }
            }
        }
        else if (ret < 0)
            std::cerr << "poll() returned " << ret << std::endl;
    }
}

void SocketInterface::acceptConnection(int fd)
{
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int clientFd = accept(fd, (struct sockaddr *)&clientAddr, &clientAddrLen);

    if (clientFd >= 0)
    {
        fcntl(clientFd, F_SETFL, O_NONBLOCK, FD_CLOEXEC);
        addClientsToPollfds(clientFd);
    }
    else {
        std::cerr << "accept() returned " << clientFd << std::endl;
        exit(1);
    }
}

std::pair<std::string, std::string> SocketInterface::parseHostAndPortFromRequest(const std::string &request)
{
    std::string hostHeader = "Host: ";
    size_t start = request.find(hostHeader);

    if (start == std::string::npos) return std::make_pair("", ""); // Host header not found
    start += hostHeader.length();
    size_t end = request.find("\r\n", start);
    if (end == std::string::npos) std::make_pair("", ""); // Malformed request

    std::string hostPortStr = request.substr(start, end - start);
    size_t colonPos = hostPortStr.find(':');
    if (colonPos != std::string::npos) {
        return std::make_pair(hostPortStr.substr(0, colonPos), hostPortStr.substr(colonPos + 1));
    } else {
        return std::make_pair(hostPortStr, ""); // No port specified
    }
}

void SocketInterface::handleClient(int clientSocket)
{
    char buffer[DEFAULT_MAX_BUFFER_SIZE];

    ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        std::cout << "Received request:\n"
                << buffer << std::endl;
        std::string request(buffer);

        // ホストとポートの解析
        std::pair<std::string, std::string> hostPort = parseHostAndPortFromRequest(request);

        const ServerContext &serverContext = _config->getServerContext(hostPort.second, hostPort.first);
        CoreHandler coreHandler;
        std::string response = coreHandler.processRequest(buffer, serverContext);

        write(clientSocket, response.c_str(), response.length());
        close(clientSocket);
    }
    else {
        close(clientSocket);
    }
}

// 2メガバイト+1
const int SocketInterface::DEFAULT_MAX_BUFFER_SIZE = 2097153;
