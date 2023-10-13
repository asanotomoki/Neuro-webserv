#include "SocketInterface.hpp"
#include "ServerContext.hpp"
#include "Config.hpp"
#include "RequestParser.hpp"

SocketInterface::SocketInterface(Config *config)
	: _config(config), _numClients(0)
{
	_numPorts = _config->getPorts().size();
	createSockets(_config->getPorts());
	setupPoll();
	_clients.resize(1000);
}

SocketInterface::~SocketInterface()
{
	for (std::vector<int>::iterator it = _sockets.begin(); it != _sockets.end(); ++it)
	{
		close(*it);
	}
}

void SocketInterface::createSockets(const std::vector<std::string> &ports)
{
	for (std::vector<std::string>::const_iterator it = ports.begin(); it != ports.end(); ++it)
	{
		const std::string &port = *it;
		int sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
		{
			std::cerr << "socket() failed" << std::endl;
			exit(1);
		}
		int optval = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
		{
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
	_pollFds.resize(_numPorts);
	for (int i = 0; i < _numPorts; ++i)
	{
		_pollFds[i].fd = _sockets[i];
		_pollFds[i].events = POLLIN;
	}
}

void SocketInterface::ReadRequest(int fd, RequestBuffer &client)
{
	char buf[1024];

	int ret = read(fd, buf, sizeof(buf) - 1);
	if (ret < 0)
	{
		std::cerr << "read() failed" << std::endl;
	}

	buf[ret] = '\0';
	std::string request(buf);
	if (client.request.empty() && request == "\r\n")
	{
		return ;
	}
	client.request += buf;
	
	if (client.request.find("\r\n\r\n") != std::string::npos)
	{
		client.isRequestFinished = true;
	}

}

HttpRequest SocketInterface::parseRequest(int clientFd)
{
	char buf[1024];

	read(clientFd, buf, sizeof(buf) - 1);
	buf[1024 - 1] = '\0';

	std::string request(buf);
	RequestParser parser(_config);
	HttpRequest req = parser.parse(request);
	req.fd = clientFd;
	if (req.statusCode == 400)
	{
		std::cout << "400 Bad Request" << std::endl;
		exit(1);
	}
	return parser.parse(request);
}


void SocketInterface::eventLoop()
{
	while (1)
	{
		int ret = poll(&_pollFds[0], _numPorts + _numClients, 1000);
		if (ret < 0)
		{
			std::cerr << "poll() failed" << std::endl;
			continue;
		}
		for (int i = 0; i < _numPorts + _numClients; ++i)
		{
			if (_pollFds[i].revents & POLLIN)
			{
				std::cout << "POLLIN" << std::endl;
				if (i < _numPorts) // Listening sockets
				{
					acceptConnection(_pollFds[i].fd);
				}
				else // Client sockets
				{
					ReadRequest(_pollFds[i].fd, _clients[_pollFds[i].fd]);
					if (_clients[_pollFds[i].fd].isRequestFinished)
					{
						std::cout << "Request: " << _clients[_pollFds[i].fd].request << std::endl;
						_clients[_pollFds[i].fd].isRequestFinished = false;
						_clients[_pollFds[i].fd].request = "";
						_clients[_pollFds[i].fd].state = WRITE_RESPONSE;
						_pollFds[i].events = POLLOUT;
					}
				}
			}
			else if (_pollFds[i].revents & POLLOUT)
			{
				std::cout << "POLLOUT" << std::endl;
				sendResponse(_pollFds[i].fd);
				_pollFds[i].events = POLLIN;
				_clients[_pollFds[i].fd].state = READ_REQUEST;
				_clients[_pollFds[i].fd].request = "";
			}
			else if (_pollFds[i].revents & POLLHUP)
			{
				close(_pollFds[i].fd);
				_pollFds.erase(_pollFds.begin() + i);
				--_numClients;
			}
		}
	}
}

void SocketInterface::sendResponse(int fd)
{
	write(fd, "HTTP/1.1 200 OK\r\n", 17);
	write(fd, "Content-Type: text/html\r\n", 25);
	write(fd, "Content-Length: 44\r\n", 20);
	write(fd, "\r\n", 2);
	write(fd, "<html><body><h1>hello</h1></body></html>\r\n\r\n", 44);
}

void SocketInterface::acceptConnection(int fd)
{
	// sockaddr_in clientAddr;
	// socklen_t clientAddrLen = sizeof(clientAddr);
	std::cout << "accepting new connection : " << fd << std::endl;
	int clientFd = accept(fd, NULL, NULL);
	if (clientFd < 0)
	{
		std::cerr << "accept() failed" << std::endl;
	}
	std::cout << "clientFd: " << clientFd << std::endl;
	fcntl(clientFd, F_SETFL, O_NONBLOCK);
	pollfd new_pollFd;
	std::cout << "new client connected" << std::endl;
	new_pollFd.fd = clientFd;
	new_pollFd.events |= POLLIN;
	_pollFds.push_back(new_pollFd);
	RequestBuffer client;
	_clients.at(clientFd) = client;
	++_numClients;
}