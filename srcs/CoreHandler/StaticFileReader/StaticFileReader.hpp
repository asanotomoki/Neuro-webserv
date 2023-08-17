#ifndef STATIC_FILE_READER_HPP
#define STATIC_FILE_READER_HPP

#include "ServerContext.hpp"
#include <string>
#include <vector>

//記憶を取り出す部分
class StaticFileReader {
public:
    StaticFileReader();
    std::string readFile(const std::string& path, const ServerContext& server_context);
    ~StaticFileReader();
};


#endif