#include "CoreHandler.hpp"
#include "RequestParser.hpp"
#include "StaticFileReader.hpp"
#include "DataProcessor.hpp"
#include "LocationContext.hpp"
#include <iostream>
#include <algorithm>

CoreHandler::CoreHandler()
{
}

std::string getLocationPath (std::string url)
{
    // パスの開始位置を探す
    size_t start_pos = url.find_first_of('/');

    // クエリ文字列の開始位置または文字列の終端を探す
    size_t end_pos = url.find_first_of("?#", start_pos);

    // もしクエリ文字列またはフラグメント識別子が見つからなかったら、文字列の終端を使用する
    if (end_pos == std::string::npos) {
        end_pos = url.length();
    }

    // スラッシュの後の第二のスラッシュの位置を探す
    size_t second_slash_pos = url.find_first_of('/', start_pos + 1);

    // 第二のスラッシュが見つかったら、その位置までのパスを返す
    if (second_slash_pos != std::string::npos && second_slash_pos < end_pos) {
        return url.substr(start_pos, second_slash_pos - start_pos) + "/";
    } else {
        // そうでなければ、クエリ文字列の開始位置または文字列の終端までのパスを返す
        return url.substr(start_pos, end_pos - start_pos) + "/";
    }
}

std::string successResponse(std::string fileContent, std::string contentType)
{
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + std::to_string(fileContent.size()) + "\r\n";
    response += "\r\n";
    response += fileContent;
    return response;
}

std::string errorResponse(int statusCode, std::string message, const LocationContext& locationContext)
{
    StaticFileReader fileReader;
    std::string fileContent = fileReader.readErrorFile(locationContext);
    std::string response = "HTTP/1.1 " + std::to_string(statusCode) + " " + message + "\r\n"; 
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + std::to_string(fileContent.size()) + "\r\n";
    response += "\r\n";
    response += fileContent;
    return response;
}

std::string getMethod(const std::string& filename, const LocationContext& locationContext,
                        const ServerContext& serverContext)
{
    // 静的ファイルを提供する場合
    StaticFileReader fileReader;
    std::string fileContent = fileReader.readFile(filename, locationContext,
                                                        serverContext);
    std::string response = successResponse(fileContent, "text/html");

    std::cout << "getMethod :: GET SUCCESS\n";
    return response;
}

std::string postMethod(std::string body)
{
    DataProcessor dataProcessor;
    ProcessResult result = dataProcessor.processPostData(body);
    // TODO FIX!! エラーハンドリングを追加する必要がある

    // レスポンスの生成
    std::string response = successResponse(result.message, "text/html");

    std::cout << "postMethod :: POST SUCCESS\n";
    std::cout << "postMethod :: response: " << response << "\n";
    return response;
}

std::string deleteMethod(const std::string& filename, const ServerContext& serverContext)
{
    if (std::remove(("./docs/upload" + filename).c_str()) != 0)
    {
        std::cerr << "deleteMethod :: ERROR: File not found or delete failed.\n";
        std::cout << "deleteMethod :: DELETE FAILED\n";
        LocationContext locationContext = serverContext.get404LocationContext();
        return errorResponse(404, "Not Found", locationContext);
    }

    std::cout << "deleteMethod :: DELETE SUCCESS\n";
    return "HTTP/1.1 204 No Content\r\n\r\n"; // 成功のレスポンス
}

std::string CgiBlockMethod(HttpRequest &req, const ServerContext &serverContext, std::string file)
{
    CGIContext cgiContext = serverContext.getCGIContext();
    std::string command  = cgiContext.getDirective("command");
    std::string path = cgiContext.getDirective("alias") + file;
    std::cout << "CgiMethod :: command: " <<  command<< "\n";
    std::cout << "CgiMethod :: path: " << path << "\n";
    Cgi cgi(req, command, path);
    return cgi.CgiHandler();
}

std::string CgiMethod(HttpRequest &req, const ServerContext &serverContext, std::string file)
{
    // Get Cgi Path, executable file path
    LocationContext locationContext = serverContext.getLocationContext(getLocationPath(req.url));
    std::string command = locationContext.getDirective("command");
    std::cout << "CgiMethod :: command: " <<  command<< "\n";
    std::string path = locationContext.getDirective("alias") + file;
    std::cout << "CgiMethod :: path: " << path << "\n";
    Cgi cgi(req, command, path);
    return cgi.CgiHandler();
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
bool CoreHandler::isCgi(const std::string &request, const ServerContext &serverContext, const std::string path)
{
    LocationContext locationContext = serverContext.getLocationContext(getLocationPath(path));
    bool res = locationContext.getIsCgi();
    return res;
}

std::string CoreHandler::processRequest(const std::string &request, const ServerContext &serverContext)
{
    // リクエストを解析
    RequestParser parser;
    HttpRequest httpRequest = parser.parse(request, serverContext);

    if (httpRequest.url == "/favicon.ico") {
        std::cout << "WARNING: favicon.ico request, ignoring\n";
        return {};
    }
    // directoryname, filenameを取得
    // locationContextも取得。
    // requestPathが"/"で終わっていない場合に、"/"を追加
    if (httpRequest.url[httpRequest.url.size() - 1] != '/') {
        httpRequest.url += '/';
    }
    std::string directory, file;
    // httpRequest.urlをディレクトリとファイルに分割
    // 先頭から次の'/'までをディレクトリとする
    size_t nextSlash = httpRequest.url.find_first_of("/", 1);
    directory = httpRequest.url.substr(0, nextSlash + 1);
    // nextSlash以降、末尾の'/'の手前までをファイル名とする
    file = httpRequest.url.substr(nextSlash + 1, httpRequest.url.length() - nextSlash - 2); // 末尾の"/"を除く
    // ディレクトリが"/"の場合、ディレクトリ名としてfileを設定し、fileを空にする
    if (directory == "/") {
        directory += file;
        file = "";
    }
    // fileが空でない場合、directoryの末尾に'/'を追加
    if (file.empty() && directory.back() != '/') {
        directory += "/";
    }
    std::cout << "processRequest :: directory: " << directory << ", file: " << file << "\n";

    
    // directoryからLocationContextを取得
    std::string redirectPath = serverContext.getReturnPath(directory);
    std::cout << "processRequest :: redirectPath: " << redirectPath << "\n";
    LocationContext locationContext;
    if (!redirectPath.empty()) {
        locationContext = serverContext.getLocationContext(redirectPath);
    }
    else {
        locationContext = serverContext.getLocationContext(directory);
    }
    if (isCgiBlock(serverContext, httpRequest.url))
    {
        std::cout << "processRequest :: path: " << "IS CGI BLOCK" << "\n";
        return CgiBlockMethod(httpRequest, serverContext, file);
    }
    else if (isCgi(request, serverContext, httpRequest.url))
    {
        std::cout << "processRequest :: path: " << "IS CGI" << "\n";
        return CgiMethod(httpRequest, serverContext, file);
    }
    else if (httpRequest.method == "GET")
    {
        if (!locationContext.isAllowedMethod("GET"))
        {
            std::cout << "processRequest :: GET FAILED\n";
            locationContext = serverContext.get405LocationContext();
            return errorResponse(405, "Method Not Allowed", locationContext);
        }
        return getMethod(file, locationContext, serverContext);
    }
    else if (httpRequest.method == "POST")
    {
        if (!locationContext.isAllowedMethod("POST"))
        {
            std::cout << "processRequest :: POST FAILED\n";
            locationContext = serverContext.get405LocationContext();
            return errorResponse(405, "Method Not Allowed", locationContext);
        }
        return postMethod(httpRequest.body);
    }
    else if (httpRequest.method == "DELETE")
    {
        if (!locationContext.isAllowedMethod("DELETE"))
        {
            std::cout << "processRequest :: DELETE FAILED\n";
            locationContext = serverContext.get405LocationContext();
            return errorResponse(405, "Method Not Allowed", locationContext);
        }
        return deleteMethod(file, serverContext);
    }
    std::cout << "processRequest :: NOT IMPLEMENTED\n";
    locationContext = serverContext.get501LocationContext();
    return errorResponse(501, "Not Implemented", locationContext);
}

CoreHandler::~CoreHandler()
{
}
