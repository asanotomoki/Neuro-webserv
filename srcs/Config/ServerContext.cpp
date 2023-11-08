#include "ServerContext.hpp"
#include "ConfigError.hpp"
#include <stdexcept>
#include <iostream>
#include <set>

ServerContext::ServerContext():
	_listen(DEFAULT_LISTEN),
	_serverNames(),
	_maxBodySize(DEFAULT_MAX_BODY_SIZE),
	_is_Cgi(false),
	_errorPages(),
	_locations(),
	_directives()
{
}

ServerContext::~ServerContext()
{
}

void ServerContext::setListen(const std::string& listen)
{
	int intListen = std::stoi(listen.c_str());
	if (intListen < 1024 || intListen > 65535)
		throw ConfigError(INVALID_LISTEN, listen);
	_listen = listen;
}

void ServerContext::setServerName(const std::string& server_name)
{
	_serverNames.push_back(server_name);
}

void ServerContext::setMaxBodySize(const std::string& max_body_size)
{
	_maxBodySize = max_body_size;
}

void ServerContext::setIsCgi(bool is_cgi)
{
	_is_Cgi = is_cgi;
}

//　設定ファイルから値を読み取る
void ServerContext::setErrorPage(int status_code, const std::string& filename)
{
	_errorPages.insert(std::make_pair(status_code, filename));
}

void ServerContext::setPortAndHostVec()
{
	if (_serverNames.empty())
		_serverNames.push_back("localhost");
	for (std::vector<std::string>::const_iterator it = _serverNames.begin(); it != _serverNames.end(); ++it)
	{
		_portAndHostVec.push_back(std::make_pair(_listen, *it));
	}
}

const std::string& ServerContext::getListen() const
{
	return _listen;
}

const std::string& ServerContext::getMaxBodySize() const
{
	return _maxBodySize;
}

bool ServerContext::getIsCgi() const
{
	return _is_Cgi;
}

std::string ServerContext::getErrorPage(int status_code) const
{
	std::map<int, std::string>::const_iterator it = _errorPages.find(status_code);
	// もし見つからなかった場合
	if (it == _errorPages.end())
		return "";
	return it->second;
}

const std::vector<std::pair<std::string, std::string> >& ServerContext::getPortAndHostVec() const
{
	return _portAndHostVec;
}

void ServerContext::addLocationContext(LocationContext& location)
{
	if (location.hasDirective("return")) {
		_returnLocations.insert(std::make_pair(location.getDirective("path"), location.getDirective("return")));
	}
	_locations.push_back(location);
}

void ServerContext::verifyReturnLocations()
{
	std::map<std::string, std::string>::const_iterator it;
    for (it = _returnLocations.begin(); it != _returnLocations.end(); ++it) {
        std::set<std::string> visited; // 訪れたlocationを保存
        std::string current = it->first;

        while (_returnLocations.find(current) != _returnLocations.end()) {
            if (visited.find(current) != visited.end()) {
                throw ConfigError(INVALID_RETURN, current);
            }
            visited.insert(current);
            current = _returnLocations[current];
        }
    }
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
		throw std::runtime_error("getLocationContext :: location not found: " + path);
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

// returnディレクティブの値を取得する関数
std::string ServerContext::getReturnPath(const std::string& path) const
{
    LocationContext locationContext = getLocationContext(path);
    if (locationContext.hasDirective("return"))
    {
        return locationContext.getDirective("return");
    }
    return "";
}

void ServerContext::addPathPair(const std::pair<std::string, std::string>& pathPair)
{
	_pathMap.insert(pathPair);
}

void ServerContext::addServerPathPair(const std::pair<std::string, std::string>& pathPair)
{
	_serverPathMap.insert(pathPair);
}

const std::string& ServerContext::getServerPath(const std::string& path) const
{
	// _pathMapからpathに対応するserver_pathを取得する
	// pathはmapのpairの1つ目の要素
	std::map<std::string, std::string>::const_iterator it = _serverPathMap.find(path);
	if (it == _serverPathMap.end())
		return path;
	return it->second;
}

const std::string& ServerContext::getClientPath(const std::string& path) const
{
	// _pathMapからpathに対応するclient_pathを取得する
	// pathはmapのpairの最初の要素
	std::map<std::string, std::string>::const_iterator it = _pathMap.find(path);
	if (it == _pathMap.end())
		return path;
	return it->second;
}

const std::string ServerContext::DEFAULT_LISTEN = "65535";
const std::string ServerContext::DEFAULT_MAX_BODY_SIZE = "1048576";