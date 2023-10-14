#include "SocketInterface.hpp"
#include "ServerContext.hpp"
#include "Config.hpp"
#include "RequestParser.hpp"
#include "CoreHandler.hpp"
#include "Cgi.hpp"

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

HttpRequest SocketInterface::parseRequest(std::string request)
{
	RequestParser parser(_config);
	HttpRequest req = parser.parse(request);
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
				//std::cout << "POLLIN" << std::endl;
				if (i < _numPorts) // Listening sockets
				{
					acceptConnection(_pollFds[i].fd);
				}
				else // Client sockets
				{
					//std::cout << "Client socket" << std::endl;
					ReadRequest(_pollFds[i].fd, _clients[_pollFds[i].fd]);
					if (_clients[_pollFds[i].fd].isRequestFinished)
					{
						// Requestの解析
						RequestParser parser(_config);
						_clients[_pollFds[i].fd].httpRequest = parser.parse(_clients[_pollFds[i].fd].request);
						std::cout << _clients[_pollFds[i].fd].httpRequest.protocol << " : " << _clients[_pollFds[i].fd].httpRequest.statusCode << std::endl;
						if (_clients[_pollFds[i].fd].httpRequest.isCgi)
						{
							std::cout << "set cgi" << std::endl;
							_clients[_pollFds[i].fd].state = WITE_TO_CGI;
						} else {
							_clients[_pollFds[i].fd].state = WRITE_RESPONSE;
						}
						_clients[_pollFds[i].fd].isRequestFinished = false;
						_clients[_pollFds[i].fd].request = "";
						_pollFds[i].events = POLLOUT;
					}
				}
			}
			else if (_pollFds[i].revents & POLLOUT && _clients[_pollFds[i].fd].state == WRITE_RESPONSE)
			{
				std::cout << "POLLOUT" << std::endl;
				CoreHandler coreHandler(_config->getServerContext("2000", "localhost"));
				std::string response = coreHandler.processRequest(_clients[_pollFds[i].fd].httpRequest);
				std::cout << response << std::endl;
				sendResponse(_pollFds[i].fd, response);
				_pollFds[i].events = POLLIN;
				_clients[_pollFds[i].fd].state = READ_REQUEST;
				_clients[_pollFds[i].fd].isRequestFinished = false;
				_clients[_pollFds[i].fd].request = "";
			}

			else if (_pollFds[i].revents & POLLOUT &&  _clients[_pollFds[i].fd].state == WITE_TO_CGI)//CGIの実行
			{
				// CGIは実行と結果の受け取りは別で行う
				std::cout << "POLLOUT CGI" << std::endl;
				Cgi Cgi(_clients[_pollFds[i].fd].httpRequest);
				CgiResponse response = Cgi.CgiHandler();
				_pollFds[i].events = POLLIN;
				_clients[_pollFds[i].fd].state = READ_CGI;
				_clients[_pollFds[i].fd].isRequestFinished = false;
				_clients[_pollFds[i].fd].request = "";
			}
			else if (_pollFds[i].revents & POLLHUP)
			{
				std::cout << "connect closed" << std::endl;
				close(_pollFds[i].fd);
				_pollFds.erase(_pollFds.begin() + i);
				_clients.erase(_clients.begin() + i - _numPorts);
				--_numClients;
			}
		}
	}
}

void SocketInterface::sendResponse(int fd, std::string response)
{
	int ret = write(fd, response.c_str(), response.size());
	if (ret < 0)
	{
		std::cerr << "write() failed" << std::endl;
	}
}

void SocketInterface::acceptConnection(int fd)
{

	
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
	client.state = READ_REQUEST;
	_clients.at(clientFd) = client;
	++_numClients;
}