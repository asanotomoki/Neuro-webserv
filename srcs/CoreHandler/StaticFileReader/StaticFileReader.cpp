#include "StaticFileReader.hpp"
#include <fstream>
#include <iterator>
#include <iostream>

StaticFileReader::StaticFileReader() {
}

// 引数で与えられたファイル（パス）が存在するかどうか判定する関数
bool isFileExist(const std::string& name) {
    std::ifstream file(name);
    return file.is_open();
}

std::string StaticFileReader::readErrorFile(const LocationContext& locationContext, int statusCode) {

    std::string alias = locationContext.getDirective("alias");
    std::string filename = locationContext.getDirective("index");
    std::string filePath = alias + filename;

    if (!isFileExist(filePath)) {
        if (statusCode == 403)
            filePath = "./docs/error_page/default/403.html";
        else if (statusCode == 404)
            filePath = "./docs/error_page/default/404.html";
        else if (statusCode == 405)
            filePath = "./docs/error_page/default/405.html";
        else if (statusCode == 500)
            filePath = "./docs/error_page/default/500.html";
        else if (statusCode == 501)
            filePath = "./docs/error_page/default/501.html";
        else
            filePath = "./docs/error_page/default/404.html";
    }
    std::ifstream file(filePath, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

std::string StaticFileReader::readFile(std::string fullpath, LocationContext locationContext,
                                        const ServerContext& serverContext, bool isAutoIndex) {
    
    if (!isAutoIndex) {
        locationContext = serverContext.get403LocationContext();
        return readErrorFile(locationContext, 403);
    }

    // ファイルをバイナリモードで読み込み
    std::ifstream file(fullpath, std::ios::binary);
    if (!file) {
        locationContext = serverContext.get404LocationContext();
        return readErrorFile(locationContext, 404);
    }
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

StaticFileReader::~StaticFileReader() {
}
