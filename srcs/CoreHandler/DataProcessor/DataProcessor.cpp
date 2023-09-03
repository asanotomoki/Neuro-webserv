#include "DataProcessor.hpp"
#include <fstream>

ProcessResult DataProcessor::processPostData(const std::string& postData, const LocationContext& locationContext) {

    // ファイルデータの部分を解析
    size_t fileDataStart = postData.find("\r\n") + 1;
    size_t fileDataEnd = postData.find("\r\n", fileDataStart);
    std::string fileData = postData.substr(fileDataStart, fileDataEnd - fileDataStart);

    // インデックスを見つけてファイル名を生成
    int index = 1;
    std::string filePath;
    do {
        filePath = "./docs/upload/uploaded_file_" + std::to_string(index) + ".txt";
        index++;
    } while (std::ifstream(filePath)); // 既存ファイルがある場合、インデックスを増やす

    // 指定のディレクトリにファイルを保存
    std::ofstream file(filePath, std::ios::binary);
    if (!file) {
        return { "error", "Failed to create file.", 500 };
    }

    file.write(fileData.c_str(), fileData.size());
    file.close();

    return { "success", "File uploaded successfully.", 200 };
}
