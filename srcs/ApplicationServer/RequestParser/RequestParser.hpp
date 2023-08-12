#ifndef REQUEST_PARSER_HPP
#define REQUEST_PARSER_HPP

#include <string>
#include <map>

struct HttpRequest {
    std::string method;
    std::string url;
    std::map<std::string, std::string> headers;
};

//大脳における感覚情報の解析と処理
class RequestParser {
public:
    HttpRequest parse(const std::string& request);
};

#endif // REQUEST_PARSER_HPP
