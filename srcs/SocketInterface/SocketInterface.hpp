#ifndef SOCKETINTERFACE_HPP
#define SOCKETINTERFACE_HPP

#include "Config.hpp"
#include <vector>
#include <map>
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
    WAIT_CGI,
    WRITE_REQUEST_ERROR,
};

struct RequestBuffer
{
    std::string request;
    int clientFd; // CGIの場合のみ使用 (CGIの結果を返すため)
    int cgiFd; // CGIの場合のみ使用 (CGIを削除するため)
    pid_t cgiPid; // CGIの場合のみ使用 (CGIを削除するため)
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
    void pushDelPollFd(int fd, int index);

private:
    Config *_config;
    std::vector<int> _sockets;
    std::vector<struct pollfd> _pollFds;
    std::vector<struct pollfd> _addPollFds;
    std::vector<int> _delIndex;
    int _numPorts;
    int _numClients;
     
    // buffer
    std::map<int, RequestBuffer> _clients;

    void createSockets(const std::vector<std::string> &ports);
    void setupPoll();
    HttpRequest parseRequest(std::string request);
    void deleteClient();
    int sendResponse(int fd, std::string response);
    void ReadRequest(int fd, RequestBuffer &client);
    RequestBuffer createRequestBuffer();
    void execReadRequest(pollfd &pollfd, RequestBuffer &client);
    void execCoreHandler(pollfd &pollfd, RequestBuffer &client);
    void execCgi(pollfd &pollfd, RequestBuffer &client);
    void execReadCgi(pollfd &pollFd, RequestBuffer &client);
    void execWriteCgi(pollfd &pollFd, RequestBuffer &client);
    void execWriteError(pollfd &pollFd, int index);
    pollfd createClient(int fd, State state);
};

#endif