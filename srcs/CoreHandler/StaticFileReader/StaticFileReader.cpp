#include "StaticFileReader.hpp"
#include <fstream>
#include <iterator>
#include <iostream>

StaticFileReader::StaticFileReader() {
}

std::string StaticFileReader::readErrorFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return "404.html not found"; // ここでは空文字列を返していますが、エラーを示す特別な値や例外を返すことも考えられます。
    }
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

std::string StaticFileReader::readFile(const std::string& requestPath, const std::string& method,
                                       const ServerContext& serverContext) {
    if (requestPath == "/favicon.ico") {
        std::cout << "DEBUG MSG: favicon.ico request, ignoring\n";
        return {};
    }

    // リクエストパスから適切なLocationContextを取得
    LocationContext locationContext;
    if (requestPath == "404.html")
        locationContext = serverContext.get404LocationContext();
    else if (requestPath == "405.html")
        locationContext = serverContext.get405LocationContext();
    else if (requestPath == "501.html")
        locationContext = serverContext.get501LocationContext();
    else {
        locationContext = serverContext.getLocationContext(requestPath);
        if (!locationContext.isAllowedMethod(method))
            locationContext = serverContext.get405LocationContext();
    }

    // "alias" ディレクティブの値を取得
    std::string alias = locationContext.getDirective("alias");

    std::cout << "DEBUG MSG:: alias: " << alias << "\n";

    std::string filename = locationContext.getDirective("name");

    std::cout << "DEBUG MSG:: filename: " << filename << "\n";

    // aliasで指定されたパスと組み合わせて、完全なファイルパスを作成
    std::string filePath = alias + filename;

    std::cout << "DEBUG MSG:: filePath: " << filePath << "\n";

    // ファイルをバイナリモードで読み込み
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "ERROR: File not found: " << filePath << "\n"; // エラーメッセージをログに出力
        locationContext = serverContext.get404LocationContext();
        alias = locationContext.getDirective("alias");
        filename = locationContext.getDirective("name");
        filePath = alias + filename;
        return readErrorFile(filePath);
    }

    std::cout << "DEBUG MSG: success readFile\n";
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

StaticFileReader::~StaticFileReader() {
}
