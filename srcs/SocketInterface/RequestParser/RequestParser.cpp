#include "RequestParser.hpp"
#include <sstream>
#include <iostream>

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
    return tokens.size() >= 2 && tokens[0] == "cgi-bin";
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



std::pair<std::string, std::string> parseHostAndPortFromRequest(const std::string &request)
{
    std::cout << request << std::endl;
    std::string hostHeader = "Host: ";
    size_t start = request.find(hostHeader);

    if (start == std::string::npos) return std::make_pair("", ""); // Host header not found
    start += hostHeader.length();
    size_t end = request.find("\r\n\r\n", start);
    if (end == std::string::npos) std::make_pair("", ""); // Malformed request

    std::string hostPortStr = request.substr(start, end - start);
    size_t colonPos = hostPortStr.find(':');
    if (colonPos != std::string::npos) {
        return std::make_pair(hostPortStr.substr(0, colonPos), hostPortStr.substr(colonPos + 1));
    } else {
        return std::make_pair(hostPortStr, ""); // No port specified
    }
}


HttpRequest RequestParser::parse(const std::string& request) {
    HttpRequest httpRequest;
    httpRequest.isCgi = false;
    httpRequest.statusCode = 200;
    std::istringstream requestStream(request);

    std::pair<std::string, std::string> hostPort = parseHostAndPortFromRequest(request);
    if (hostPort.first.empty() || hostPort.second.empty()) {
        httpRequest.statusCode = 400;
        return httpRequest;
    }
	ServerContext serverContext = _config->getServerContext(hostPort.second, hostPort.first);

    // 改行のみの場合は飛ばす
    if (requestStream.peek() == '\r') {
        requestStream.ignore();
    }
    // メソッドとURLを解析    
    requestStream >> httpRequest.method >> httpRequest.url >> httpRequest.protocol;
    
    // 正しくない形式の場合は飛ばす
    if (httpRequest.protocol.empty() || httpRequest.protocol.find("HTTP/") == std::string::npos || httpRequest.url.empty() || httpRequest.method.empty()) {
        httpRequest.statusCode = 400;
        return httpRequest;
    }
    // ヘッダーを解析
    std::string headerLine;
    int contentLength = 0; // Content-Lengthを保存する変数
    bool isCR = false;
    while (std::getline(requestStream, headerLine)) {
        // ヘッダーの終了判定
        if (headerLine == "\r" && isCR) {
            break;
        }
        if (headerLine == "\r") {
            isCR = true;
            continue;
        }
        isCR = false;
        std::istringstream headerStream(headerLine);
        std::string key;
        std::string value;
        std::getline(headerStream, key, ':');
        std::getline(headerStream, value);
        if (!value.empty() && value[0] == ' ') {
            value = value.substr(1); // コロンの後のスペースをスキップ
        }
        httpRequest.headers[key] = value;

        // Content-Lengthの取得
        if (key == "Content-Length") {
            contentLength = std::stoi(value);
        }

        

        // Transfer-Encodingが指定されている場合は、ボディを解析しない
        if (key == "Transfer-Encoding" && value == "chunked") {
            return httpRequest;
        }

    }
    // Method /path HTTP/1.1以外の場合はisRequestFinishedをfalseにする
    if (httpRequest.method == "" &&  httpRequest.url == "") {
        return httpRequest;
    }
    // ボディを解析 (Content-Lengthが指定されていれば)
    if (contentLength > 0) {
        int maxBodySize = std::stoi(serverContext.getMaxBodySize());
        if (contentLength > maxBodySize) {
            contentLength = maxBodySize;
        }
        char* buffer = new char[contentLength];
        requestStream.read(buffer, contentLength);
        httpRequest.body = std::string(buffer, contentLength);

        delete[] buffer;
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
