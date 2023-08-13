#include "RequestParser.hpp"
#include <sstream>
#include <iostream>

HttpRequest RequestParser::parse(const std::string& request) {
    HttpRequest httpRequest;
    std::istringstream requestStream(request);

    std::cout << "DEBUG MESSAGE 1\n";

    // メソッドとURLを解析
    requestStream >> httpRequest.method >> httpRequest.url;

    std::cout << "DEBUG MESSAGE 2\n";

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

    std::cout << "DEBUG MESSAGE 3\n";

    return httpRequest;
}
