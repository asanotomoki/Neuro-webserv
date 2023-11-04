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
        void addServerContext(const ServerContext& server);
		void addDirective(const std::string& directive, const std::string& value,
							const std::string& filepath, int line_number);
		const std::map<std::string, std::vector<ServerContext> >& getServers() const;
		const ServerContext& getServerContext(const std::string& port, const std::string& host) const;
		void verifyConfig();

    private:
    	std::map<std::string, std::vector<ServerContext> > _servers;
		std::map<std::string, std::string> _directives;      
};

#endif