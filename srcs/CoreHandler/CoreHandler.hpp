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
    std::string message;
    int autoindex;
    int errorflag;
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
    std::string getMethod(const std::string &fullpath, const LocationContext &locationContext,
                            const ParseUrlResult& result);
    std::string postMethod(std::string body);
    std::string deleteMethod(const std::string &filename);
    std::string getFile(std::vector<std::string> tokens, LocationContext &locationContext, ParseUrlResult &result);
    LocationContext determineLocationContext(ParseUrlResult &result);
    int isFile(const std::string& token, std::string fullpath = "");
    int validatePath(const std::string& path);
    bool isFileIncluded(std::vector<std::string> tokens);
    CoreHandler();

public:
    CoreHandler(const ServerContext &serverContext);
    ~CoreHandler();
    std::string processRequest(HttpRequest httpRequest);
};

#endif
