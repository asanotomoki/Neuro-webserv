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
    std::cout << "readErrorFile :: success readFile\n";
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

std::string StaticFileReader::readFile(std::string filename, LocationContext locationContext,
                                        const ServerContext& serverContext) {
    // // requestPathが"/"で終わっていない場合に、"/"を追加
    // if (requestPath[requestPath.size() - 1] != '/') {
    //     requestPath += '/';
    // }
    // std::string directory, filename;
    // // requestPathをディレクトリとファイルに分割
    // // 先頭から次の'/'までをディレクトリとする
    // size_t nextSlash = requestPath.find_first_of("/", 1);
    // directory = requestPath.substr(0, nextSlash + 1);
    // // nextSlash以降、末尾の'/'の手前までをファイル名とする
    // filename = requestPath.substr(nextSlash + 1, requestPath.length() - nextSlash - 2); // 末尾の"/"を除く
    // // ディレクトリが"/"の場合、ディレクトリ名としてfilenameを設定し、filenameを空にする
    // if (directory == "/") {
    //     directory += filename;
    //     filename = "";
    // }
    // // filenameが空でない場合、directoryの末尾に'/'を追加
    // if (filename.empty() && directory.back() != '/') {
    //     directory += "/";
    // }
    // std::cout << "DEBUG MSG 0 :: directory: " << directory << ", filename: " << filename << "\n";
    // リクエストパスから適切なLocationContextを取得
    // LocationContext locationContext;
    // locationContext = serverContext.getLocationContext(directory);

    // "alias" ディレクティブの値を取得
    std::string alias = locationContext.getDirective("alias");

    std::cout << "readFile :: alias: " << alias << "\n";

    // ファイル名が空の場合、"index" ディレクティブの値を取得
    if (filename.empty()) {
        filename = locationContext.getDirective("index");
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
