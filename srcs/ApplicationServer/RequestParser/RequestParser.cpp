#include "RequestParser.hpp"
#include <sstream>
#include <iostream>

HttpRequest RequestParser::parse(const std::string& request) {
    HttpRequest httpRequest;
    std::istringstream requestStream(request);

    // メソッドとURLを解析
    requestStream >> httpRequest.method >> httpRequest.url;

    // ヘッダーを解析
    std::string headerLine;
    
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
    }
    // ボディを解析 (存在すれば)
    std::string bodyLine;
    std::ostringstream bodyStream;
    while (std::getline(requestStream, bodyLine)) {
        bodyStream << bodyLine << "\r\n";
    }
    httpRequest.body = bodyStream.str();

    return httpRequest;
}
