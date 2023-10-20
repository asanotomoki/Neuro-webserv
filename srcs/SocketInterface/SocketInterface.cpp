#include "SocketInterface.hpp"
#include "ServerContext.hpp"
#include "Config.hpp"
#include "RequestParser.hpp"
#include "CoreHandler.hpp"
#include "Cgi.hpp"
#include <iostream>
// POLLRDHUPのinclude
#include <poll.h>

// std::map<int, RequestBuffer> _clients;
// _client -> clientFdをキーにして、RequestBufferを格納する

SocketInterface::SocketInterface(Config *config)
	: _config(config), _numClients(0)
{
	_numPorts = _config->getPorts().size();
	createSockets(_config->getPorts());
	setupPoll();
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
			perror("socket");
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
		return;
	}
	else if (ret == 0)
	{

		client.state = READ_REQUEST;
		return;
	}

	buf[ret] = '\0';
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
		if (client.httpRequest.isCgi)
		{
			client.state = EXEC_CGI;
		}
		else
		{
			client.state = WRITE_RESPONSE;
		}
		client.isRequestFinished = false;
		client.request = "";
		pollfd.events = POLLOUT;
	}
}

void SocketInterface::execCoreHandler(pollfd &pollFd, RequestBuffer &client)
{
	CoreHandler coreHandler(_config->getServerContext("2000", "localhost"));
	std::string response = coreHandler.processRequest(client.httpRequest);
	sendResponse(pollFd.fd, response);
	pollFd.events = POLLIN;
	client.state = READ_REQUEST;
}

void SocketInterface::execCgi(pollfd &pollFd, RequestBuffer &client) // clientのfd
{
	// clientはユーザー側 Cgiは今後pipeのfdでeventLoopを回す
	Cgi Cgi(client.httpRequest);
	int fd = Cgi.execCGI(); // cgiのpipeのfd

	std::cout << "cgi output fd: " << fd << std::endl;
	createClient(fd, READ_CGI);
	_clients[fd].clientPollFd = &pollFd;
	// cgiが終わったらresponseに書き込まれるので、終わり次第レスポンスとして送信する
	pollFd.events = POLLOUT;
	client.state = WAIT_CGI;
}

void SocketInterface::execReadCgi(pollfd &pollFd, RequestBuffer &client) // cgiのfd
{

	char buf[1024];
	int ret = read(pollFd.fd, buf, sizeof(buf) - 1);
	if (ret < 0)
	{
		std::cerr << "read() failed" << std::endl;
	}
	else if (ret == 0)
	{
		client.state = WAIT_CGI;
		return;
	}
	buf[ret] = '\0';
	std::string response(buf);
	// 接続されているクライアントにレスポンスを送信する
	_clients.at(client.clientPollFd->fd).response += response;
	// 接続の方を読み取れるようにステータスを変更する
	_clients.at(client.clientPollFd->fd).state = WRITE_CGI;
}

void SocketInterface::eventLoop()
{

	while (1)
	{
		int ret = poll(&_pollFds[0], _numPorts + _numClients, -1);
		if (ret < 0)
		{
			std::cerr << "poll() failed" << std::endl;
			continue;
		}
		else if (ret == 0)
		{
			std::cout << "poll() timeout" << std::endl;
			continue;
		}
		usleep(10000);
		for (int i = 0; i < _numPorts + _numClients; ++i)
		{
			if (_pollFds[i].revents & POLLIN)
			{
				if (i < _numPorts)
				{
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
						std::cout << "READ_CGI" << std::endl;
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
					std::cout << "EXEC_CGI" << std::endl;
					execCgi(_pollFds[i], _clients[_pollFds[i].fd]);
				}
				else if (state == WRITE_CGI)
				{
					std::cout << "WRITE_CGI" << std::endl;
					if (sendResponse(_pollFds[i].fd, _clients[_pollFds[i].fd].response) > 0)
					{
						_clients[_pollFds[i].fd].response = "";
						_clients[_pollFds[i].fd].state = READ_REQUEST;
						_pollFds[i].events = POLLIN;
					}
					else
					{
						_clients[_pollFds[i].fd].state = WRITE_CGI;
						_pollFds[i].events = POLLOUT;
					}
				}
			}
		}
		for (size_t i = 0; i < _pollFds.size(); ++i)
		{
			State state = _clients[_pollFds[i].fd].state;
			if ((state != EXEC_CGI && state != READ_CGI) && _pollFds[i].revents & POLLHUP)
			{
				std::cout << "POLLHUP " << _pollFds[i].fd  << std::endl;
				std::cout << "state: " << state << std::endl;
				// 削除すべき要素のインデックスを記録
				_clients[_pollFds[i].fd].state = READ_REQUEST;
				_clients[_pollFds[i].fd].response = "";
				_clients[_pollFds[i].fd].request = "";
				_clients.erase(_pollFds[i].fd);
				_delIndex.push_back(i);
				_pollFds[i].events = 0;
			}
		}
		for (int i = static_cast<int>(_delIndex.size()) - 1; i >= 0; --i)
		{
			int index = _delIndex[i];
			close(_pollFds[index].fd);
			_pollFds.erase(_pollFds.begin() + index);
			_numClients--;
		}
		if (_delIndex.size() > 0)
			_delIndex.clear();
		for (size_t i = 0; i < _addPollFds.size(); ++i)
		{
			_pollFds.push_back(_addPollFds[i]);
			_numClients++;
		}
		if (_addPollFds.size() > 0)
			_addPollFds.clear();
	}
}

int SocketInterface::sendResponse(int fd, std::string response)
{
	int ret = write(fd, response.c_str(), response.size());
	if (ret < 0)
	{
		std::cerr << "write() failed" << std::endl;
	}
	return ret;
}

pollfd SocketInterface::createClient(int fd, State state)
{
	pollfd pollFd;
	pollFd.fd = fd;
	pollFd.events |= POLLIN;
	_addPollFds.push_back(pollFd);
	RequestBuffer client;
	client.state = state;
	client.response = "";
	client.request = "";
	client.isRequestFinished = false;
	// pollFdのfdをキーにしてクライアントを管理する
	_clients.insert(std::make_pair(fd, client));
	return pollFd;
}

void SocketInterface::acceptConnection(int fd)
{
	// sockaddr_in clientAddr;
	// socklen_t clientAddrSize = sizeof(clientAddr);
	// int clientFd = accept(fd, (sockaddr *)&clientAddr, &clientAddrSize);
	int clientFd = accept(fd, NULL, NULL);
	if (clientFd < 0)
	{
		std::cerr << "accept() failed" << std::endl;
		perror("accept");
		return;
	}
	std::cout << "clientFd: " << clientFd << std::endl;
	fcntl(clientFd, F_SETFL, O_NONBLOCK);
	createClient(clientFd, READ_REQUEST);
	// アクセスされたサーバーのhost名を取得する
	// getsockname(clientFd, (sockaddr *)&clientAddr, &clientAddrSize);
	// int serverPort = ntohs(clientAddr.sin_port);
	// char serverHost[1024];
	// gethostname(serverHost, 1024);

	// std::cout << "serverHost: " << serverHost << std::endl;
	// std::cout << "serverPort: " << serverPort << std::endl;
}