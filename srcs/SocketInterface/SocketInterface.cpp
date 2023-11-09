#include "SocketInterface.hpp"
#include "ServerContext.hpp"
#include "Config.hpp"
#include "CoreHandler.hpp"
#include "Cgi.hpp"
#include <iostream>
#include <sstream>
// POLLRDHUPのinclude
#include <poll.h>
#include <signal.h>

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
		if (usedPorts.find(port) != usedPorts.end())
		{
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
		std::cout << "listen port : " << port << std::endl;
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
	client.response = "";
	return client;
}

std::string SocketInterface::getErrorPage(int status, const std::pair<std::string, std::string> &hostAndPort)
{
	ServerContext serverContext = _config->getServerContext(hostAndPort.second, hostAndPort.first);
	std::string errorPage = serverContext.getErrorPage(status);

	if (errorPage == "")
		return (default_error_page(status));
	else
	{
		std::ifstream file(errorPage, std::ios::binary);
		std::string page = std::string(std::istreambuf_iterator<char>(file),
									   std::istreambuf_iterator<char>());
		return error_page(status, page);
	}
}

int parseChunkedRequest(std::string body, RequestBuffer &client)
{
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
		// bodyに読み込んでいない部分を残す
		client.chunkedSize = -1;
		client.chunkedBody = "";
		return 200;
	}
	else
	{
		// chunkedSizeの分だけ読み込んでいない場合は、bodyをそのまま返す
		client.isRequestFinished = false;
		return 200;
	}
}

void SocketInterface::execReadRequest(pollfd &pollfd, RequestBuffer &client)
{
	char buf[1024];

	int ret = read(pollfd.fd, buf, sizeof(buf) - 1);
	buf[ret] = '\0';
	std::string request(buf);
	if (ret < 0)
	{
		std::cerr << "read() failed" << std::endl;
		client.httpRequest.statusCode = 500;
		client.state = WRITE_REQUEST_ERROR;
		return;
	}
	pollfd.events = POLLIN;
	execParseRequest(pollfd, client, request);
}

HttpRequest SocketInterface::parseRequest(std::string request, RequestBuffer &client)
{
	RequestParser parser(_config);
	HttpRequest req = parser.parse(request, client.isChunked, client.hostAndPort.second);
	return req;
}

void SocketInterface::setCgiBody(RequestBuffer &client, std::string &body)
{
	int fd = client.cgi.getPipeStdin()[1];

	createClient(fd, WRITE_CGI_BODY);
	_clients[fd].request = body;
	_clients[fd].isRequestFinished = true;
	_clients[fd].clientFd = client.clientFd;
}

int parseLine(RequestBuffer &client, std::string request)
{
	// requestが空の場合またはCRLFは、200を返す
	if (client.request.empty() && request == "\r\n" && client.isChunked == false)
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
			if (contentLength.empty())
			{
				return 411;
			}
			size_t len = 0;
			try
			{
				len = std::stoi(contentLength);
			}
			catch (std::exception &e)
			{
				return 400;
			}
			if (contentLength == "0")
			{
				client.isRequestFinished = true;
			}
			else if (body.size() >= len)
			{
				client.isRequestFinished = true;
			}
			else
			{
				client.isRequestFinished = false;
				return 200;
			}
		}
		client.isRequestFinished = true;
	}
	return 200;
}

void SocketInterface::execParseRequest(pollfd &pollfd, RequestBuffer &client, std::string request)
{
	client.lastAccessTime = getNowTime();
	int ret = parseLine(client, request);

	if (ret != 200)
	{
		client.httpRequest.statusCode = ret;
		client.state = WRITE_REQUEST_ERROR;
		client.response = getErrorPage(client.httpRequest.statusCode, client.hostAndPort);
		client.isRequestFinished = false;
		client.request = "";
		pollfd.events = POLLOUT;
		return;
	}
	if (client.isRequestFinished)
	{
		// Requestの解析
		client.httpRequest = parseRequest(client.request, client);
		client.hostAndPort.first = client.httpRequest.hostname;
		if (client.httpRequest.statusCode != 200)
		{
			client.state = WRITE_REQUEST_ERROR;
			client.response = getErrorPage(client.httpRequest.statusCode, client.hostAndPort);
			client.isRequestFinished = false;
			client.request = "";
			pollfd.events = POLLOUT;
			return;
		}
		if (client.httpRequest.isCgi)
		{

			ServerContext context = _config->getServerContext(client.hostAndPort.second, client.hostAndPort.first);
			client.cgi = Cgi(client.httpRequest, context);
			if (client.httpRequest.method == "POST")
			{
				setCgiBody(client, client.httpRequest.body);
				client.state = WAIT_CGI;
			}
			else
			{
				client.state = EXEC_CGI;
			}
		}
		else
		{
			client.state = EXEC_CORE_HANDLER;
		}
		client.isRequestFinished = false;
		client.request = "";
		pollfd.events = POLLOUT;
	}
	else
	{
		pollfd.events = POLLIN;
		client.state = READ_REQUEST;
	}
}

void SocketInterface::execWriteResponse(pollfd &pollFd, RequestBuffer &client)
{
	int res = sendResponse(pollFd.fd, client.response);
	client.lastAccessTime = getNowTime();
	if (res == 0)
	{
		pollFd.events = POLLIN;
		client.state = READ_REQUEST;
		client.isChunked = false;
		client.isRequestFinished = false;
		client.request = "";
		client.response = "";
		client.cgiLocalRedirectCount = 0;
		client.cgiPid = -1;
		if (client.httpRequest.headers["Connection"] == "close")
		{
			client.isClosed = true;
		}
	}
	else if (res > 0)
	{
		client.response = client.response.substr(res);
	}
	else
	{
		client.state = WRITE_REQUEST_ERROR;
		client.isRequestFinished = false;
		client.response = "";
		client.httpRequest.statusCode = 500;
		client.response = getErrorPage(500, client.hostAndPort);
		pollFd.events = POLLOUT;
	}
}

void SocketInterface::execCoreHandler(pollfd &pollFd, RequestBuffer &client)
{
	CoreHandler coreHandler(_config->getServerContext(client.hostAndPort.second, client.hostAndPort.first));
	client.lastAccessTime = getNowTime();
	client.response = coreHandler.processRequest(client.httpRequest, client.hostAndPort);
	pollFd.events = POLLOUT;
	client.state = WRITE_RESPONSE;
}

void SocketInterface::execCgi(pollfd &pollFd, RequestBuffer &client) // clientのfd
{
	// clientはユーザー側 Cgiは今後pipeのfdでeventLoopを回す
	CgiResult result;
	result.body = "";
	result.statusCode = 200;
	result.fd = -1;
	result.pid = -1;
	client.cgi.execCGI(result);
	client.lastAccessTime = getNowTime();
	if (result.statusCode != 200)
	{
		client.httpRequest.statusCode = result.statusCode;
		client.state = WRITE_REQUEST_ERROR;
		pollFd.events = POLLOUT;
		client.response = getErrorPage(client.httpRequest.statusCode, client.hostAndPort);
		return;
	}
	int fd = result.fd;
	pollfd cgi = createClient(fd, WAIT_CGI_CHILD);
	_clients[fd].clientFd = pollFd.fd; // clientのpollFdをcgiに渡す
	_clients[fd].cgiPid = result.pid;
	_clients[fd].hostAndPort = client.hostAndPort;
	client.cgiPid = result.pid;
	client.cgiFd = cgi.fd; // 削除時にpollFdを削除するために必要

	pollFd.events = POLLOUT;
	client.state = WAIT_CGI;
}

void SocketInterface::execWriteError(pollfd &pollFd, RequestBuffer &client, int index)
{
	int ret = sendResponse(pollFd.fd, client.response);
	pollFd.events = POLLOUT;
	if (ret == 0)
	{
		pollFd.events = 0;
		client.state = CLOSE_CONNECTION;
		pushDelPollFd(pollFd.fd, index);
		return;
	}
	else if (ret > 0)
	{
		client.response = client.response.substr(ret);
	}
	else if (ret < 0)
	{
		pushDelPollFd(pollFd.fd, index);
	}
}

void SocketInterface::execWriteCGIBody(pollfd &pollFd, RequestBuffer &client)
{
	std::string response = client.request;
	client.lastAccessTime = getNowTime();
	int ret = sendResponse(pollFd.fd, response);
	if (ret == 0)
	{
		pollFd.events = 0;
		_clients[client.clientFd].state = EXEC_CGI;
	}
	else if (ret > 0)
	{
		client.request = client.request.substr(ret);
	}
	else
	{
		pollFd.events = POLLOUT;
	}
}

std::string SocketInterface::parseCgiResponse(std::string response, std::string method, RequestBuffer &client) // client はユーザー側
{
	std::string header = response.substr(0, response.find("\r\n\r\n"));
	std::string body = response.substr(response.find("\r\n\r\n") + 4);
	CgiParser parser(header, body, method);
	std::string cgiResponse = parser.generateCgiResponse();
	if (parser.getCgiResponseType() == Server_Redirect)
	{
		client.cgiLocalRedirectCount++;
		if (client.cgiLocalRedirectCount > MAX_LOCAL_REDIRECT_COUNT)
		{
			client.httpRequest.statusCode = 500;
			client.response = getErrorPage(500, client.hostAndPort);
			client.state = WRITE_REQUEST_ERROR;
			return client.response;
		}
		client.isChunked = false;
		client.isRequestFinished = false;
		client.chunkedSize = 0;
		client.response = "";
		client.httpRequest.statusCode = 200;
		client.httpRequest.headers.clear();
		client.cgiFd = -1;
		client.cgiPid = -1;
		client.chunkedBody = "";
		client.request = ""; // リクエストの形式でcgiResponseを保存
		for (int i = 0; i < _numClients + _numPorts; ++i)
		{
			if (_pollFds[i].fd == client.clientFd)
			{
				_pollFds[i].events = POLLOUT;
				execParseRequest(_pollFds[i], _clients[client.clientFd], cgiResponse);
				break;
			}
		}
	}
	else if (parser.getCgiResponseType() == Client_Redirect)
	{
		client.httpRequest.statusCode = 302;
		client.response = cgiResponse;
		client.state = WRITE_CGI;
	}
	else
	{
		client.response = cgiResponse;
		client.state = WRITE_CGI;
	}

	return cgiResponse;
}

void SocketInterface::execWaitCgiChild(pollfd &pollFd, RequestBuffer &client)
{
	int status;
	int waitRes = waitpid(client.cgiPid, &status, WNOHANG);
	if (waitRes < 0)
	{
		std::cerr << "waitpid() failed" << std::endl;
		return;
	}
	else if (waitRes == 0)
	{
		return;
	}
	if (status != 0)
	{
		try
		{
			_clients.at(client.clientFd).state = WRITE_REQUEST_ERROR;
			_clients.at(client.clientFd).httpRequest.statusCode = 500;
			_clients.at(client.clientFd).response = getErrorPage(500, client.hostAndPort);
			client.state = WAIT_CGI;
		}
		catch (std::exception &e)
		{
			;
		}
		return;
	}
	client.state = READ_CGI;
	pollFd.events = POLLIN;
}

void SocketInterface::execReadCgi(pollfd &pollFd, RequestBuffer &client) // cgiのfd
{
	char buf[1024];
	int ret = read(pollFd.fd, buf, sizeof(buf) - 1);
	if (ret < 0)
	{
		std::cerr << "read() failed" << std::endl;
		return;
	}
	buf[ret] = '\0';
	std::string response(buf);
	_clients.at(client.clientFd).lastAccessTime = getNowTime();
	if (ret == 0)
	{
		client.state = WAIT_CGI;
		pollFd.events = 0;
		parseCgiResponse(client.response, _clients.at(client.clientFd).httpRequest.method, _clients.at(client.clientFd));
		return;
	}
	client.response += response;
	pollFd.events = POLLIN;
	client.state = READ_CGI;
}

void SocketInterface::execWriteCgi(pollfd &pollFd, RequestBuffer &client) // clientのfd
{
	// レスポンスを送信する
	int ret = sendResponse(pollFd.fd, client.response);
	client.lastAccessTime = getNowTime();
	if (ret == 0)
	{
		close(_clients[pollFd.fd].cgiFd);
		if (client.httpRequest.headers["Connection"] == "close")
		{
			client.isClosed = true;
		}
		_clients[pollFd.fd].response = "";
		_clients[pollFd.fd].request = "";
		_clients[pollFd.fd].isRequestFinished = false;
		_clients[pollFd.fd].cgiFd = -1;
		_clients[pollFd.fd].state = READ_REQUEST;
		pollFd.events = POLLIN;
	}
	else if (ret > 0)
	{
		client.response = client.response.substr(ret);
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
	close(fd);
	_clients.erase(fd);
}

void SocketInterface::deleteClient()
{
	for (size_t i = 0; i < _pollFds.size(); ++i)
	{
		State state = _clients[_pollFds[i].fd].state;
		if (((state != EXEC_CGI && state != READ_CGI && state != WAIT_CGI_CHILD && state != WRITE_REQUEST_ERROR) && _pollFds[i].revents & POLLHUP) ||
			_clients[_pollFds[i].fd].isClosed || isTimeout(_clients[_pollFds[i].fd].lastAccessTime, TIMEOUT))
		{
			if (_clients[_pollFds[i].fd].cgiFd > 0)
			{
				for (size_t j = i; j < _pollFds.size(); ++j)
				{
					if (_pollFds[j].fd == _clients[_pollFds[i].fd].cgiFd)
					{
						kill(_clients[_pollFds[i].fd].cgiPid, SIGKILL);
						waitpid(_clients[i].cgiPid, NULL, 0);
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
		std::cout << " connection close : " << _pollFds[index].fd << std::endl;
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
				if (state == EXEC_CORE_HANDLER)
				{
					execCoreHandler(_pollFds[i], _clients[_pollFds[i].fd]);
				}
				if (state == WRITE_RESPONSE)
				{
					execWriteResponse(_pollFds[i], _clients[_pollFds[i].fd]);
				}
				else if (state == EXEC_CGI)
				{
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
				else if (state == WRITE_CGI_BODY)
				{
					execWriteCGIBody(_pollFds[i], _clients[_pollFds[i].fd]);
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
					else if (state == WAIT_CGI_CHILD)
					{
						execWaitCgiChild(_pollFds[i], _clients[_pollFds[i].fd]);
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
	fcntl(fd, F_SETFL, O_NONBLOCK);
	pollFd.fd = fd;
	if (state == WRITE_CGI_BODY)
	{
		pollFd.events = POLLOUT;
	}
	else
	{
		pollFd.events = POLLIN;
	}
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
	client.isClosed = false;
	client.lastAccessTime = getNowTime();
	client.cgiPid = -1;
	client.cgiLocalRedirectCount = 0;
	// pollFdのfdをキーにしてクライアントを管理する
	_clients.insert(std::make_pair(fd, client));
	return pollFd;
}

std::string itostr(int num)
{
	std::stringstream ss;
	ss << num;
	return ss.str();
}

void SocketInterface::acceptConnection(int fd)
{
	sockaddr_in clientAddr;
	socklen_t clientAddrSize = sizeof(clientAddr);
	int clientFd = accept(fd, (sockaddr *)&clientAddr, &clientAddrSize);

	if (clientFd < 0)
	{
		std::cerr << "accept() failed" << std::endl;
		return;
	}
	createClient(clientFd, READ_REQUEST);
	std::cout << "accept connection : " << clientFd << std::endl;
	// アクセスされたサーバーのhost名を取得する
	getsockname(clientFd, (sockaddr *)&clientAddr, &clientAddrSize);
	// ポートを取得
	_clients[clientFd].hostAndPort.second = itostr(ntohs(clientAddr.sin_port));
}
