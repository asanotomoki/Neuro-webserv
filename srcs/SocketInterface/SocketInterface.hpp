#ifndef SOCKETINTERFACE_HPP
#define SOCKETINTERFACE_HPP

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

        void eventLoop();
        void acceptConnection(int fd);
        void handleClient(int clientSocket);

    private:
        Config* _config;
        std::vector<int> _sockets;
        std::vector<struct pollfd> _pollfds;
        int _numPorts;
        int _numClients;

        void addClientsToPollfds(int clientFd);
        bool isListeningSocket(int fd);
        void createSockets(const std::vector<std::string>& ports);
        void setupPoll();
        std::pair<std::string, std::string> parseHostAndPortFromRequest(const std::string& request);

        static const int DEFAULT_MAX_BUFFER_SIZE;
};

#endif
