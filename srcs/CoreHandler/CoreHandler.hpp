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

//大脳クラス
class CoreHandler
{
    private: 
        bool isCgi(const std::string& request, const ServerContext& server_context, std::string path);
        bool isCgiBlock(const ServerContext& server_context, std::string path);
    public:
    CoreHandler();
    std::string processRequest(const std::string& request, const ServerContext& server_context);
    ~CoreHandler();
};

#endif
