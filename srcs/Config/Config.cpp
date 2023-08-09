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
    _instance = this;

	std::string errorLogFile = getHTTPBlock().getErrorLogFile();
	redirectErrorLogFile(errorLogFile);

	std::string accessLogFile = getHTTPBlock().getAccessLogFile();
	redirectAccessLogFile(accessLogFile);
}

int	Config::redirectErrorLogFile(std::string errorLogFile)
{
	if (errorLogFile.empty())
		return (0);
	else
	{
		int logFile;
		logFile = open(errorLogFile.c_str(), O_RDWR | O_CREAT | O_APPEND, 0666);
		if (logFile > 0)
		{
			if (dup2(logFile, STDERR_FILENO) == -1)
				throw (ConfigError(SYSTEM_ERROR, "dup2"));
		}
		else
		{
			std::cerr << "DEBUG MESSAGE 1: open error" << std::endl;
			throw (ConfigError(SYSTEM_ERROR, "open"));
		}
	}
	return (0);
}

int Config::redirectAccessLogFile(std::string  accessLogFile)
{
	if (accessLogFile.empty())
		return (0);
	else
	{
		int logFile;
		logFile = open(accessLogFile.c_str(), O_RDWR | O_CREAT | O_APPEND, 0666);
		if (logFile > 0)
		{
			if (dup2(logFile, NEW_FD) == -1)
				throw (ConfigError(SYSTEM_ERROR, "dup2"));
		}
		else
		{	
			std::cerr << "DEBUG MESSAGE 2: open error" << std::endl;
			throw (ConfigError(SYSTEM_ERROR, "open"));
		}
	}
	return (0);
}

Config::~Config()
{
}

HTTPContext& Config::getHTTPBlock()
{
    return _http_block;
}

Config* Config::getInstance()
{
    return _instance;
}

const std::vector<std::string> Config::getPorts()
{
	std::vector<std::string> ports;

	
	for (std::map<std::string, std::vector<ServerContext> >::const_iterator it = _http_block.getServers().begin();
			it != _http_block.getServers().end(); ++it)
	{
		for (std::vector<ServerContext>::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
		{
			//test
			//std::cout << it2->getListen() << std::endl;
			ports.push_back(it2->getListen());
		}
	}
	return ports;
}

Config* Config::_instance = NULL;
const int Config::NEW_FD = 3;
