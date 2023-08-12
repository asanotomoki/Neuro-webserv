#include "Config.hpp"
#include "ApplicationServer.hpp"
#include "./RequestParser/RequestParser.hpp"
#include "./StaticFileReader/StaticFileReader.hpp"

ApplicationServer::ApplicationServer() {
    // 必要に応じて初期化処理をここに記述
}

std::string ApplicationServer::processRequest(const std::string& request, Config* config) {
    // リクエストを解析
    RequestParser parser;
    HttpRequest httpRequest = parser.parse(request);

    if (httpRequest.method == "GET") {
        // 静的ファイルを提供する場合
        StaticFileReader fileReader;
        std::string filePath = httpRequest.url;
        std::string response = fileReader.readFile(filePath, config);

        return response; // ファイルの内容を応答として返す
    }

    // 他のHTTPメソッドの処理...

    return "HTTP/1.1 501 Not Implemented\r\n\r\n"; // 未実装のメソッド
}

ApplicationServer::~ApplicationServer() {
    // 必要に応じてクリーンアップ処理をここに記述
}
