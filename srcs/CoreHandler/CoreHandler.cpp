#include "CoreHandler.hpp"
#include "RequestParser.hpp"
#include "StaticFileReader.hpp"
#include "DataProcessor.hpp"
#include "LocationContext.hpp"
#include <iostream>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

CoreHandler::CoreHandler()
{
}

std::string redirectResponse(std::string location)
{
    std::string response = "HTTP/1.1 302 Moved Permanently\r\n";
    response += "Location: " + location + "\r\n";
    response += "\r\n";
    return response;
}

std::string successResponse(std::string fileContent, std::string contentType)
{
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + contentType + "; charset=UTF-8\r\n";
    response += "Content-Length: " + std::to_string(fileContent.size()) + "\r\n";
    response += "\r\n";
    response += fileContent;
    return response;
}

std::string errorResponse(int statusCode, std::string message, const LocationContext& locationContext)
{
    StaticFileReader fileReader;
    std::string fileContent = fileReader.readErrorFile(locationContext, statusCode);
    std::string response = "HTTP/1.1 " + std::to_string(statusCode) + " " + message + "\r\n"; 
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + std::to_string(fileContent.size()) + "\r\n";
    response += "\r\n";
    response += fileContent;
    return response;
}

std::string getMethod(const std::string& fullpath, const LocationContext& locationContext,
                        const ServerContext& serverContext, bool isAutoIndex)
{
    // 静的ファイルを提供する場合
    StaticFileReader fileReader;
    std::string fileContent = fileReader.readFile(fullpath, locationContext,
                                                        serverContext, isAutoIndex);
    std::string response = successResponse(fileContent, "text/html");
    return response;
}

std::string postMethod(std::string body)
{
    DataProcessor dataProcessor;
    ProcessResult result = dataProcessor.processPostData(body);
    std::string response = successResponse(result.message, "text/html");
    return response;
}

std::string deleteMethod(const std::string& filename, const ServerContext& serverContext)
{
    std::cout << "deleteMethod :: filename: " << filename << "\n";
    if (std::remove(("./docs/upload/" + filename).c_str()) != 0)
    {
        std::cerr << "deleteMethod :: ERROR: File not found or delete failed.\n";
        std::cout << "deleteMethod :: DELETE FAILED\n";
        LocationContext locationContext = serverContext.get404LocationContext();
        return errorResponse(404, "Not Found", locationContext);
    }
    return "HTTP/1.1 204 No Content\r\n\r\n"; // 成功のレスポンス
}


std::string CgiBlockMethod(HttpRequest &req, const ServerContext &serverContext, ParseUrlResult path)
{
    CGIContext cgiContext = serverContext.getCGIContext();
    std::string command  = cgiContext.getDirective("command");

    if (access(path.fullpath.c_str(), F_OK) == -1)
    {
        LocationContext locationContext = serverContext.get404LocationContext();
        return errorResponse(404, "Not Found", locationContext);
    }
    Cgi cgi(req, command, path);
    CgiResponse cgiResponse = cgi.CgiHandler();
    if (cgiResponse.status == 500)
    {
        LocationContext locationContext = serverContext.get500LocationContext();
        return errorResponse(cgiResponse.status, cgiResponse.message, locationContext);
    }
    else
    {
        return cgiResponse.message;
    }
}

std::string CgiMethod(HttpRequest &req, const ServerContext &serverContext, ParseUrlResult path)
{
    // Get Cgi Path, executable file path
    LocationContext locationContext = serverContext.getLocationContext("/cgi-bin/");
    std::string command = locationContext.getDirective("command");
    if (access(path.fullpath.c_str(), F_OK) == -1)
    {
        LocationContext locationContext = serverContext.get404LocationContext();
        return errorResponse(404, "Not Found", locationContext);
    }
    Cgi cgi(req, command, path);
    CgiResponse cgiResponse = cgi.CgiHandler();
    if (cgiResponse.status == 500)
    {
        locationContext = serverContext.get500LocationContext();
        return errorResponse(cgiResponse.status, cgiResponse.message, locationContext);
    }
    else
    {
        return cgiResponse.message;
    }
}

bool CoreHandler::isCgiBlock(const ServerContext &serverContext, const std::string path)
{
    bool isCgi = serverContext.getIsCgi();
    if (isCgi == true)
    {
        CGIContext cgiContext = serverContext.getCGIContext();
        std::string extension = cgiContext.getDirective("extension");
        if (path.find(extension) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}
bool CoreHandler::isCgi(const std::string dir)
{
    return dir == "/cgi-bin/";
}



std::string CoreHandler::processRequest(const std::string &request, const ServerContext &serverContext,
                                        const std::pair<std::string, std::string>& hostPort)
{
    // リクエストを解析
    RequestParser parser;
    HttpRequest httpRequest = parser.parse(request, serverContext);

    if (httpRequest.url == "/favicon.ico") {
        std::cout << "WARNING: favicon.ico request, ignoring\n";
        return "";
    }

    // httpRequest.urlが"/"で終わっていない場合に、"/"を追加
    if (httpRequest.url[httpRequest.url.size() - 1] != '/') {
        httpRequest.url += '/';
    }

    ParseUrlResult parseUrlResult = parseUrl(httpRequest.url, serverContext);

    LocationContext locationContext = serverContext.getLocationContext(parseUrlResult.directory);

    // directoryからLocationContextを取得
    locationContext = serverContext.getLocationContext(parseUrlResult.directory);
    if (parseUrlResult.statusCode >= 300 && parseUrlResult.statusCode < 400) {
        std::string location = "http://" + hostPort.first + ":" + hostPort.second + parseUrlResult.fullpath;
        return redirectResponse(location);
    }
    if (isCgiBlock(serverContext, httpRequest.url))
    {
        return CgiBlockMethod(httpRequest, serverContext, parseUrlResult);
    }
    else if (isCgi(parseUrlResult.directory))
    {
        return CgiMethod(httpRequest, serverContext,  parseUrlResult);
    }
    else if (httpRequest.method == "GET")
    {
        if (!locationContext.isAllowedMethod("GET"))
        {
            locationContext = serverContext.get405LocationContext();
            return errorResponse(405, "Method Not Allowed", locationContext);
        }
        return getMethod(parseUrlResult.fullpath, locationContext, serverContext,  parseUrlResult.isAutoIndex);
    }
    else if (httpRequest.method == "POST")
    {
        if (!locationContext.isAllowedMethod("POST"))
        {
            locationContext = serverContext.get405LocationContext();
            return errorResponse(405, "Method Not Allowed", locationContext);
        }
        return postMethod(httpRequest.body);
    }
    else if (httpRequest.method == "DELETE")
    {
        if (!locationContext.isAllowedMethod("DELETE"))
        {
            locationContext = serverContext.get405LocationContext();
            return errorResponse(405, "Method Not Allowed", locationContext);
        }
        return deleteMethod(parseUrlResult.file, serverContext);
    }
    locationContext = serverContext.get501LocationContext();
    return errorResponse(501, "Not Implemented", locationContext);
}

CoreHandler::~CoreHandler()
{
}