#include "ApplicationServer.hpp"
#include "./RequestParser/RequestParser.hpp"
#include "./StaticFileReader/StaticFileReader.hpp"
#include <iostream>

ApplicationServer::ApplicationServer() {
    // 必要に応じて初期化処理をここに記述
}

std::string ApplicationServer::processRequest(const std::string& request, const ServerContext& server_context) {
    // リクエストを解析
    RequestParser parser;
    HttpRequest httpRequest = parser.parse(request);

    if (httpRequest.method == "GET") {
        std::cout << "DEBUG MESSAGE: GET" << std::endl;
        // 静的ファイルを提供する場合
        StaticFileReader fileReader;
        std::string filePath = httpRequest.url;
        std::string fileContent = fileReader.readFile(filePath, server_context);

        if (fileContent.empty()) {
            // ファイルが見つからなかった場合
            return "HTTP/1.1 404 Not Found\r\n\r\n";
        }

        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n"; // もしHTMLファイルであれば
        response += "Content-Length: " + std::to_string(fileContent.size()) + "\r\n";
        response += "\r\n";
        response += fileContent;

        std::cout << "DEBUG MESSAGE: GET SUCCESS\n";
        return response; // レスポンスを返す
    }
    // 他のHTTPメソッドの処理...

    return "HTTP/1.1 501 Not Implemented\r\n\r\n"; // 未実装のメソッド
}

ApplicationServer::~ApplicationServer() {
    // 必要に応じてクリーンアップ処理をここに記述
}
