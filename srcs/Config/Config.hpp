#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "HTTPContext.hpp"
#include <string>

class Config
{
    public:
        Config(const std::string& filepath);
		HTTPContext& getHTTPBlock();
		static Config* getInstance();
		const std::vector<std::string> getPorts();
		~Config();

    private:
        HTTPContext _http_block;
        static Config* _instance;
		int	redirectErrorLogFile(std::string errorLogFile);
        int redirectAccessLogFile(std::string accessLogFile);
        static const int NEW_FD;
};

#endif