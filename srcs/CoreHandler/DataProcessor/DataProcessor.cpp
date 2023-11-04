#include "DataProcessor.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>

ProcessResult DataProcessor::processPostData(const std::string& body, const std::string& url, 
                                                const ServerContext& serverContext)
{

    // ファイルデータの部分を解析
    size_t fileDataStart = body.find("\r\n") + 1;
    size_t fileDataEnd = body.find("\r\n", fileDataStart);
    std::string fileData = body.substr(fileDataStart, fileDataEnd - fileDataStart);

    std::string directoryPath = serverContext.getServerPath(url);
    // 指定されたディレクトリが存在するか確認
    struct stat st;
    std::string fullDirPath = directoryPath;
    if (stat(fullDirPath.c_str(), &st) == -1) {
        // ディレクトリが存在しない場合、４０４エラーを返す
        ProcessResult result = ProcessResult("error", "Not found.", 404, fullDirPath);
        return result;
    }
    std::cout << "fullDirPath: " << fullDirPath << std::endl;
    // インデックスを見つけてファイル名を生成
    int index = 1;
    std::string filePath;
    do {
        filePath = fullDirPath + "/uploaded_file_" + std::to_string(index) + ".txt";
        index++;
    } while (std::ifstream(filePath)); // 既存ファイルがある場合、インデックスを増やす

    // 指定のディレクトリにファイルを保存
    std::ofstream file(filePath, std::ios::binary);
    if (!file) {
        ProcessResult result = ProcessResult("error", "Failed to create file.", 500, filePath);
        return result;
    }

    file.write(fileData.c_str(), fileData.size());
    file.close();

    ProcessResult result = ProcessResult("success", "File uploaded successfully.", 201, filePath);
    return result;
}

bool isDirectory(std::string path) {
    DIR* dir = opendir(path.c_str());
    if (dir) {
        closedir(dir);
        return true;
    }
    return false;
}

bool isFile(std::string path) {
    if (path.find('.') != std::string::npos)
        return true;
    return false;
}

std::string DataProcessor::getAutoIndexHtml(std::string path, const ServerContext& serverContext) {

    std::string html;
    html += "<html><body><h1>Index of " + path + "</h1><ul>";

    DIR* dir = opendir(path.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            std::string name(entry->d_name);
            if (entry->d_type == DT_DIR) {
                name += "/";
            }
            // nameが"."で始まる場合は表示しない
            if (name[0] == '.') {
                continue;
            }
            std::string tempPath = path;
            std::string clientPath;
            if (path == "./docs/") {
                if (isFile(name)) {
                    clientPath = serverContext.getClientPath(path);
                    clientPath += name;
                } else {
                    path += name;
                    clientPath = serverContext.getClientPath(path);
                }
            } else {
                clientPath = serverContext.getClientPath(path);
                clientPath += name;
            }
            html += "<li><a href=\"" + clientPath + "\">" + name + "</a></li>\n";
            if (path != tempPath) {
                // pathからnameを削除
                path = tempPath;
            }

        }
        closedir(dir);
    }
    html += "</ul></body></html>";
    return html;
}
