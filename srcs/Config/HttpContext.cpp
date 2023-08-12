#include "HttpContext.hpp"
#include "ConfigError.hpp"
#include <stdexcept>

HttpContext::HttpContext():
	_accessLogFile(""),
	_errorLogFile("")
{
}

HttpContext::~HttpContext()
{
}

void HttpContext::setAccessLogFile(const std::string& accessLogFile)
{
	_accessLogFile = accessLogFile;
}

void HttpContext::setErrorLogFile(const std::string& errorLogFile)
{
	_errorLogFile = errorLogFile;
}

void HttpContext::addServerBlock(const ServerContext& server)
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

void HttpContext::addDirective(const std::string& directive, const std::string& value,
                                const std::string& filepath, int line_number)
{
    // check if directive is not duplicated
    if (_directives.find(directive) != _directives.end())
        throw ConfigError(DUPRICATE_DIRECTIVE, directive, filepath, line_number);

    _directives.insert(std::make_pair(directive, value));
}

const std::string& HttpContext::getAccessLogFile() const
{
	return _accessLogFile;
}

const std::string& HttpContext::getErrorLogFile() const
{
	return _errorLogFile;
}

const std::map<std::string, std::vector<ServerContext> >& HttpContext::getServers() const
{
    return _servers;
}

/**
 * @brief IPアドレスとポート番号が一致するserverブロックを取得する
 *
 * @detail 記述順で最初に一致したブロックを返す（server_nameが一致する場合はそれを返す）
 * @param port
 * @return const std::vector<ServerContext>&
 */
const ServerContext& HttpContext::getServerContext(const std::string& port, const std::string& host) const
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
