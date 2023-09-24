#ifndef COREHANDLER_HPP
#define COREHANDLER_HPP

#include "ServerContext.hpp"
#include "RequestParser.hpp"
#include "Cgi.hpp"
#include <string>

struct ProcessResult {
    std::string status;
    std::string message;
    int statusCode;
};

struct ParseUrlResult {
    std::string file;
    std::string directory;
    std::string fullpath;
    std::string query;
    std::string pathInfo;
    bool isAutoIndex;
};

//大脳クラス
class CoreHandler
{
    private: 
        bool isCgi(const ServerContext& server_context, std::string path);
        bool isCgiBlock(const ServerContext& server_context, std::string path);
        ParseUrlResult parseUrl(std::string url, const ServerContext& server_context);
    public:
    CoreHandler();
    std::string processRequest(const std::string& request, const ServerContext& server_context);
    ~CoreHandler();
};

#endif
