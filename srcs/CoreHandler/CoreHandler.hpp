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
        bool isCgi(std::string dir);
        bool isCgiBlock(const ServerContext& server_context, std::string path);
        ParseUrlResult parseUrl(std::string url, const ServerContext& server_context);
    public:
    CoreHandler();
    std::string processRequest(const std::string& request, const ServerContext& server_context);
    ~CoreHandler();
};

#endif
