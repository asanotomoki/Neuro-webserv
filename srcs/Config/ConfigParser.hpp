#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include "Config.hpp"
#include "LocationContext.hpp"
#include "ServerContext.hpp"
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>

class ConfigParser
{
    enum ContextType
    {
        HTTP_CONTEXT,
        SERVER_CONTEXT,
        LOCATION_CONTEXT,
        CGI_CONTEXT
    };

    enum DirectiveType
    {
        SERVER,
        LISTEN,
        SERVER_NAME,
        MAX_BODY_SIZE,
        ERROR_PAGE,
        LOCATION,
        CGI,
        ALIAS,
        INDEX,
        LIMIT_EXCEPT,
        COMMAND,
        CGI_ON,
        RETURN,
        UNKNOWN
    };

    public:
        ConfigParser(Config& config);
        ~ConfigParser();
        void parseFile(const std::string& filepath);
        void getAndSplitLines(std::ifstream& ifs);
        std::vector<std::string> splitLine(const std::string& line);
        void parseLines();
        const ServerContext setServerContext();
        const LocationContext setLocationContext();
        const CGIContext setCGIContext();
        void setContextType(ContextType context);
        void setDirectiveType(const std::string& directive);
        bool isAllowedDirective();
        bool isInHttpContext();
        bool isInServerContext();
        bool isInLocationContext();
        bool isInCgiContext();

    private:
        Config& _config;
        size_t _lineNumber;
        std::string _filepath;
        std::vector<std::vector<std::string> > _lines;
        std::vector<std::string> _oneLine;
        ContextType _contextType;
        DirectiveType _directiveType;
		bool isFile(const char* path);
};

#endif