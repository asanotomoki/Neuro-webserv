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
    verifyPortAndHost();
}

Config::~Config()
{
}

const std::vector<std::string> Config::getPorts()
{
    // _portAndHostVecAllからportのみを取り出す
    // 重複がある場合は無視する
    std::vector<std::string> ports;

    for (size_t i = 0; i < _portAndHostVecAll.size(); ++i) {
        std::string port = _portAndHostVecAll[i].first;
        bool isDuplicate = false;
        for (size_t j = 0; j < ports.size(); ++j) {
            if (port == ports[j]) {
                isDuplicate = true;
                break;
            }
        }
        if (!isDuplicate)
            ports.push_back(port);
    } 
    return ports;
}

void Config::addServerContext(const std::pair<std::string, std::string> portAndHost, const ServerContext& server)
{
    _servers.insert(std::make_pair(portAndHost, server));
}

void Config::addPortAndHostVecAll(std::pair<std::string, std::string> portAndHost)
{
    std::string first = portAndHost.first;
    std::string second = portAndHost.second;
    // firstとsecondが共に一致するpairが既に存在する場合、例外をスロー
    for (size_t i = 0; i < _portAndHostVecAll.size(); ++i) {
        if (_portAndHostVecAll[i].first == first && _portAndHostVecAll[i].second == second) {
            throw ConfigError(DUPLICATE_PORT_AND_HOST, first, second);
        }
    }
    _portAndHostVecAll.push_back(portAndHost);
}

void Config::addDirective(const std::string& directive, const std::string& value,
                                const std::string& filepath, int line_number)
{
    // check if directive is not duplicated
    if (_directives.find(directive) != _directives.end())
        throw ConfigError(DUPLICATE_DIRECTIVE, directive, filepath, line_number);

    _directives.insert(std::make_pair(directive, value));
}

const ServerContext& Config::getServerContext(const std::string& port, const std::string& host) const
{
    // portとhostのペアから、一致するServerContextを返す
    std::pair <std::string, std::string> port_host_pair = std::make_pair(port, host);
    std::map<std::pair<std::string, std::string>, ServerContext >::const_iterator
        it = _servers.find(port_host_pair);
    // 一致するServerContextがない場合は、ポートから一致するServerContextを返す
    if (it == _servers.end()) {
        for (it = _servers.begin(); it != _servers.end(); ++it) {
            if (it->first.first == port)
                return it->second;
        }
    }
    return it->second;
}

void Config::verifyPortAndHost()
{
    
}