#ifndef STATIC_FILE_READER_HPP
#define STATIC_FILE_READER_HPP

#include "ServerContext.hpp"
#include "RequestParser.hpp"
#include <string>
#include <vector>

//記憶を取り出す部分
class StaticFileReader {
public:
    StaticFileReader();
    std::string readFile(const std::string& requestPath, const std::string& method,
                         const ServerContext& serverContext);
    std::string readErrorFile(const std::string& filepath);
    ~StaticFileReader();
};


#endif