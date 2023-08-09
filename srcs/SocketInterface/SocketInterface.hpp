#ifndef SOCKET_INTERFACE_HPP
#define SOCKET_INTERFACE_HPP

#include <vector>
#include <string>
#include <sys/poll.h>

class SocketInterface
{
public:
    SocketInterface(const std::vector<std::string>& ports);
    ~SocketInterface();

    void listen();
    void acceptConnection();
    void handleClient(int clientSocket);

private:
    std::vector<int> _sockets;
    struct pollfd* _pollfds;
    int _numPorts;

    void createSockets(const std::vector<std::string>& ports);
    void setupPoll();
};

#endif // SOCKET_INTERFACE_HPP
