#include "ServerContext.hpp"
#include "ConfigError.hpp"
#include <stdexcept>
#include <iostream>
#include <set>

ServerContext::ServerContext():
	_listen(DEFAULT_LISTEN),
	_serverName(),
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
	// 403を指定したディレクティブがなかった場合は、デフォルトのエラーページを設定する
	if (_errorPages.find("403") == _errorPages.end()) {
		_403LocationContext.addDirective("alias", "./docs/error_page/default/");
		_403LocationContext.addDirective("index", "403.html");
		_403LocationContext.addAllowedMethod("GET");
	} else {
		_403LocationContext.addDirective("alias", "./docs/error_page/");
		_403LocationContext.addDirective("index", getErrorPage("403"));
		_403LocationContext.addAllowedMethod("GET");
	}

	if (_errorPages.find("404") == _errorPages.end()) {
		_404LocationContext.addDirective("alias", "./docs/error_page/default/");
		_404LocationContext.addDirective("index", "404.html");
		_404LocationContext.addAllowedMethod("GET");
	} else {
		_404LocationContext.addDirective("alias", "./docs/error_page/");
		_404LocationContext.addDirective("index", getErrorPage("404"));
		_404LocationContext.addAllowedMethod("GET");
	}

	if (_errorPages.find("405") == _errorPages.end()) {
		_405LocationContext.addDirective("alias", "./docs/error_page/default/");
		_405LocationContext.addDirective("index", "405.html");
		_405LocationContext.addAllowedMethod("GET");
	} else {
		_405LocationContext.addDirective("alias", "./docs/error_page/");
		_405LocationContext.addDirective("index", getErrorPage("405"));
		_405LocationContext.addAllowedMethod("GET");
	}

	if (_errorPages.find("500") == _errorPages.end()) {
		_500LocationContext.addDirective("alias", "./docs/error_page/default/");
		_500LocationContext.addDirective("index", "500.html");
		_500LocationContext.addAllowedMethod("GET");
	} else {
		_500LocationContext.addDirective("alias", "./docs/error_page/");
		_500LocationContext.addDirective("index", getErrorPage("500"));
		_500LocationContext.addAllowedMethod("GET");
	}

	if (_errorPages.find("501") == _errorPages.end()) {
		_501LocationContext.addDirective("alias", "./docs/error_page/default/");
		_501LocationContext.addDirective("index", "501.html");
		_501LocationContext.addAllowedMethod("GET");
	} else {
		_501LocationContext.addDirective("alias", "./docs/error_page/");
		_501LocationContext.addDirective("index", getErrorPage("501"));	
		_501LocationContext.addAllowedMethod("GET");
	}
}

//　設定ファイルから値を読み取る
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
	return it->second;
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

const LocationContext& ServerContext::get403LocationContext() const
{
	return _403LocationContext;
}

const LocationContext& ServerContext::get404LocationContext() const
{
	return _404LocationContext;
}

const LocationContext& ServerContext::get405LocationContext() const
{
	return _405LocationContext;
}

const LocationContext& ServerContext::get500LocationContext() const
{
	return _500LocationContext;
}

const LocationContext& ServerContext::get501LocationContext() const
{
	return _501LocationContext;
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

void ServerContext::addPathPair(const std::pair<std::string, std::string>& path_pair)
{
	_pathMap.insert(path_pair);
}

const std::string& ServerContext::getClientPath(const std::string& path) const
{
	// _pathMapからpathに対応するclient_pathを取得する
	// pathはmapのpairの最初の要素
	std::map<std::string, std::string>::const_iterator it = _pathMap.find(path);
	if (it == _pathMap.end())
		throw std::runtime_error("getClientPath :: path not found: " + path);
	return it->second;
}

const std::string ServerContext::DEFAULT_LISTEN = "65535";
const std::string ServerContext::DEFAULT_MAX_BODY_SIZE = "1048576";