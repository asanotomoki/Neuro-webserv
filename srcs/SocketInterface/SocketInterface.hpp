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

enum State {
    READ_REQUEST,
    READ_CGI,
    WRITE_RESPONSE,
    WITE_TO_CGI,
    CLOSE_CONNECTION,
    PORT
};

struct RequestBuffer {
    std::string request;
    int fd;
    State state;
    bool isRequestFinished;
};

// 脊髄クラス
class SocketInterface
{
	 public:
        SocketInterface(Config* config);
        ~SocketInterface();

        void eventLoop();
        void acceptConnection(int fd);
		

	private:
		Config* _config;
        std::vector<int> _sockets;
        std::vector<struct pollfd> _pollFds;
        int _numPorts;
        int _numClients;
        // buffer
        std::vector<RequestBuffer> _clients;

		void createSockets(const std::vector<std::string>& ports);
        void setupPoll();
        HttpRequest parseRequest(int fd);
        void sendResponse(int fd);
        void ReadRequest(int fd, RequestBuffer& client);
};

#endif