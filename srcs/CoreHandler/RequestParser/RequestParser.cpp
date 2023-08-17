#include "RequestParser.hpp"
#include <sstream>
#include <iostream>

HttpRequest RequestParser::parse(const std::string& request, const ServerContext &serverContext) {
    HttpRequest httpRequest;
    std::istringstream requestStream(request);

    // メソッドとURLを解析
    requestStream >> httpRequest.method >> httpRequest.url;

    // ヘッダーを解析
    std::string headerLine;
    int contentLength = 0; // Content-Lengthを保存する変数

    while (std::getline(requestStream, headerLine)) {
        if (headerLine == "\r" || headerLine.empty())
            break;

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
    std::cout << "DEBUG MSG:: contentLength: " << contentLength << std::endl;
    // ボディを解析 (Content-Lengthが指定されていれば)
    if (contentLength > 0) {
        if (contentLength > std::stoi(serverContext.getMaxBodySize())) {
            contentLength = std::stoi(serverContext.getMaxBodySize());
        }
        char* buffer = new char[contentLength];
        requestStream.read(buffer, contentLength);
        httpRequest.body = std::string(buffer, contentLength);
        std::cout << "DEBUG MSG:: body: " << httpRequest.body << std::endl;
        delete[] buffer;
    }

    return httpRequest;
}
