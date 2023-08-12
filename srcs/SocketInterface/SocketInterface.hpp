#ifndef SOCKET_INTERFACE_HPP
#define SOCKET_INTERFACE_HPP

#include "Config.hpp"
#include <vector>
#include <string>
#include <sys/poll.h>

//脊髄クラス
class SocketInterface
{
public:
    SocketInterface(Config* config);
    ~SocketInterface();

    void listen();
    void acceptConnection();
    void handleClient(int clientSocket);

private:
    Config* _config;
    std::vector<int> _sockets;
    struct pollfd* _pollfds;
    int _numPorts;

    void createSockets(const std::vector<std::string>& ports);
    void setupPoll();
    std::pair<std::string, std::string> parseHostAndPortFromRequest(const std::string& request);
};

#endif // SOCKET_INTERFACE_HPP
