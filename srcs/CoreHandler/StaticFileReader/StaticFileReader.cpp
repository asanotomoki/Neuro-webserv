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
        std::cout << "WARNING : favicon.ico request, ignoring\n";
        return {};
    }

    std::string directory, filename;
    // requestPathをディレクトリとファイルに分割
    size_t lastSlash = requestPath.find_last_of("/");
    if (lastSlash != std::string::npos) {
        directory = requestPath.substr(0, lastSlash + 1);
        filename = requestPath.substr(lastSlash + 1);
    } else {
        directory = "/";
        filename = requestPath;
    }
    std::cout << "DEBUG MSG 0 :: directory: " << directory << ", filename: " << filename << "\n";

    // リクエストパスから適切なLocationContextを取得
    LocationContext locationContext;
    if (requestPath == "404.html")
        locationContext = serverContext.get404LocationContext();
    else if (requestPath == "405.html")
        locationContext = serverContext.get405LocationContext();
    else if (requestPath == "501.html")
        locationContext = serverContext.get501LocationContext();
    else {
        locationContext = serverContext.getLocationContext(directory);
        if (!locationContext.isAllowedMethod(method))
            locationContext = serverContext.get405LocationContext();
    }

    // "alias" ディレクティブの値を取得
    std::string alias = locationContext.getDirective("alias");

    std::cout << "DEBUG MSG 1 :: alias: " << alias << "\n";

    // ファイル名に"."が含まれていない場合は、indexディレクティブの値を使用
    if (filename.find(".") == std::string::npos) {
        filename = locationContext.getDirective("index");
    }

    // aliasで指定されたパスと組み合わせて、完全なファイルパスを作成
    std::string filePath = alias + filename;

    std::cout << "DEBUG MSG 2 :: filePath: " << filePath << "\n";

    // ファイルをバイナリモードで読み込み
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "ERROR: File not found: " << filePath << "\n"; // エラーメッセージをログに出力
        locationContext = serverContext.get404LocationContext();
        alias = locationContext.getDirective("alias");
        filename = locationContext.getDirective("index");
        filePath = alias + filename;
        return readErrorFile(filePath);
    }

    std::cout << "DEBUG MSG 3 :: success readFile\n";
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

StaticFileReader::~StaticFileReader() {
}
