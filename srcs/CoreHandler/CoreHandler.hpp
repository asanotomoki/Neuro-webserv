#ifndef COREHANDLER_HPP
#define COREHANDLER_HPP

#include "ServerContext.hpp"
#include "RequestParser.hpp"

struct ParseUrlResult {
    std::string file;
    std::string directory;
    std::string fullpath;
    std::string query;
    std::string pathInfo;
    int statusCode;
    bool isAutoIndex;
};

#include "Cgi.hpp"
#include <string>

struct ProcessResult {
    std::string status;
    std::string message;
    int statusCode;

    ProcessResult(const std::string s, const std::string m, int c)
        : status(s), message(m), statusCode(c) {}
};



//大脳クラス
class CoreHandler
{
    private: 
        ParseUrlResult parseUrl(std::string url, const ServerContext& server_context);
    public:
    	CoreHandler();
    	~CoreHandler();
};

#endif
