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

    std::cout << "readErrorFile :: filePath: " << filePath << "\n";

    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "ERROR: File not found: " << filePath << "\n"; 
        return filePath + " not found";
    }
    std::cout << "readErrorFile :: success readErrorFile\n";
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

std::string StaticFileReader::readFile(std::string fullpath, LocationContext locationContext,
                                        const ServerContext& serverContext, bool isAutoIndex) {
    
    if (!isAutoIndex)
    {
        locationContext = serverContext.get403LocationContext();
        return readErrorFile(locationContext);
    }


    // ファイルをバイナリモードで読み込み
    std::ifstream file(fullpath, std::ios::binary);
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
