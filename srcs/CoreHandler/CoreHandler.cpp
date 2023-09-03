#include "CoreHandler.hpp"
#include "RequestParser.hpp"
#include "StaticFileReader.hpp"
#include "DataProcessor.hpp"
#include <iostream>
#include <algorithm>

CoreHandler::CoreHandler() {
    // 必要に応じて初期化処理をここに記述
}

std::string CoreHandler::processRequest(const std::string& request, const ServerContext& server_context) {
    // リクエストを解析
    RequestParser parser;
    HttpRequest httpRequest = parser.parse(request, server_context);
    std::cout << "DEBUG MSG: filePath: " << httpRequest.url << "\n";

    if (httpRequest.method == "GET") {
        // 静的ファイルを提供する場合
        StaticFileReader fileReader;
        std::string fileContent = fileReader.readFile(httpRequest.url, httpRequest.method, server_context);

        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n"; // もしHTMLファイルであれば
        response += "Content-Length: " + std::to_string(fileContent.size()) + "\r\n";
        response += "\r\n";
        response += fileContent;

        std::cout << "DEBUG MSG: GET SUCCESS\n";
        return response;
    }
    else if (httpRequest.method == "POST") {

        LocationContext locationContext = server_context.getLocationContext("/upload");

        if (locationContext.isAllowedMethod("POST") == false) {
            StaticFileReader fileReader;
            std::string fileContent = fileReader.readFile("405.html", "GET", server_context);

            std::string response = "HTTP/1.1 405 Method Not Allowed\r\n";
            // response += "Content-Type: text/html\r\n";
            // response += "Content-Length: " + std::to_string(fileContent.size()) + "\r\n";
            response += "\r\n";
            response += fileContent;

            std::cout << "DEBUG MSG: POST FAILED\n";
            return response;
        }

        DataProcessor dataProcessor;
        ProcessResult result = dataProcessor.processPostData(httpRequest.body, locationContext);
        // エラーハンドリングを追加する必要がある

        // レスポンスの生成
        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: Web/json\r\n"; // もしJSONレスポンスであれば
        response += "Content-Length: " + std::to_string(result.message.size()) + "\r\n";
        response += "\r\n";
        response += result.message;

        std::cout << "DEBUG MSG: POST SUCCESS\n";
        std::cout << "DEBUG MSG:: response: " << response << "\n";
        return response;
    }
    else if (httpRequest.method == "DELETE") {
        
        LocationContext locationContext = server_context.getLocationContext("/upload");
        if (locationContext.isAllowedMethod("DELETE") == false) {
            StaticFileReader fileReader;
            std::string fileContent = fileReader.readFile("405.html", "GET", server_context);

            std::string response = "HTTP/1.1 405 Method Not Allowed\r\n";
            // response += "Content-Type: text/html\r\n";
            // response += "Content-Length: " + std::to_string(fileContent.size()) + "\r\n";
            response += "\r\n";
            response += fileContent;

            std::cout << "DEBUG MSG: DELETE FAILED\n";
            return response;
        }
        if (std::remove(("." + httpRequest.url).c_str()) != 0) {
            std::cerr << "ERROR: File not found or delete failed.\n";
            return "HTTP/1.1 404 Not Found\r\n\r\n"; // エラーレスポンス
        }

        std::cout << "DEBUG MSG: DELETE SUCCESS\n";
        return "HTTP/1.1 204 No Content\r\n\r\n"; // 成功のレスポンス
    }
    // 未実装のメソッドの場合
    StaticFileReader fileReader;
    std::string fileContent = fileReader.readFile("501.html", "GET", server_context);

    std::string response = "HTTP/1.1 501 Not Implemented\r\n";
    // response += "Content-Type: text/html\r\n";
    // response += "Content-Length: " + std::to_string(fileContent.size()) + "\r\n";
    response += "\r\n";
    response += fileContent;

    std::cout << "DEBUG MSG: NOT IMPLEMENTED\n";
    return response;
}

CoreHandler::~CoreHandler() {
    // 必要に応じてクリーンアップ処理をここに記述
}
