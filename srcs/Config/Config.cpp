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
}

Config::~Config()
{
}

const std::vector<std::string> Config::getPorts()
{
	std::vector<std::string> ports;

	for (std::map<std::string, std::vector<ServerContext> >::const_iterator it = getServers().begin();
			it != getServers().end(); ++it)
	{
		for (std::vector<ServerContext>::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
		{
			ports.push_back(it2->getListen());
		}
	}
	return ports;
}

void Config::addServerContext(const ServerContext& server)
{
    std::string listen = server.getListen();
    std::map<std::string, std::vector<ServerContext> >::iterator
        port_found = _servers.find(listen);

    if (port_found != _servers.end())
    {
        if (server.getServerName().empty())
        {
            //exception

        }
        std::vector<ServerContext> &servers = port_found->second;
        for (size_t i = 0; i < servers.size(); i++)
        {
            if (servers.at(i).getServerName() == server.getServerName())
            {
                //exception
            }
        }
        servers.push_back(server);
    }
    else
    {
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
    try
    {
        // ポート番号が一致するServerブロックをすべて取得する
        const std::vector<ServerContext>& serverContexts = getServers().at(port);

        // server_nameがhostヘッダーと一致する場合、そのserverブロックを返す
        for (std::vector<ServerContext>::const_iterator it = serverContexts.begin(); it != serverContexts.end(); ++it)
        {
            if (it->getServerName() == host)
                return *it;
        }
        // server_nameがhostヘッダーと一致しない場合、最初のserverブロックを返す
        return serverContexts.at(0);
    }
    catch (std::out_of_range& e)
    {
        // 一致するポート番号がない場合は上位に投げる
        throw std::runtime_error("port not found!");
    }
}

