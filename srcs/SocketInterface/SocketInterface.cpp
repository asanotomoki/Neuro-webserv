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
	else if (ret == 0)
	{
		return;
	}
	buf[ret - 1] = '\0';
	std::string request(buf);
	if (client.request.empty() && request == "\r\n")
	{
		return;
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

void SocketInterface::execReadRequest(pollfd &pollfd, RequestBuffer &client)
{
	ReadRequest(pollfd.fd, client);
	if (client.isRequestFinished)
	{
		// Requestの解析
		RequestParser parser(_config);
		client.httpRequest = parser.parse(client.request);
		std::cout << client.httpRequest.protocol << " : " << client.httpRequest.statusCode << std::endl;
		if (client.httpRequest.isCgi)
		{
			client.state = EXEC_CGI;
		}
		else
		{
			std::cout << "execCoreHandler" << std::endl;
			client.state = WRITE_RESPONSE;
		}
		client.isRequestFinished = false;
		client.request = "";
		pollfd.events = POLLOUT;
	}
}

void SocketInterface::execCoreHandler(pollfd &pollFd, RequestBuffer &client)
{
	std::cout << "POLLOUT" << std::endl;
	CoreHandler coreHandler(_config->getServerContext("2000", "localhost"));
	std::string response = coreHandler.processRequest(client.httpRequest);
	std::cout << response << std::endl;
	sendResponse(pollFd.fd, response);
	pollFd.events = POLLIN;
	client.state = READ_REQUEST;
}

void SocketInterface::execCgi(pollfd &pollFd, RequestBuffer &client) // clientのfd
{
	// clientはユーザー側 Cgiは今後pipeのfdでeventLoopを回す
	std::cout << "POLLOUT CGI" << std::endl;
	Cgi Cgi(client.httpRequest);
	int fd = Cgi.execCGI(); // cgiのpipeのfd
	pollfd cgiPollFd = createPollFd(fd);
	_pollFds.push_back(cgiPollFd);
	_numClients++;
	RequestBuffer cgiClient;
	cgiClient.state = READ_CGI;
	cgiClient.clientPollFd = &pollFd;
	_clients.at(fd) = cgiClient;
	// cgiが終わったらresponseに書き込まれるので、終わり次第レスポンスとして送信する
	pollFd.events = POLLOUT;
	client.state = WAIT_CGI;
}

void SocketInterface::execReadCgi(pollfd &pollFd, RequestBuffer &client) // cgiのfd
{
	std::cout << "READ_CGI" << std::endl;
	char buf[1024];

	int ret = read(pollFd.fd, buf, sizeof(buf) - 1);
	if (ret < 0)
	{
		std::cerr << "read() failed" << std::endl;
	}
	else if (ret == 0)
	{
		// cgiが終了したら
		// cgiのfdをcloseする
		// eventLoopからpollFdを削除する
		//close(pollFd.fd); // Close the pipe
		pollFd.events = 0;
		client.state = WRITE_RESPONSE;

		// Remove from _pollFds
		//for (size_t i = 0; i < _pollFds.size(); ++i)
		//{
		//	if (_pollFds[i].fd == pollFd.fd)
		//	{
		//		_pollFds.erase(_pollFds.begin() + i);
		//		break;
		//	}
		//}
		//for (size_t i = 0; i < _clients.size(); ++i)
		//{
		//	if (_clients[i].fd == client.fd)
		//	{
		//		_clients.erase(_clients.begin() + i);
		//		break;
		//	}
		//}
		//_numClients--;
		return;
	}
	buf[ret] = '\0';
	std::string response(buf);
	// 接続されているクライアントにレスポンスを送信する
	_clients.at(client.clientPollFd->fd).response += response;
	// 接続の方を読み取れるようにステータスを変更する
	_clients.at(client.clientPollFd->fd).state = WRITE_CGI;
	std::cout << "response : " << response << std::endl;
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
				if (i < _numPorts)
				{ // Listening sockets
					acceptConnection(_pollFds[i].fd);
				}
				else
				{
					State state = _clients[_pollFds[i].fd].state;

					if (state == READ_REQUEST) // Client sockets
					{
						execReadRequest(_pollFds[i], _clients[_pollFds[i].fd]);
					}
					else if (state == READ_CGI)
					{
						// CGIの結果を受け取る
						execReadCgi(_pollFds[i], _clients[_pollFds[i].fd]);
					}
				}
			}
			else if (_pollFds[i].revents & POLLOUT)
			{
				State state = _clients[_pollFds[i].fd].state;
				if (state == WRITE_RESPONSE)
				{
					execCoreHandler(_pollFds[i], _clients[_pollFds[i].fd]);
				}
				else if (state == EXEC_CGI)
				{
					execCgi(_pollFds[i], _clients[_pollFds[i].fd]);
				}
				else if (state == WRITE_CGI)
				{
					// CGIの結果をクライアントに送信する
					std::cout << "WRITE_CGI" << std::endl;
					std::cout << _clients[_pollFds[i].fd].response << std::endl;
					std::cout << _clients[_pollFds[i].fd].fd << std::endl;
					sendResponse(_pollFds[i].fd, _clients[_pollFds[i].fd].response);
					_clients[_pollFds[i].fd].state = WAIT_CGI;
					_clients[_pollFds[i].fd].response = "";
					_pollFds[i].events = POLLIN;
				}
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

pollfd SocketInterface::createPollFd(int fd)
{
	pollfd pollFd;
	pollFd.fd = fd;
	pollFd.events |= POLLIN;
	return pollFd;
}

RequestBuffer SocketInterface::createRequestBuffer()
{
	RequestBuffer requestBuffer;
	requestBuffer.state = READ_REQUEST;
	requestBuffer.isRequestFinished = false;
	requestBuffer.request = "";
	return requestBuffer;
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