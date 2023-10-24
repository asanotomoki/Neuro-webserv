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
    return tokens.size() >= 2 && tokens[1] == "cgi-bin";
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




HttpRequest RequestParser::parse(const std::string& request, bool isChunked) {
    HttpRequest httpRequest;
    httpRequest.isCgi = false;
    httpRequest.statusCode = 200;
    std::istringstream requestStream(request);

	ServerContext serverContext = _config->getServerContext("2000", "localhost"); //TODO FIX 動的に取得する
    // メソッドとURLを解析
    requestStream >> httpRequest.method >> httpRequest.url >> httpRequest.protocol;

    // 正しくない形式の場合は400を返す
    if (httpRequest.protocol.empty() || httpRequest.protocol.find("HTTP/1.1") == std::string::npos || httpRequest.url.empty() || httpRequest.method.empty()) {
        std::cout << "400 Bad Request" << std::endl;
        httpRequest.statusCode = 400;
        return httpRequest;
    }
    // ヘッダーを解析
    std::string headerLine;
    int contentLength = -1; // Content-Lengthを保存する変数
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
    }
    if (isChunked)
    {
        // header以降のデータを読み込む
        // requestにbody(sizeを除いたもの)が入っている
        std::cout << "request: " << request << std::endl;
        
    }
    if (httpRequest.method == "POST" && contentLength == -1 && !isChunked) {
        std::cout << "411 Length Required" << std::endl;
        httpRequest.statusCode = 411;
        return httpRequest;
    }
    // ボディを解析 (Content-Lengthが指定されていれば)
    if (contentLength > 0) {
        int maxBodySize = std::stoi(serverContext.getMaxBodySize()); //TODO FIX!!
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
