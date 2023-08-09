#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include "Config.hpp"
#include "HTTPContext.hpp"
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
        LOCATION_CONTEXT
    };

    enum DirectiveType
    {
        HTTP,
        ACCESS_LOG,
        ERROR_LOG,
        SERVER,
        LISTEN,
        SERVER_NAME,
        LOCATION,
        ALIAS,
        INDEX,
        ERROR_PAGE,
        UNKNOWN
    };

    public:
        ConfigParser(Config& config);
        ~ConfigParser();
        void parseFile(const std::string& filepath);
        void getAndSplitLines(std::ifstream& ifs);
        std::vector<std::string> splitLine(const std::string& line);
        void parseLines();
        void setHTTPContext();
        const ServerContext getServerContext();
        const LocationContext getLocationContext();
        void setContextType(ContextType context);
        void setDirectiveType(const std::string& directive);
        bool isAllowedDirective();
        bool isInHTTPContext();
        bool isInServerContext();
        bool isInLocationContext();

    private:
        Config& _config;
        size_t _line_number;
        std::string _filepath;
        std::vector<std::vector<std::string> > _lines;
        std::vector<std::string> _one_line;
        ContextType _context_type;
        DirectiveType _directive_type;
		bool isFile(const char *path);
};

#endif