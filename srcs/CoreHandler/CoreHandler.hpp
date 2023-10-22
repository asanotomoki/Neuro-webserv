#ifndef COREHANDLER_HPP
#define COREHANDLER_HPP

#include <string>

#include "ServerContext.hpp"
#include "RequestParser.hpp"
#include "StaticFileReader.hpp"

struct ParseUrlResult
{
    std::string file;
    std::string directory;
    std::string fullpath;
    std::string query;
    std::string pathInfo;
    int statusCode;
    bool isAutoIndex;
};

struct ProcessResult
{
    std::string status;
    std::string message;
    int statusCode;

    ProcessResult(const std::string s, const std::string m, int c)
        : status(s), message(m), statusCode(c) {}
};

#include "DataProcessor.hpp"
// 大脳クラス
class CoreHandler
{
private:
    ParseUrlResult parseUrl(std::string url);
    ServerContext _serverContext;
    std::string getMethod(const std::string &fullpath, const LocationContext &locationContext);
    std::string postMethod(std::string body);
    std::string deleteMethod(const std::string &filename);
    CoreHandler();

public:
    CoreHandler(const ServerContext &serverContext);
    ~CoreHandler();
    std::string processRequest(HttpRequest httpRequest);
};

#endif
