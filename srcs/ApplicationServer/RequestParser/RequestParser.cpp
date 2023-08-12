#include "RequestParser.hpp"
#include <sstream>

HttpRequest RequestParser::parse(const std::string& request) {
    HttpRequest httpRequest;
    std::istringstream requestStream(request);

    // メソッドとURLを解析
    requestStream >> httpRequest.method >> httpRequest.url;

    // ヘッダーを解析
    std::string headerLine;
    while (std::getline(requestStream, headerLine) && headerLine != "\r") {
        std::istringstream headerStream(headerLine);
        std::string key;
        std::string value;
        std::getline(headerStream, key, ':');
        std::getline(headerStream, value);
        httpRequest.headers[key] = value.substr(1); // コロンの後のスペースをスキップ
    }
    return httpRequest;
}
