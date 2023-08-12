#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "HttpContext.hpp"
#include <string>

//遺伝子クラス
class Config
{
    public:
        Config(const std::string& filepath);
		HttpContext& getHttpContext();
		static Config* getInstance();
		const std::vector<std::string> getPorts();
		~Config();

    private:
        HttpContext _http_context;
        static Config* _instance;
		int	redirectErrorLogFile(std::string errorLogFile);
        int redirectAccessLogFile(std::string accessLogFile);
        static const int NEW_FD;
};

#endif