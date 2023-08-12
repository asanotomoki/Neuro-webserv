#include "StaticFileReader.hpp"
#include <fstream>
#include <iterator>

StaticFileReader::StaticFileReader() {
}

std::string StaticFileReader::readFile(const std::string& requestPath, Config* config) {
    // リクエストパスから、aliasディレクティブで指定された部分を取り除く
    std::string modifiedRequestPath = requestPath.substr(requestPath.find_first_of('/', 1));

    // aliasで指定されたパスと組み合わせて、完全なファイルパスを作成
    std::string filePath = config->getHttpBlock()->getServerContext() + modifiedRequestPath;

    // ファイルをバイナリモードで読み込み
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        // エラー処理
        return {};
    }

    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}


StaticFileReader::~StaticFileReader() {
}
