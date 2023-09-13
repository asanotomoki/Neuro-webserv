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

std::string successResponse(std::string fileContent, std::string contentType)
{
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + std::to_string(fileContent.size()) + "\r\n";
    response += "\r\n";
    response += fileContent;
    return response;
}

std::string errorResponse(int statusCode, std::string message, std::string fileContent)
{
    std::string response = "HTTP/1.1 " + std::to_string(statusCode) + " " + message + "\r\n";
    // response += "Content-Type: text/html\r\n";
    // response += "Content-Length: " + std::to_string(fileContent.size()) + "\r\n";
    response += "\r\n";
    response += fileContent;
    return response;
}

std::string methodNotAllowedResponse(const ServerContext &server_context)
{
    StaticFileReader fileReader;
    std::string fileContent = fileReader.readFile("405.html", "GET", server_context); // TODO FIX!! 405.html は動的に変更できるようにする

    // ERROR レスポンスの生成
    std::string response = errorResponse(405, "Method Not Allowed", fileContent);
    return response;
}

std::string getMethod(std::string filePath, const ServerContext &server_context)
{
    // 静的ファイルを提供する場合
    StaticFileReader fileReader;
    std::string fileContent = fileReader.readFile(filePath, "GET", server_context);

    std::string response = successResponse(fileContent, "text/html");

    std::cout << "DEBUG MSG: GET SUCCESS\n";
    return response;
}

std::string postMethod(std::string body, const ServerContext &server_context)
{
    LocationContext locationContext = server_context.getLocationContext("/upload"); // TODO FIX!! /upload is magic number

    if (locationContext.isAllowedMethod("POST") == false)
    {
        std::cout << "DEBUG MSG: POST FAILED\n";
        return methodNotAllowedResponse(server_context);
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

std::string deleteMethod(std::string url, const ServerContext &server_context)
{
    LocationContext locationContext = server_context.getLocationContext("/upload");
    if (locationContext.isAllowedMethod("DELETE") == false)
    {
        std::cout << "DEBUG MSG: DELETE FAILED\n";
        return methodNotAllowedResponse(server_context);
    }
    if (std::remove(("." + url).c_str()) != 0)
    {
        std::cerr << "ERROR: File not found or delete failed.\n";
        StaticFileReader fileReader;
        std::string fileContent = fileReader.readFile("404.html", "GET", server_context);

        // ERROR レスポンスの生成
        std::string response = errorResponse(404, "Not Found", fileContent);

        std::cout << "DEBUG MSG: DELETE FAILED\n";
        return response;
    }

    std::cout << "DEBUG MSG: DELETE SUCCESS\n";
    return "HTTP/1.1 204 No Content\r\n\r\n"; // 成功のレスポンス
}

std::string CoreHandler::processRequest(const std::string &request, const ServerContext &server_context)
{
    // リクエストを解析
    RequestParser parser;
    HttpRequest httpRequest = parser.parse(request, server_context);
    std::cout << "DEBUG MSG: filePath: " << httpRequest.url << "\n";

    if (httpRequest.method == "GET")
    {
        return getMethod(httpRequest.url, server_context);
    }
    else if (httpRequest.method == "POST")
    {
        return postMethod(httpRequest.body, server_context);
    }
    else if (httpRequest.method == "DELETE")
    {
        return deleteMethod(httpRequest.url , server_context);
    }
    // 未実装のメソッドの場合
    StaticFileReader fileReader;
    std::string fileContent = fileReader.readFile("501.html", "GET", server_context);

    // ERROR レスポンスの生成
    std::string response = errorResponse(501, "Not Implemented", fileContent);

    std::cout << "DEBUG MSG: NOT IMPLEMENTED\n";
    return response;
}

CoreHandler::~CoreHandler()
{
    // 必要に応じてクリーンアップ処理をここに記述
}
