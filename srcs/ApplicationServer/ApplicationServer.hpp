#ifndef APPLICATION_SERVER_HPP
#define APPLICATION_SERVER_HPP

#include "ServerContext.hpp"
#include <string>

//大脳クラス
class ApplicationServer
{
public:
    ApplicationServer();
    std::string processRequest(const std::string& request, const ServerContext& server_context);
    ~ApplicationServer();
};

#endif
