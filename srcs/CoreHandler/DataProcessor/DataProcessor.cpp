#include "DataProcessor.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>

std::string parseDirectoryFromPostData(const std::string& postData) {
    std::string directoryField = "directory=";
    size_t startPos = postData.find(directoryField);

    if (startPos == std::string::npos) {
        // "directory="が見つからない場合、空の文字列を返す
        std::cerr << "directory= not found" << std::endl;
        return "";
    }

    startPos += directoryField.length(); // "directory="の長さを足して、値のスタート位置を見つける
    size_t endPos = postData.find('&', startPos); // 次のパラメータの開始位置を見つける

    // パラメータが最後である場合、endPosはstd::string::nposになる
    if (endPos == std::string::npos) {
        endPos = postData.length();
    }

    // 部分文字列を取得
    std::string directoryValue = postData.substr(startPos, endPos - startPos);
  
    // 必要ならURLデコードを行う
    // ここでは省略

    return directoryValue;
}

ProcessResult DataProcessor::processPostData(const std::string& postData) {
    std::cout << "postData: " << postData << std::endl;
    // ファイルデータの部分を解析
    size_t fileDataStart = postData.find("\r\n") + 1;
    size_t fileDataEnd = postData.find("\r\n", fileDataStart);
    std::string fileData = postData.substr(fileDataStart, fileDataEnd - fileDataStart);

    std::string directory = parseDirectoryFromPostData(postData);
    std::cout << "directory: " << directory << std::endl;

    // 指定されたディレクトリが存在するか確認
    struct stat st;
    std::string fullDirPath = "./docs/" + directory;
    if (stat(fullDirPath.c_str(), &st) == -1) {
        // ディレクトリが存在しない場合、作成する
        mkdir(fullDirPath.c_str(), 0755);
    }
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
        ProcessResult result = ProcessResult("error", "Failed to create file.", 500);
        return result;
    }

    file.write(fileData.c_str(), fileData.size());
    file.close();

    ProcessResult result = ProcessResult("success", "File uploaded successfully.", 200);
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
