#ifndef COREHANDLER_HPP
#define COREHANDLER_HPP

#include "ServerContext.hpp"
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
public:
    CoreHandler();
    std::string processRequest(const std::string& request, const ServerContext& server_context);
    ~CoreHandler();
};

#endif
