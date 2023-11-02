#include "StaticFileReader.hpp"
#include "DataProcessor.hpp"
#include "utils.hpp"
#include <fstream>
#include <iterator>
#include <iostream>
#include <dirent.h>

StaticFileReader::StaticFileReader() {
}

// 引数で与えられたファイル（パス）が存在するかどうか判定する関数
bool isFileExist(const std::string& name) {
    std::ifstream file(name);
    return file.is_open();
}

std::string StaticFileReader::readErrorFile(int statusCode, const ServerContext& serverContext) {

    std::string filePath = serverContext.getErrorPage(statusCode);
    if (!isFileExist(filePath)) {
        return default_error_page(statusCode);
    }
    std::ifstream file(filePath, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

bool isDirectory_y(std::string path) {
    DIR* dir = opendir(path.c_str());
    if (dir) {
        closedir(dir);
        return true;
    }
    return false;
}

std::string StaticFileReader::readFile(std::string fullpath, LocationContext locationContext,
                                        const ServerContext& serverContext, const ParseUrlResult& result) {
    
    // ファイルをバイナリモードで読み込み
    std::ifstream file(fullpath, std::ios::binary);
    if (!file || isDirectory_y(fullpath) || result.autoindex == 1) {
        if (locationContext.hasDirective("autoindex")) {
            if (locationContext.getDirective("autoindex") == "on") {
                std::string response = DataProcessor::getAutoIndexHtml(locationContext.getDirective("alias"), serverContext);
                return response;
            }
        }
        return readErrorFile(404, serverContext);
    }
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

StaticFileReader::~StaticFileReader() {
}
