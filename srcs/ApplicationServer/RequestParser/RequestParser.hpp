#ifndef REQUEST_PARSER_HPP
#define REQUEST_PARSER_HPP

#include "ServerContext.hpp"
#include <string>
#include <map>

struct HttpRequest {
    std::string method;
    std::string url;
    std::map<std::string, std::string> headers;
    std::string body;
};

//大脳における感覚情報の解析と処理
class RequestParser {
public:
    HttpRequest parse(const std::string& request, const ServerContext& context);
};

#endif // REQUEST_PARSER_HPP
