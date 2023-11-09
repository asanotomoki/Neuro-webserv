#ifndef REQUEST_PARSER_HPP
#define REQUEST_PARSER_HPP

#include "ServerContext.hpp"
#include "Config.hpp"
#include <string>
#include <map>

struct HttpRequest {
    std::string method;
    std::string protocol; // HTTP/1.1
    std::string url;
    std::map<std::string, std::string> headers;
    std::string body;
    std::string hostname;
    bool isCgi;
    int fd;
    int statusCode;
};

// 大脳における感覚情報の解析と処理
class RequestParser
{
public:
    HttpRequest parse(const std::string &request, bool isChunked, const std::string &client,
                        const std::string& sessionId);
    RequestParser(Config *config);
    ~RequestParser();

private:
    HttpRequest _httpRequest;
    RequestParser();
    Config *_config;
    bool isCgiBlockPath(const ServerContext &server_context, std::vector<std::string> tokens);
    bool isCgiDir(std::vector<std::string> tokens, const ServerContext &server_context);
    HttpRequest _HttpRequest;
};

#endif // REQUEST_PARSER_HPP
