#include "CoreHandler.hpp"
#include "RequestParser.hpp"
#include "StaticFileReader.hpp"
#include "DataProcessor.hpp"
#include <iostream>

CoreHandler::CoreHandler() {
    // 必要に応じて初期化処理をここに記述
}

std::string CoreHandler::processRequest(const std::string& request, const ServerContext& server_context) {
    // リクエストを解析
    RequestParser parser;
    HttpRequest httpRequest = parser.parse(request, server_context);

    if (httpRequest.method == "GET") {
        std::cout << "DEBUG MSG: GET" << std::endl;
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

        std::cout << "DEBUG MSG: GET SUCCESS\n";
        return response; // レスポンスを返す
    }
    // POSTメソッドの処理
    else if (httpRequest.method == "POST") {
        std::cout << "DEBUG MSG: POST" << std::endl;
        
        if (httpRequest.body.empty()) {
            std::cerr << "ERROR: Empty body.\n";
            return "HTTP/1.1 400 Bad Request\r\n\r\n"; // クライアントにエラーレスポンスを返す
        }
        
        // データ処理クラスを使用してPOSTデータを処理
        DataProcessor dataProcessor;
        std::string result = dataProcessor.processPostData(httpRequest.body, server_context);

        // レスポンスの生成
        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: Web/json\r\n"; // もしJSONレスポンスであれば
        response += "Content-Length: " + std::to_string(result.size()) + "\r\n";
        response += "\r\n";
        response += result;

        std::cout << "DEBUG MSG: POST SUCCESS\n";
        std::cout << "DEBUG MSG:: response: " << response << "\n";
        return response; // レスポンスを返す
    }
    else if (httpRequest.method == "DELETE") {
        std::cout << "DEBUG MSG: DELETE" << std::endl;

        // 削除するファイルのパスを特定
        std::string filePath = httpRequest.url;
        std::cout << "DEBUG MSG: filePath: " << filePath << "\n";

        // ファイルを削除
        if (std::remove(("." + filePath).c_str()) != 0) {
            std::perror("ERROR: File delete failed");
            std::cerr << "ERROR: File not found or delete failed.\n";
            return "HTTP/1.1 404 Not Found\r\n\r\n"; // エラーレスポンス
        }

        std::cout << "DEBUG MSG: DELETE SUCCESS\n";
        return "HTTP/1.1 204 No Content\r\n\r\n"; // 成功のレスポンス
    }
    // 未実装のメソッドの場合
    return "HTTP/1.1 501 Not Implemented\r\n\r\n"; // 未実装のメソッド
}

CoreHandler::~CoreHandler() {
    // 必要に応じてクリーンアップ処理をここに記述
}
