#include "ServerContext.hpp"
#include "ConfigError.hpp"
#include <stdexcept>
#include <iostream>

ServerContext::ServerContext():
	_listen(),
	_serverName(),
	_maxBodySize(),
	_errorPages(),
	_locations(),
	_directives(),
	_is_Cgi(false)
{
}

ServerContext::~ServerContext()
{
}

void ServerContext::setListen(const std::string& listen)
{
	_listen = listen;
}

void ServerContext::setServerName(const std::string& server_name)
{
	_serverName = server_name;
}

void ServerContext::setMaxBodySize(const std::string& max_body_size)
{
	_maxBodySize = max_body_size;
}

void ServerContext::setIsCgi(bool is_cgi)
{
	_is_Cgi = is_cgi;
}

void ServerContext::setErrorPages()
{
	_404LocationContext.addDirective("alias", "./docs/error_page/");
	_404LocationContext.addDirective("index", getErrorPage("404"));
	_405LocationContext.addDirective("alias", "./docs/error_page/");
	_405LocationContext.addDirective("index", getErrorPage("405"));
	_501LocationContext.addDirective("alias", "./docs/error_page/");
	_501LocationContext.addDirective("index", getErrorPage("501"));	
}

void ServerContext::setErrorPage(std::string status_code, const std::string& filename)
{
	_errorPages.insert(std::make_pair(status_code, filename));
}

const std::string& ServerContext::getListen() const
{
	return _listen;
}

const std::string& ServerContext::getServerName() const
{
	return _serverName;
}

const std::string& ServerContext::getMaxBodySize() const
{
	return _maxBodySize;
}

bool ServerContext::getIsCgi() const
{
	return _is_Cgi;
}

const std::string& ServerContext::getErrorPage(std::string status_code) const
{
	std::map<std::string, std::string>::const_iterator it = _errorPages.find(status_code);
	// if (it == _errorPages.end())
	// 	return _404LocationContext.getDirective("alias") + _404LocationContext.getDirective("index");
	return it->second;
}

void ServerContext::addLocationContext(const LocationContext& location)
{
	_locations.push_back(location);
}

void ServerContext::addCGIContext(const CGIContext& cgi)
{
	_cgi = cgi;
	setIsCgi(true);
}

void ServerContext::addDirectives(const std::string& directive, const std::string& value,
	const std::string& filepath, int line_number)
{
	// check if directive is not duplicated
	if (_directives.find(directive) != _directives.end()) {
		if (directive == "error_page") {
			_directives.insert(std::make_pair(directive, value));
			return ;
		}
		throw ConfigError(DUPLICATE_DIRECTIVE, directive, filepath, line_number);
	}
	_directives.insert(std::make_pair(directive, value));
}

const std::vector<LocationContext>& ServerContext::getLocations() const
{
	return _locations;
}

const LocationContext& ServerContext::getLocationContext(const std::string& path) const
{
	const std::vector<LocationContext>& locations = getLocations();

	bool isMatched = false;
	std::vector<LocationContext>::const_iterator matched = locations.begin();
	std::string::size_type max = 0;
	for (std::vector<LocationContext>::const_iterator it = locations.begin(); it != locations.end(); ++it)
	{
		std::string locationPath = it->getDirective("path");
		std::string::size_type currentMatch = getMaxPrefixLength(path, locationPath);
		if (currentMatch != std::string::npos && currentMatch == locationPath.size() && currentMatch == path.size())
		{
			if (!isMatched || max < currentMatch)
			{
				max = currentMatch;
				matched = it;
				isMatched = true;
			}
		}
	}
	if (!isMatched)
	{
		return _404LocationContext;
	}
	return *matched;
}

const CGIContext& ServerContext::getCGIContext() const
{
	return _cgi;
}

std::string::size_type ServerContext::getMaxPrefixLength(const std::string& str1, const std::string& str2) const
{
	std::string::size_type i = 0;
	while (i < str1.size() && i < str2.size() && str1[i] == str2[i])
	{
		++i;
	}
	return i;
}

const LocationContext& ServerContext::get404LocationContext() const
{
	return _404LocationContext;
}

const LocationContext& ServerContext::get405LocationContext() const
{
	return _405LocationContext;
}

const LocationContext& ServerContext::get501LocationContext() const
{
	return _501LocationContext;
}
