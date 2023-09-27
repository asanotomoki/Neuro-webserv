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

    std::cout << "successResponse :: SUCCESS\n" << response << std::endl;    

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

std::string getMethod(const std::string& fullpath, const LocationContext& locationContext,
                        const ServerContext& serverContext, bool isAutoIndex)
{
    // 静的ファイルを提供する場合
    StaticFileReader fileReader;
    std::string fileContent = fileReader.readFile(fullpath, locationContext,
                                                        serverContext, isAutoIndex);
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
    std::cout << "deleteMethod :: filename: " << filename << "\n";
    if (std::remove(("./docs/upload/" + filename).c_str()) != 0)
    {
        std::cerr << "deleteMethod :: ERROR: File not found or delete failed.\n";
        std::cout << "deleteMethod :: DELETE FAILED\n";
        LocationContext locationContext = serverContext.get404LocationContext();
        return errorResponse(404, "Not Found", locationContext);
    }

    std::cout << "deleteMethod :: DELETE SUCCESS\n";
    return "HTTP/1.1 204 No Content\r\n\r\n"; // 成功のレスポンス
}


std::string CgiBlockMethod(HttpRequest &req, const ServerContext &serverContext, ParseUrlResult path)
{
    CGIContext cgiContext = serverContext.getCGIContext();
    std::string command  = cgiContext.getDirective("command");

    std::cout << "CgiMethod :: command: " <<  command<< "\n";
    std::cout << "CgiMethod :: path.fullpath: " << path.fullpath << "\n";
    Cgi cgi(req, command, path);
    std::cout << "CGIBlockMethod :: cgi.CgiHandler(): " << "\n";
    CgiResponse cgiResponse = cgi.CgiHandler();
    if (access(path.fullpath.c_str(), F_OK) == -1)
    {
        std::cout << "CGIBlockMethod :: access failed\n";
        LocationContext locationContext = serverContext.get404LocationContext();
        return errorResponse(404, "Not Found", locationContext);
    }
    if (cgiResponse.status == 200)
    {
        return cgiResponse.message;
    }
    else
    {
        LocationContext locationContext = serverContext.get500LocationContext();
        return errorResponse(cgiResponse.status, cgiResponse.message, locationContext);
    }
}

std::string CgiMethod(HttpRequest &req, const ServerContext &serverContext, ParseUrlResult path)
{
    // Get Cgi Path, executable file path
    LocationContext locationContext = serverContext.getLocationContext("/cgi-bin/");
    std::string command = locationContext.getDirective("command");
    std::cout << "CgiMethod :: fullpath path: " << path.fullpath << "\n";
    std::cout << "CgiMethod :: command: " << command << "\n";
    Cgi cgi(req, command, path);
    CgiResponse cgiResponse = cgi.CgiHandler();
    if (access(path.fullpath.c_str(), F_OK) == -1)
    {
        std::cout << "CGIBlockMethod :: access failed\n";
        LocationContext locationContext = serverContext.get404LocationContext();
        return errorResponse(404, "Not Found", locationContext);
    }
    if (cgiResponse.status == 200)
    {
        return cgiResponse.message;
    }
    else
    {
        locationContext = serverContext.get500LocationContext();
        return errorResponse(cgiResponse.status, cgiResponse.message, locationContext);
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

bool isDirectory(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false; // エラー時はfalse
    } else if (info.st_mode & S_IFDIR) {
        return true; // ディレクトリです
    } else {
        return false; // ディレクトリではありません
    }
}

bool isFile(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false; // エラー時はfalse
    } else if (info.st_mode & S_IFREG) {
        return true; // 通常のファイルです
    } else {
        return false; // 通常のファイルではありません
    }
}

std::string CoreHandler::processRequest(const std::string &request, const ServerContext &serverContext)
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

    std::cout << "parseUrlResult.file: " << parseUrlResult.file << "\n";
    std::cout << "parseUrlResult.directory: " << parseUrlResult.directory << "\n";
    std::cout << "parseUrlResult.fullpath: " << parseUrlResult.fullpath << "\n";

    // std::string redirectPath = serverContext.getReturnPath(directory);
    // if (!redirectPath.empty())
    //     directory = redirectPath;
    LocationContext locationContext = serverContext.getLocationContext(parseUrlResult.directory);


    //// httpRequest.urlをディレクトリとファイルに分割
    //std::string directory, file;
    //// 先頭から次の'/'までをディレクトリとする
    //size_t nextSlash = httpRequest.url.find_first_of("/", 1);
    //directory = httpRequest.url.substr(0, nextSlash + 1);
    //std::cout << "processRequest :: directory: " << directory << "\n";
    //// nextSlash以降、末尾の'/'の手前までをファイル名とする
    //file = httpRequest.url.substr(nextSlash + 1, httpRequest.url.length() - nextSlash - 2); // 末尾の"/"を除く
    

    //// return ディレクティブがある場合にディレクトリを置き換える。
    //std::string redirectPath = serverContext.getReturnPath(directory);
    //std::cout << "processRequest :: redirectPath: " << redirectPath << "\n";
    //if (!redirectPath.empty())
    //    directory = redirectPath;


    //LocationContext locationContext = serverContext.getLocationContext(directory); 
    //std::string alias = locationContext.getDirective("alias");
    //std::string fullPath = alias + file;
    //if (fullPath.back() == '/')
    //    fullPath.pop_back();
    //std::cout << "processRequest :: fullPath: " << fullPath << "\n";
    

    // httpRequest.urlに'/'が２つ以下しか含まれない場合に以下の処理を行う
    //if (std::count(httpRequest.url.begin(), httpRequest.url.end(), '/') <= 2) {
    //    if (isDirectory(fullPath)) {
    //        if (directory.back() != '/')
    //            directory += "/";
    //    } else if (isFile(fullPath)) {
    //        file = directory.substr(1, directory.size() - 2);
    //        directory = "/";
    //    } else {
    //        std::cout << "processRequest :: NOT FOUND\n";
    //        locationContext = serverContext.get404LocationContext();
    //        return errorResponse(404, "Not Found", locationContext);
    //    }
    //}
    //std::cout << "processRequest :: directory: " << directory << ", file: " << file << "\n";
    
    
    // directoryからLocationContextを取得
    locationContext = serverContext.getLocationContext(parseUrlResult.directory);
    if (parseUrlResult.statusCode >= 300 && parseUrlResult.statusCode < 400) {
        std::cout << "processRequest :: REDIRECT\n";
        std::string location = "http://localhost:2000" + parseUrlResult.fullpath;
        return redirectResponse(location);
    }
    if (isCgiBlock(serverContext, httpRequest.url))
    {
        std::string res = CgiBlockMethod(httpRequest, serverContext, parseUrlResult);
        std::cout << "Is CGI BLOCK :: res: " << res << "\n";
        return res;
    }
    else if (isCgi(parseUrlResult.directory))
    {
        std::string res = CgiMethod(httpRequest, serverContext,  parseUrlResult);
        std::cout << "Is CGI :: res: " << res << "\n";
        return res;
    }
    else if (httpRequest.method == "GET")
    {
        if (!locationContext.isAllowedMethod("GET"))
        {
            std::cout << "processRequest :: GET FAILED\n";
            locationContext = serverContext.get405LocationContext();
            return errorResponse(405, "Method Not Allowed", locationContext);
        }
        return getMethod(parseUrlResult.fullpath, locationContext, serverContext,  parseUrlResult.isAutoIndex);
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
        return deleteMethod(parseUrlResult.file, serverContext);
    }
    std::cout << "processRequest :: NOT IMPLEMENTED\n";
    locationContext = serverContext.get501LocationContext();
    return errorResponse(501, "Not Implemented", locationContext);
}

CoreHandler::~CoreHandler()
{
}