#include "CoreHandler.hpp"
#include "RequestParser.hpp"
#include "StaticFileReader.hpp"
#include "DataProcessor.hpp"
#include <iostream>
#include <algorithm>

CoreHandler::CoreHandler()
{
    // 必要に応じて初期化処理をここに記述
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

std::string errorResponse(int statusCode, std::string message, const ServerContext &serverContext)
{
    StaticFileReader fileReader;
    std::string fileContent = fileReader.readErrorFile(statusCode, serverContext);
    std::string response = "HTTP/1.1 " + std::to_string(statusCode) + " " + message + "\r\n"; 
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + std::to_string(fileContent.size()) + "\r\n";
    response += "\r\n";
    response += fileContent;
    return response;
}

std::string getMethod(std::string filePath, const ServerContext &serverContext)
{
    // 静的ファイルを提供する場合
    StaticFileReader fileReader;
    std::string fileContent = fileReader.readFile(filePath, "GET", serverContext);

    std::string response = successResponse(fileContent, "text/html");

    std::cout << "DEBUG MSG: GET SUCCESS\n";
    return response;
}

std::string postMethod(std::string body, const ServerContext &serverContext, std::string path)
{
    std::string filePath = getLocationPath(path);
    LocationContext locationContext = serverContext.getLocationContext(getLocationPath(path));

    if (locationContext.isAllowedMethod("POST") == false)
    {
        std::cout << "DEBUG MSG: POST FAILED\n";
        return errorResponse(405, "Method Not Allowed", serverContext);
    }
    DataProcessor dataProcessor;
    ProcessResult result = dataProcessor.processPostData(body, locationContext);
    // TODO FIX!! エラーハンドリングを追加する必要がある

    // レスポンスの生成
    std::string response = successResponse(result.message, "text/html");

    std::cout << "DEBUG MSG: POST SUCCESS\n";
    std::cout << "DEBUG MSG:: response: " << response << "\n";
    return response;
}

std::string deleteMethod(std::string url, const ServerContext &serverContext)
{
    LocationContext locationContext = serverContext.getLocationContext(getLocationPath(url));
    if (locationContext.isAllowedMethod("DELETE") == false)
    {
        std::cout << "DEBUG MSG: DELETE FAILED\n";
        return errorResponse(405, "Method Not Allowed", serverContext);
    }
    // urlからファイルパスを生成
    // url = /form/delete/uploaded.txt -> url = /uploaded.txt
    // url = /form/delete/uploaded.txt?hoge=fuga -> url = /uploaded.txt
    url = url.substr(url.find_last_of("/"));
    if (std::remove(("./docs/upload" + url).c_str()) != 0)
    {
        std::cerr << "ERROR: File not found or delete failed.\n";
        std::cout << "DEBUG MSG: DELETE FAILED\n";

        return errorResponse(404, "Not Found", serverContext);
    }

    std::cout << "DEBUG MSG: DELETE SUCCESS\n";
    return "HTTP/1.1 204 No Content\r\n\r\n"; // 成功のレスポンス
}

std::string getPath(std::string url, const ServerContext &serverContext)
{
    std::string path = url;
    if (path.find("?") != std::string::npos)
    {
        path = path.substr(0, path.find("?"));
    }
    return path;
}

std::string getExecutablePath(const ServerContext &serverContext)
{
    std::string res = "";
    return res;
}

std::string CgiMethod(HttpRequest &req, const ServerContext &serverContext)
{
    // Get Cgi Path, executable file path
    std::string executablePath = getExecutablePath(serverContext);
    std::cout << "DEBUG MSG: executablePath: " << executablePath << "\n";
    std::string path = getPath(req.url, serverContext);
    Cgi cgi(req, executablePath, path);
    return cgi.CgiHandler();
}

bool CoreHandler::isCgi(const std::string &request, const ServerContext &serverContext, const std::string path)
{
    //bool isCgi = serverContext.getIsCgi();
    //if (isCgi == true)
    //{
    //    CGIContext cgiContext = serverContext.getCGIContext();
    //    std::string extension = cgiContext.getDirective("extension");
    //    if (path.find(extension) != std::string::npos)
    //    {
    //        return true;
    //    }
    //}
    // /upload
    LocationContext locationContext = serverContext.getLocationContext(getLocationPath(path));
    bool res = locationContext.getIsCgi();
    if (res) {
        std::cout << "DEBUG MSG: isCgi: " << "true" << "\n";
    } else {
        std::cout << "DEBUG MSG: isCgi: " << "false" << "\n";
    }

    return res;
}

std::string CoreHandler::processRequest(const std::string &request, const ServerContext &serverContext)
{
    // リクエストを解析
    RequestParser parser;
    HttpRequest httpRequest = parser.parse(request, serverContext);
    // requestPathが"/"で終わっていない場合に、"/"を追加
    if (httpRequest.url[httpRequest.url.size() - 1] != '/') {
        httpRequest.url += '/';
    }
    std::cout << "DEBUG MSG:: httpRequest.url: " << httpRequest.url << "\n";
    size_t nextSlash = httpRequest.url.find_first_of("/", 1);
    std::string directoryPath = httpRequest.url.substr(0, nextSlash + 1);
    std::cout << "DEBUG MSG: directoryPath: " << directoryPath << "\n";
    // directoryPathからLocationContextを取得
    std::string returnPath = serverContext.getReturnPath(directoryPath);
    std::cout << "DEBUG MSG: returnPath: " << returnPath << "\n";
    if (!returnPath.empty()) {
        httpRequest.url = returnPath;
    }

    if (isCgi(request, serverContext, httpRequest.url))
    {
        std::cout << "DEBUG MSG: path: " << "IS CGI" << "\n";
        return CgiMethod(httpRequest, serverContext);
    }
    else if (httpRequest.method == "GET")
    {
        return getMethod(httpRequest.url, serverContext);
    }
    else if (httpRequest.method == "POST")
    {
        return postMethod(httpRequest.body, serverContext, httpRequest.url);
    }
    else if (httpRequest.method == "DELETE")
    {
        return deleteMethod(httpRequest.url, serverContext);
    }
    std::cout << "DEBUG MSG: NOT IMPLEMENTED\n";
    return errorResponse(501, "Not Implemented", serverContext);
}

CoreHandler::~CoreHandler()
{
    // 必要に応じてクリーンアップ処理をここに記述
}
