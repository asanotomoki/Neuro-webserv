#ifndef STATIC_FILE_READER_HPP
#define STATIC_FILE_READER_HPP

#include "ServerContext.hpp"
#include "LocationContext.hpp"
#include "RequestParser.hpp"
#include <string>
#include <vector>

//記憶を取り出す部分
class StaticFileReader {
public:
    StaticFileReader();
    std::string readFile(std::string filename, LocationContext locationContext,
                        const ServerContext &serverContext);
    std::string readErrorFile(const LocationContext& locationContext, int statusCode);
    ~StaticFileReader();
};


#endif