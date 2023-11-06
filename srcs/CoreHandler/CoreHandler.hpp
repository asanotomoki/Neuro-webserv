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
    std::string location;

    ProcessResult(const std::string s, const std::string m, int c, const std::string l)
        : status(s), message(m), statusCode(c), location(l) {}
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
    std::string postMethod(const std::string& body, const std::string& url);
<<<<<<< HEAD
    std::string deleteMethod(const std::string& directory, const std::string& file);
=======
    std::string deleteMethod(const std::string& fullpath);
>>>>>>> other/main
    std::string getFile(std::vector<std::string> tokens, LocationContext &locationContext, ParseUrlResult &result);
    int isFile(const std::string& token, std::string fullpath = "");
    int validatePath(std::string& path);
    bool isFileIncluded(std::vector<std::string> tokens);
    void parseHomeDirectory(std::string url, ParseUrlResult& result);
    CoreHandler();

public:
    CoreHandler(const ServerContext &serverContext);
    ~CoreHandler();
    std::string processRequest(HttpRequest httpRequest, const std::pair<std::string, std::string> &hostPort);
};

#endif
