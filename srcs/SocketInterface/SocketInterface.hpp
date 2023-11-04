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
#include "ServerContext.hpp"
#include "Cgi.hpp"
#include "RequestParser.hpp"
#include "CgiParser.hpp"
#include "utils.hpp"
#include <ctime>
#include <fstream>
#include <iterator>
#include <dirent.h>
#define TIMEOUT 15
#define MAX_LOCAL_REDIRECT_COUNT 10


enum State
{
    READ_REQUEST,
    READ_CGI,
    WRITE_RESPONSE,
    WITE_TO_CGI,
    CLOSE_CONNECTION,
    EXEC_CGI,
    EXEC_CORE_HANDLER,
    WRITE_CGI,
    WAIT_CGI,
    WRITE_REQUEST_ERROR,
    WRITE_CGI_BODY,
    WAIT_CGI_CHILD,
};

struct RequestBuffer
{
    std::string request;
    int clientFd; // CGIの場合のみ使用 (CGIの結果を返すため)
    int cgiFd;    // CGIの場合のみ使用 (CGIを削除するため)
    pid_t cgiPid; // CGIの場合のみ使用 (CGIを削除するため)
    int cgiLocalRedirectCount;
    State state;
    bool isRequestFinished;
    HttpRequest httpRequest;
    bool isChunked;
    ssize_t chunkedSize;
    std::string chunkedBody;
    ServerContext serverContext;
    std::string response;
    Cgi cgi;
    std::pair<std::string, std::string> hostAndPort;
    bool isClosed;
    std::time_t lastAccessTime;
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
    void setCgiBody(RequestBuffer &client, std::string &body);
    HttpRequest parseRequest(std::string request, RequestBuffer &client);
    void deleteClient();
    int sendResponse(int fd, std::string response);
    void execReadRequest(pollfd &pollfd, RequestBuffer &client);
    void execParseRequest(pollfd &pollfd, RequestBuffer &client, std::string request);
    void execCoreHandler(pollfd &pollfd, RequestBuffer &client);
    void execCgi(pollfd &pollfd, RequestBuffer &client);
    std::string parseCgiResponse(std::string response, std::string method, RequestBuffer &client);
    void execReadCgi(pollfd &pollFd, RequestBuffer &client);
    void execWaitCgiChild(pollfd &pollFd, RequestBuffer &client);
    void execWriteCgi(pollfd &pollFd, RequestBuffer &client);
    void execWriteError(pollfd &pollFd, RequestBuffer &client, int index);
    void execWriteCGIBody(pollfd &pollFd, RequestBuffer &client);
    void execWriteResponse(pollfd &pollFd, RequestBuffer &client);
    pollfd createClient(int fd, State state);

    std::string getErrorPage(int status, const  std::pair<std::string, std::string> &hostAndPort);
};

#endif