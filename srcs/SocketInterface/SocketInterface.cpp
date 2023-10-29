#include "SocketInterface.hpp"
#include "ServerContext.hpp"
#include "Config.hpp"
#include "CoreHandler.hpp"
#include "Cgi.hpp"
#include <iostream>
#include <sstream>
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
	std::set<std::string> usedPorts;

	for (std::vector<std::string>::const_iterator it = ports.begin(); it != ports.end(); ++it)
	{
		const std::string &port = *it;
		// もしポート番号が重複していたら、continueする
		if (usedPorts.find(port) != usedPorts.end()) {
			std::cout << "Duplicate port " << port << " ignored." << std::endl;
			_numPorts--;
			continue;
		}
		usedPorts.insert(port);
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

RequestBuffer initRequestBuffer(int fd)
{
	RequestBuffer client;
	client.clientFd = fd;
	client.cgiFd = -1;
	client.cgiPid = -1;
	client.state = READ_REQUEST;
	client.isRequestFinished = false;
	client.request = "";
	client.cgiPid = -1;
	client.response = "";
	return client;
}

int parseChunkedRequest(std::string body, RequestBuffer &client)
{
	std::cout << "body = " << body << std::endl;
	if (body.empty())
	{
		client.isRequestFinished = false;
		client.chunkedSize = -1;
		return 200;
	}
	if (client.chunkedSize < 0)
	{
		std::string chunkedSizeStr = body.substr(0, body.find("\r\n"));
		try
		{
			client.chunkedSize = std::stoi(chunkedSizeStr, 0, 16);
		}
		catch (std::exception &e)
		{
			client.isRequestFinished = false;
			client.chunkedSize = -1;
			return 400;
		}
		client.chunkedBody = body.substr(body.find("\r\n") + 2);
	}
	else
	{
		client.chunkedBody += body;
	}
	// chunkedSizeが0かつ、bodyに\r\n\r\nが含まれている場合は、chunkedSizeを-1に戻して、chunkedBodyを空にする
	if (client.chunkedSize == 0 && body == "\r\n")
	{
		client.chunkedSize = -1;
		client.chunkedBody = "";
		client.isRequestFinished = true;
		return 200;
	}
	if (client.chunkedBody.size() > (size_t)client.chunkedSize + 2)
	{
		client.isRequestFinished = false;
		return 400;
	}
	else if (client.chunkedBody.size() == (size_t)client.chunkedSize + 2)
	{
		// chunkedSizeの分だけ読み込んだら、chunkedSizeを-1に戻して、chunkedSizeの分だけ読み込んだbodyをresponseに追加する
		client.request += client.chunkedBody.substr(0, client.chunkedSize);
		std::cout << "request: " << client.request << std::endl;
		// bodyに読み込んでいない部分を残す
		body = client.chunkedBody.substr(client.chunkedSize + 2);
		client.chunkedSize = -1;
		client.chunkedBody = "";
		return parseChunkedRequest(body, client);
	}
	else
	{
		// chunkedSizeの分だけ読み込んでいない場合は、bodyをそのまま返す
		client.isRequestFinished = false;
		return 200;
	}
}

int SocketInterface::ReadRequest(int fd, RequestBuffer &client)
{
	char buf[1024];

	int ret = read(fd, buf, sizeof(buf) - 1);
	if (ret < 0)
	{
		std::cerr << "read() failed" << std::endl;
		return 200;
	}
	buf[ret] = '\0';
	std::string request(buf);
	std::cout << "request: " << request << std::endl;
	if (client.request.empty() && request == "\r\n")
	{
		return 200;
	}
	if (client.isChunked)
	{
		return parseChunkedRequest(request, client);
	}
	client.request += request;
	if (client.request.find("\r\n\r\n") != std::string::npos)
	{
		std::cout << "request: " << request << std::endl;
		std::string header = client.request.substr(0, client.request.find("\r\n\r\n"));
		std::string method = header.substr(0, header.find(" "));
		std::string body = client.request.substr(client.request.find("\r\n\r\n") + 4);
		if (header.find("Transfer-Encoding: chunked") != std::string::npos)
		{
			client.isChunked = true;
			client.request = header + "\r\n\r\n";
			return parseChunkedRequest(body, client);
		}
		if (method == "POST" && !client.isChunked)
		{
			std::string contentLength = header.substr(header.find("Content-Length: ") + 16);
			std::cout << "contentLength: " << contentLength << std::endl;
			if (contentLength.empty())
			{
				return 411;
			}
			size_t len = 0;
			try {
				len = std::stoi(contentLength);
			} catch (std::exception &e) {
				return 400;
			}
			if (contentLength == "0")
			{
				client.isRequestFinished = true;
			}
			else if (body.size() >= len)
			{
				std::cout << "body.size(): " << body.size() << std::endl;
				client.isRequestFinished = true;
			} else {
				client.isRequestFinished = false;
				return 200;
			}
		}
		client.isRequestFinished = true;
	}
	return 200;
}

HttpRequest SocketInterface::parseRequest(std::string request, RequestBuffer &client)
{
	RequestParser parser(_config);
	HttpRequest req = parser.parse(request, client.isChunked, client.hostAndPort.second);
	return req;
}

void SocketInterface::execReadRequest(pollfd &pollfd, RequestBuffer &client)
{
	int ret = ReadRequest(pollfd.fd, client);
	if (ret != 200)
	{
		client.httpRequest.statusCode = ret;
		client.state = WRITE_REQUEST_ERROR;
		client.isRequestFinished = false;
		client.request = "";
		pollfd.events = POLLOUT;
		return;
	}
	if (client.isRequestFinished)
	{
		// Requestの解析
		RequestParser parser(_config);
		client.httpRequest = parseRequest(client.request, client);
		client.hostAndPort.first = client.httpRequest.hostname;

		if (client.httpRequest.statusCode != 200)
		{
			client.state = WRITE_REQUEST_ERROR;
			client.isRequestFinished = false;
			client.request = "";
			pollfd.events = POLLOUT;
			return;
		}
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
	CoreHandler coreHandler(_config->getServerContext
							(client.hostAndPort.second, client.hostAndPort.first));
	std::string response = coreHandler.processRequest(client.httpRequest);
	if (sendResponse(pollFd.fd, response) >= 0)
	{
		pollFd.events = POLLIN;
		client.state = READ_REQUEST;
		client.isChunked = false;
		client.isRequestFinished = false;
	}
	else
	{
		pollFd.events = POLLOUT;
		client.state = WRITE_RESPONSE;
	}
}

void SocketInterface::execCgi(pollfd &pollFd, RequestBuffer &client) // clientのfd
{
	// clientはユーザー側 Cgiは今後pipeのfdでeventLoopを回す
	Cgi Cgi(client.httpRequest);
	CgiResult result = Cgi.execCGI(); // cgiのpipeのfd
	int fd = result.fd;
	pollfd cgi = createClient(fd, READ_CGI);
	_clients[fd].clientFd = pollFd.fd; // clientのpollFdをcgiに渡す
	_clients[fd].cgiPid = result.pid;
	client.cgiFd = cgi.fd; // 削除時にpollFdを削除するために必要
	// cgiが終わったらresponseに書き込まれるので、終わり次第レスポンスとして送信する
	pollFd.events = POLLOUT;
	client.state = WAIT_CGI;
}

void SocketInterface::execWriteError(pollfd &pollFd, RequestBuffer &client, int index)
{
	std::string statusCode = std::to_string(client.httpRequest.statusCode);
	std::string response = "HTTP/1.1 " + statusCode + " ";
	if (client.httpRequest.statusCode == 411)
	{
		response += "Length Required";
	}
	else
	{
		response += "Bad Request";
	}
	response += "\r\n";
	response += "Content-Length: 0";
	response += "\r\n";
	response += "Connection: close";
	response += "\r\n";
	response += "content-type: text/html";
	response += "\r\n\r\n";

	if (sendResponse(pollFd.fd, response) >= 0)
	{
		pollFd.events = POLLIN;
		pushDelPollFd(pollFd.fd, index);
	}
	else
	{
		pollFd.events = POLLOUT;
	}
}

std::string parseCgiResponse(std::string response)
{
	// cgiのresponseにはContent-Lengthがない場合がある
	// その場合はContent-Lengthを追加する
	std::string header = response.substr(0, response.find("\r\n\r\n"));
	std::string body = response.substr(response.find("\r\n\r\n") + 4);
	if (header.find("Content-Length") == std::string::npos)
	{
		std::string contentLength = "Content-Length: " + std::to_string(body.size() - 1) + "\r\n";
		header += "\r\n" + contentLength;
	}
	return header + "\r\n" + body;
}

void SocketInterface::execReadCgi(pollfd &pollFd, RequestBuffer &client) // cgiのfd
{
	char buf[1024];
	if (waitpid(client.cgiPid, NULL, WNOHANG) <= 0)
	{
		return;
	}
	int ret = read(pollFd.fd, buf, sizeof(buf) - 1);
	if (ret < 0)
	{
		std::cerr << "read() failed" << std::endl;
		return;
	}
	else if (ret == 0)
	{
		client.state = WAIT_CGI;
		return;
	}
	buf[ret] = '\0';
	std::string response(buf);
	// 接続されているクライアントにレスポンスを送信する
	if (client.clientFd)
	{
		if (response.find("\r\n\r\n") != std::string::npos) // レスポンスの終わり
		{
			// レスポンスを送信したら、クライアントの方を読み取れるようにステータスを変更する
			_clients.at(client.clientFd).state = WRITE_CGI;
		}
		_clients.at(client.clientFd).response += parseCgiResponse(response);
	}
}

void SocketInterface::execWriteCgi(pollfd &pollFd, RequestBuffer &client) // clientのfd
{
	// レスポンスを送信する
	if (sendResponse(pollFd.fd, client.response) >= 0)
	{
		if (close(_clients[pollFd.fd].cgiFd) < 0)
		{
			std::cerr << "close() failed" << std::endl;
		}
		_clients[pollFd.fd].response = "";
		_clients[pollFd.fd].request = "";
		_clients[pollFd.fd].isRequestFinished = false;
		_clients[pollFd.fd].cgiFd = -1;
		_clients[pollFd.fd].state = READ_REQUEST;
		pollFd.events = POLLIN;
	}
	else
	{
		_clients[pollFd.fd].state = WRITE_CGI;
		pollFd.events = POLLOUT;
	}
}

void SocketInterface::pushDelPollFd(int fd, int index)
{
	_delIndex.push_back(index);
	_clients.erase(fd);
}

void SocketInterface::deleteClient()
{
	for (size_t i = 0; i < _pollFds.size(); ++i)
	{
		State state = _clients[_pollFds[i].fd].state;
		if ((state != EXEC_CGI && state != READ_CGI) && _pollFds[i].revents & POLLHUP)
		{
			if (_clients[_pollFds[i].fd].cgiFd > 0)
			{
				for (size_t j = i; j < _pollFds.size(); ++j)
				{
					std::cout << _pollFds[j].fd << std::endl;
					if (_pollFds[j].fd == _clients[_pollFds[i].fd].cgiFd)
					{
						pushDelPollFd(_pollFds[j].fd, j);
						break;
					}
				}
			}
			pushDelPollFd(_pollFds[i].fd, i);
		}
	}
	std::sort(_delIndex.begin(), _delIndex.end(), std::greater<int>());
	// 重複しているインデックスを削除する
	_delIndex.erase(std::unique(_delIndex.begin(), _delIndex.end()), _delIndex.end());
	for (size_t i = 0; i < _delIndex.size(); ++i)
	{
		int index = _delIndex[i];
		close(_pollFds[index].fd);
		_pollFds.erase(_pollFds.begin() + index);
		_numClients--;
	}
	if (_delIndex.size() > 0)
		_delIndex.clear();
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
		for (int i = 0; i < _numPorts + _numClients; ++i)
		{

			if (_pollFds[i].revents & POLLOUT)
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
					execWriteCgi(_pollFds[i], _clients[_pollFds[i].fd]);
				}
				else if (state == WRITE_REQUEST_ERROR)
				{
					execWriteError(_pollFds[i], _clients[_pollFds[i].fd], i);
				}
			}
			else if (_pollFds[i].revents & POLLIN)
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
					else if (state == READ_CGI) // CGIの結果を受け取る
					{
						execReadCgi(_pollFds[i], _clients[_pollFds[i].fd]);
					}
				}
			}
		}
		deleteClient();
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
	std::cout << "sendResponse" << std::endl
			  << response << std::endl;
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
	pollFd.events = POLLIN;
	_addPollFds.push_back(pollFd);
	RequestBuffer client;
	client.state = state;
	client.response = "";
	client.request = "";
	client.isChunked = false;
	client.chunkedSize = -1;
	client.chunkedBody = "";
	client.isRequestFinished = false;
	client.cgiFd = -1;
	client.clientFd = fd;
	// pollFdのfdをキーにしてクライアントを管理する
	_clients.insert(std::make_pair(fd, client));
	return pollFd;
}

std::string itostr(int num) {
    std::stringstream ss;
    ss << num;
    return ss.str();
}

void SocketInterface::acceptConnection(int fd)
{
	sockaddr_in clientAddr;
	socklen_t clientAddrSize = sizeof(clientAddr);
	int clientFd = accept(fd, (sockaddr *)&clientAddr, &clientAddrSize);
	// int clientFd = accept(fd, NULL, NULL);
	std::cout << "acceptConnection " << clientFd << std::endl;
	if (clientFd < 0)
	{
		std::cerr << "accept() failed" << std::endl;
		perror("accept");
		return;
	}
	fcntl(clientFd, F_SETFL, O_NONBLOCK);
	createClient(clientFd, READ_REQUEST);

	// アクセスされたサーバーのhost名を取得する
	getsockname(clientFd, (sockaddr *)&clientAddr, &clientAddrSize);
	// ポートを取得
	_clients[clientFd].hostAndPort.second = itostr(ntohs(clientAddr.sin_port));
	// int serverHost = ntohl(clientAddr.sin_addr.s_addr);

	// unsigned char bytes[4];
	// bytes[0] = serverHost & 0xFF;
	// bytes[1] = (serverHost >> 8) & 0xFF;
	// bytes[2] = (serverHost >> 16) & 0xFF;
	// bytes[3] = (serverHost >> 24) & 0xFF;

	// // 文字列に変換
	// std::stringstream ss;
	// ss << static_cast<int>(bytes[3]) << '.'
	// << static_cast<int>(bytes[2]) << '.'
	// << static_cast<int>(bytes[1]) << '.'
	// << static_cast<int>(bytes[0]);
	// std::string serverHostStr = ss.str();

	// std::cout << "serverHost: " << serverHostStr << std::endl;
	std::cout << "serverPort: " << _clients[clientFd].hostAndPort.second << std::endl;
}
