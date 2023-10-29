#include "Config.hpp"
#include "ConfigParser.hpp"
#include "ConfigError.hpp"
#include "ServerContext.hpp"
#include "LocationContext.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

Config::Config(const std::string& filepath)
{
    ConfigParser parser(*this);
    parser.parseFile(filepath);
    verifyConfig();
}

Config::~Config()
{
}

const std::vector<std::string> Config::getPorts()
{
	std::vector<std::string> ports;

	for (std::map<std::string, std::vector<ServerContext> >::const_iterator it = getServers().begin();
			it != getServers().end(); ++it) {
		for (std::vector<ServerContext>::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
			ports.push_back(it2->getListen());
	}
	return ports;
}

void Config::addServerContext(const ServerContext& server)
{
    std::string listen = server.getListen();
    std::map<std::string, std::vector<ServerContext> >::iterator
        port_found = _servers.find(listen);

    if (port_found != _servers.end()) {
        if (server.getServerName().empty()) {
            //exception
        }
        std::vector<ServerContext> &servers = port_found->second;
        for (size_t i = 0; i < servers.size(); i++) {
            if (servers.at(i).getServerName() == server.getServerName()) {
                //exception
            }
        }
        servers.push_back(server);
    }
    else {
        std::vector<ServerContext> new_servers(1, server);
        _servers.insert(std::make_pair(listen, new_servers));
    }
}

void Config::addDirective(const std::string& directive, const std::string& value,
                                const std::string& filepath, int line_number)
{
    // check if directive is not duplicated
    if (_directives.find(directive) != _directives.end())
        throw ConfigError(DUPLICATE_DIRECTIVE, directive, filepath, line_number);

    _directives.insert(std::make_pair(directive, value));
}

const std::map<std::string, std::vector<ServerContext> >& Config::getServers() const
{
    return _servers;
}

const ServerContext& Config::getServerContext(const std::string& port, const std::string& host) const
{
    // ポート番号が一致するServerブロックをすべて取得する
    const std::map<std::string, std::vector<ServerContext> >& servers = getServers();
    const std::vector<ServerContext>* serverContextsPtr;
    if (servers.find(port) == servers.end()) {
        std::cerr<<  "getServerContext :: port not found! -> " << port <<  std::endl;
        serverContextsPtr = &servers.begin()->second;
    } else {
        serverContextsPtr = &servers.at(port);
    }
    const std::vector<ServerContext>& serverContexts = *serverContextsPtr;
    // server_nameがhostヘッダーと一致する場合、そのserverブロックを返す
    for (std::vector<ServerContext>::const_iterator it = serverContexts.begin(); it != serverContexts.end(); ++it) {
        if (it->getServerName() == host) {
            return *it;
        }
    }
    return serverContexts.at(0);
}

void Config::verifyConfig()
{
    // 各サーバーのポートを確認
    // ポートが被っていないか確認
    // ポートが被っている場合に、サーバーネームが異なっているか確認
    // サーバーネームが同じorサーバーネームがない場合は例外を投げる
    for (std::map<std::string, std::vector<ServerContext> >::const_iterator it = getServers().begin();
            it != getServers().end(); ++it) {
        std::vector<ServerContext> servers = it->second;
        for (size_t i = 0; i < servers.size(); i++) {
            for (size_t j = i + 1; j < servers.size(); j++) {
                if (servers.at(i).getServerName() == servers.at(j).getServerName()) {
                    throw ConfigError(DUPLICATE_PORT, servers.at(i).getServerName());
                }
            }
        }
    }
}
