#include "RequestParser.hpp"
#include <sstream>
#include <iostream>

#include <string.h> // TODO: remove this

RequestParser::RequestParser(Config* config) : _config(config) {
};

RequestParser::~RequestParser() {
};

std::vector<std::string> RequestParser::split(const std::string &s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);

	if (s.empty())
		return tokens;
	if (s == "/")
	{
		tokens.push_back("/");
		return tokens;
	}
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

bool RequestParser::isCgiDir(std::vector<std::string> tokens)
{
    return tokens.size() >= 1 && tokens[1] == "cgi-bin";
}

bool RequestParser::isCgiBlockPath(const ServerContext& server_context, std::vector<std::string> tokens)
{
	if (!server_context.getIsCgi())
		return false;
	CGIContext cgi_context = server_context.getCGIContext();
	std::string exe = cgi_context.getDirective("extension");
	size_t i = 0;
	while (i < tokens.size())
	{
		// 拡張子を取得
		std::string ext = tokens[i].substr(tokens[i].find_last_of(".") + 1);
		if (ext == exe)
			return true;
		i++;
	}
	return false;
}

HttpRequest RequestParser::parse(const std::string& request, bool isChunked, const std::string& port) {
    HttpRequest httpRequest;
    httpRequest.isCgi = false;
    httpRequest.statusCode = 200;

    std::string header = request.substr(0, request.find("\r\n\r\n"));
    std::string body = request.substr(request.find("\r\n\r\n") + 4);
    std::istringstream headerStream(header);
    std::istringstream bodyStream(body);

    // メソッドとURLを解析
    headerStream >> httpRequest.method >> httpRequest.url >> httpRequest.protocol;

    // 正しくない形式の場合は400を返す
    if (httpRequest.protocol.empty() || httpRequest.protocol.find("HTTP/1.1") == std::string::npos || httpRequest.url.empty() || httpRequest.method.empty()) {
        httpRequest.statusCode = 400;
        return httpRequest;
    }
    // ヘッダーを解析
    std::string headerLine;
    int contentLength = -1; // Content-Lengthを保存する変数
    while (std::getline(headerStream, headerLine)) {
        std::istringstream headerStream(headerLine);
        std::string key;
        std::string value;
        std::getline(headerStream, key, ':');
        std::getline(headerStream, value);
        if (!value.empty() && value[0] == ' ') {
            value = value.substr(1); // 先頭のスペースを削除
        }
        httpRequest.headers[key] = value;
        if (key == "Content-Length") {
            try {
                contentLength = std::stoi(value);
            } catch(std::exception& e) {
                contentLength = 0;
                httpRequest.statusCode = 400;
                return httpRequest;
            }
        } 
    }
    if (header.find("Host:") != std::string::npos) {
            std::string value = httpRequest.headers["Host"];
            std::cerr << "thisisvalue: " << value << "|" << std::endl;
            //　文字列valueに：がある場合は、その前までをホスト名とする
            if (value.find(":") != std::string::npos) {
                httpRequest.hostname = value.substr(0, value.find(":"));
            }
            else if (value.find("\01") != std::string::npos)
            {
                std::cout << "thisisvalueeeeeeeee: " << value << "|" << "\n";
                httpRequest.hostname = value.substr(0, value.find("\01"));
            } else {
                std::cout << "thisisvaluzzzzzzzzz: " << value << "|" << "\n";
                httpRequest.hostname = value;
            }
            std::cout << httpRequest.hostname.compare("") << "\n";
    }
    std::cout << "portooooooooooo: " << port <<  "|" << std::endl;
    std::cout << "hostnameeeeeeee: " << httpRequest.hostname << "|" << std::endl;
    ServerContext serverContext = _config->getServerContext(port, httpRequest.hostname);
    std::cout << "server_name ha!: " << serverContext.getServerName() << std::endl;
    if (httpRequest.method == "POST" && contentLength == -1 && !isChunked) {
        httpRequest.statusCode = 411;
        return httpRequest;
    }
    // ボディを解析 (Content-Lengthが指定されていれば)
    size_t maxBodySize = 0;
    try {
        maxBodySize = std::stoi(serverContext.getMaxBodySize()); 
    } catch(std::exception& e) {
        maxBodySize = 8192;
    }
    if (contentLength > 0) {
        if (contentLength > (int)maxBodySize) {
            httpRequest.statusCode = 413;
            return httpRequest;
        }
        char* buffer = new char[contentLength];
        bodyStream.read(buffer, contentLength);
        httpRequest.body = std::string(buffer, contentLength);
        delete[] buffer;
    }
    // ボディを解析 (Transfer-Encoding: chunkedが指定されていれば)
    if (isChunked)
    {
        if (body.size() > maxBodySize) {

            httpRequest.statusCode = 413;
            return httpRequest;
        }
    }
    std::vector<std::string> tokens = split(httpRequest.url, '?');
	if (tokens.size() == 0)
        return httpRequest;
    std::vector<std::string> path_tokens = split(tokens[0], '/');
	// home directory
	if (tokens[0] == "/") {
		return httpRequest;
	}
    if (isCgiDir(path_tokens) || isCgiBlockPath(serverContext, path_tokens))
    {
        httpRequest.isCgi = true;
    }
    return httpRequest;
}
