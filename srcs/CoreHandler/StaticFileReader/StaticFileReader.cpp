#include "StaticFileReader.hpp"
#include <fstream>
#include <iterator>
#include <iostream>

StaticFileReader::StaticFileReader() {
}

std::string StaticFileReader::readErrorFile(const int status, const ServerContext& serverContext) {
    LocationContext locationContext;
    if (status == 404)
        locationContext = serverContext.get404LocationContext();
    else if (status == 405)
        locationContext = serverContext.get405LocationContext();
    else if (status == 501)
        locationContext = serverContext.get501LocationContext();
    std::string alias = locationContext.getDirective("alias");
    std::string filename = locationContext.getDirective("index");
    std::string filePath = alias + filename;

    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "ERROR: File not found: " << filePath << "\n"; 
        return filePath + " not found";
    }
    std::cout << "DEBUG MSG 3 :: success readFile\n";
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

std::string StaticFileReader::readFile(std::string& requestPath, const std::string& method,
                                       const ServerContext& serverContext) {
    if (requestPath == "/favicon.ico") {
        std::cout << "WARNING : favicon.ico request, ignoring\n";
        return {};
    }

    // requestPathが"/"で終わっていない場合に、"/"を追加
    if (requestPath[requestPath.size() - 1] != '/') {
        requestPath += '/';
    }
    std::string directory, filename;
    size_t len = requestPath.length();
    // requestPathをディレクトリとファイルに分割
    
    // size_t lastSlash = requestPath.find_last_of("/", len - 2); // 末尾の"/"を無視しない

    // 先頭から末尾の'/'までをディレクトリとする
    // directory = requestPath.substr(0, lastSlash + 1);

    // 先頭から次の'/'までをディレクトリとする
    size_t nextSlash = requestPath.find_first_of("/", 1);
    directory = requestPath.substr(0, nextSlash + 1);

    // lastSlash以降、末尾の'/'の手前までをファイル名とする
    // filename = requestPath.substr(lastSlash + 1, len - lastSlash - 2); // 末尾の"/"を除く

    // nextSlash以降、末尾の'/'の手前までをファイル名とする
    filename = requestPath.substr(nextSlash + 1, len - nextSlash - 2); // 末尾の"/"を除く

    // ディレクトリが"/"の場合、ディレクトリ名としてfilenameを設定し、filenameを空にする
    if (directory == "/") {
        directory += filename;
        filename = "";
    }

    // filenameが空でない場合、directoryの末尾に'/'を追加
    if (filename.empty() && directory.back() != '/') {
        directory += "/";
    }

    std::cout << "DEBUG MSG 0 :: directory: " << directory << ", filename: " << filename << "\n";

    // リクエストパスから適切なLocationContextを取得
    LocationContext locationContext;
    locationContext = serverContext.getLocationContext(directory);
    if (!locationContext.isAllowedMethod(method))
        locationContext = serverContext.get405LocationContext();

    // "alias" ディレクティブの値を取得
    std::string alias = locationContext.getDirective("alias");

    std::cout << "DEBUG MSG 1 :: alias: " << alias << "\n";

    // ファイル名が空の場合、"index" ディレクティブの値を取得
    if (filename.empty()) {
        filename = locationContext.getDirective("index");
    }

    // aliasで指定されたパスと組み合わせて、完全なファイルパスを作成
    std::string filePath = alias + filename;

    std::cout << "DEBUG MSG 2 :: filePath: " << filePath << "\n";

    // ファイルをバイナリモードで読み込み
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return readErrorFile(404, serverContext);
    }

    std::cout << "DEBUG MSG 3 :: success readFile\n";
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

StaticFileReader::~StaticFileReader() {
}
