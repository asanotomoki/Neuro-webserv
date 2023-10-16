#ifndef SOCKETINTERFACE_HPP
#define SOCKETINTERFACE_HPP

#include "Config.hpp"
#include <vector>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include "RequestParser.hpp"
#include "ServerContext.hpp"
#include "Cgi.hpp"

enum State
{
    READ_REQUEST,
    READ_CGI,
    WRITE_RESPONSE,
    WITE_TO_CGI,
    CLOSE_CONNECTION,
    EXEC_CGI,
    WRITE_CGI,
    WAIT_CGI
};

struct RequestBuffer
{
    std::string request;
    int fd;
    int cgiFd; // CGIの場合のみ使用 (CGIの結果を返すため)
    pollfd* clientPollFd; // CGIの場合のみ使用 (CGIの結果を返すため)
    State state;
    bool isRequestFinished;
    HttpRequest httpRequest;
    ServerContext serverContext;
    std::string response;
};

// 脊髄クラス
class SocketInterface
{
public:
    SocketInterface(Config *config);
    ~SocketInterface();

    void eventLoop();
    void acceptConnection(int fd);

private:
    Config *_config;
    std::vector<int> _sockets;
    std::vector<struct pollfd> _pollFds;
    int _numPorts;
    int _numClients;
    // buffer
    std::vector<RequestBuffer> _clients;

    void createSockets(const std::vector<std::string> &ports);
    void setupPoll();
    HttpRequest parseRequest(std::string request);
    void sendResponse(int fd, std::string response);
    void ReadRequest(int fd, RequestBuffer &client);
    RequestBuffer createRequestBuffer();
    void execReadRequest(pollfd &pollfd, RequestBuffer &client);
    void execCoreHandler(pollfd &pollfd, RequestBuffer &client);
    void execCgi(pollfd &pollfd, RequestBuffer &client);
    void execReadCgi(pollfd &pollFd, RequestBuffer &client);
    void execWriteCgi(pollfd pollFd, RequestBuffer &client);
    pollfd createPollFd(int fd);
};

#endif