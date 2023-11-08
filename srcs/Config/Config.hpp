#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "ServerContext.hpp"
#include <string>
#include <vector>
#include <map>

//遺伝子クラス
class Config
{
    public:
        Config(const std::string& filepath);
		const std::vector<std::string> getPorts();
		~Config();
        void addServerContext(std::pair<std::string, std::string> portAndHost, const ServerContext& server);
		void addPortAndHostVecAll(std::pair<std::string, std::string> portAndHost);
		void addDirective(const std::string& directive, const std::string& value,
							const std::string& filepath, int line_number);
		// const std::map<std::string, std::vector<ServerContext> >& getServers() const;
		const ServerContext& getServerContext(const std::string& port, const std::string& host) const;
		void verifyPortAndHost();

    private:
    	std::map<std::pair<std::string, std::string>, ServerContext > _servers;
		// portの取得とエラーハンドリング用
		std::vector<std::pair<std::string, std::string> > _portAndHostVecAll;
		std::map<std::string, std::string> _directives;      
};

#endif