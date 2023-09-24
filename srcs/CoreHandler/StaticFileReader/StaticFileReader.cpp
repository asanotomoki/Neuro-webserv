#include "StaticFileReader.hpp"
#include <fstream>
#include <iterator>
#include <iostream>

StaticFileReader::StaticFileReader() {
}

std::string StaticFileReader::readErrorFile(const LocationContext& locationContext) {
    std::string alias = locationContext.getDirective("alias");
    std::string filename = locationContext.getDirective("index");
    std::string filePath = alias + filename;

    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "ERROR: File not found: " << filePath << "\n"; 
        return filePath + " not found";
    }
    std::cout << "readErrorFile :: success readErrorFile\n";
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

std::string StaticFileReader::readFile(std::string filename, LocationContext locationContext,
                                        const ServerContext& serverContext) {
    
    std::cout << "readFile :: filename: " << filename << "\n";
    bool autoindexEnabled = true;
    if (locationContext.hasDirective("autoindex"))
        autoindexEnabled = locationContext.getDirective("autoindex") == "on";

    // "alias" ディレクティブの値を取得
    std::string alias = locationContext.getDirective("alias");

    std::cout << "readFile :: alias: " << alias << "\n";

    // ファイル名が空の場合、"index" ディレクティブの値を取得
    try {
        if (filename.empty()) {
            if (autoindexEnabled)
                filename = locationContext.getDirective("index");
            else {
                locationContext = serverContext.get403LocationContext();
                return readErrorFile(locationContext);
            }
        }
    } catch (std::runtime_error& e) {
        filename = "index.html";
    }

    // aliasで指定されたパスと組み合わせて、完全なファイルパスを作成
    std::string filePath = alias + filename;

    std::cout << "readFile :: filePath: " << filePath << "\n";

    // ファイルをバイナリモードで読み込み
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        locationContext = serverContext.get404LocationContext();
        return readErrorFile(locationContext);
    }

    std::cout << "readFile :: success readFile\n";
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

StaticFileReader::~StaticFileReader() {
}
