#include "ApplicationServer.hpp"

ApplicationServer::ApplicationServer() {
    // 必要に応じて初期化処理をここに記述
}

std::string ApplicationServer::processRequest(const std::string& request) {
    // この例では単純にリクエストをそのままレスポンスとして返します。
    // 実際にはここでリクエストを解析し、必要な処理を行い、適切なレスポンスを生成します。

    return "HTTP/1.1 200 server_is_running!\r\nContent-Length: 18\r\n\r\nserver_is_running!";
}

ApplicationServer::~ApplicationServer() {
    // 必要に応じてクリーンアップ処理をここに記述
}
