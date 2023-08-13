#include "StaticFileReader.hpp"
#include <fstream>
#include <iterator>
#include <iostream>

StaticFileReader::StaticFileReader() {
}

std::string StaticFileReader::readFile(const std::string& requestPath, const ServerContext& serverContext) {
    if (requestPath == "/favicon.ico") {
        std::cerr << "DEBUG MESSAGE: favicon.ico request, ignoring\n";
        return {};
    }
    
    // リクエストパスから適切なLocationContextを取得
    const LocationContext& locationContext = serverContext.getLocationContext(requestPath);

    // "alias" ディレクティブの値を取得
    std::string alias = locationContext.getDirective("alias");

    std::cout << "DEBUG MESSAGE: alias: " << alias << "\n";

    // リクエストパスから、aliasディレクティブで指定された部分を取り除く
    // std::string modifiedRequestPath = requestPath.substr(requestPath.find_first_of('/', 1));

    std::cout << "DEBUG MESSAGE: requestPath: " << requestPath << "\n";

    std::string modifiedRequestPath;
    if (requestPath == "/") {
        modifiedRequestPath = "/index.html"; // 既定のインデックスファイルへのパス
    } else {
        size_t pos = requestPath.find_first_of('/', 1);
        modifiedRequestPath = requestPath.substr(pos);
    }
    std::cout << "DEBUG MESSAGE: modifiedRequestPath: " << modifiedRequestPath << "\n";

    // aliasで指定されたパスと組み合わせて、完全なファイルパスを作成
    std::string filePath = alias + modifiedRequestPath;

    std::cout << "DEBUG MESSAGE: filePath: " << filePath << "\n";

    // ファイルをバイナリモードで読み込み
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        // エラー処理
        return {};
    }
    std::cout << "DEBUG MESSAGE: success readFile\n";
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

StaticFileReader::~StaticFileReader() {
}
