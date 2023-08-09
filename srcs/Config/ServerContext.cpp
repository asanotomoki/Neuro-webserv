#include "ServerContext.hpp"
#include "ConfigError.hpp"
#include <stdexcept>
#include <iostream>

ServerContext::ServerContext():
	_listen(),
	_server_name()
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
	_server_name = server_name;
}

const std::string& ServerContext::getListen() const
{
	return _listen;
}

const std::string& ServerContext::getServerName() const
{
	return _server_name;
}

void ServerContext::addLocationBlock(const LocationContext& location)
{
	_locations.push_back(location);
}

void ServerContext::addDirectives(const std::string& directive, const std::string& value,
	const std::string& filepath, int line_number)
{
	// check if directive is not duplicated
	if (_directives.find(directive) != _directives.end())
		throw ConfigError(DUPRICATE_DIRECTIVE, directive, filepath, line_number);

	_directives.insert(std::make_pair(directive, value));
}

const std::vector<LocationContext>& ServerContext::getLocations() const
{
	return _locations;
}

/**
 * @brief パスパターンが一致するLocationブロックを取得する
 *
 * @detail 前方一致で検索し、最も長い文字列がマッチしたLocationブロックを返す
 * @param path 検索するパスパターン
 * @return const LocationContext& 一致したLocationContext
 */
const LocationContext& ServerContext::getLocationContext(const std::string& path) const
{
	const std::vector<LocationContext>& locations = getLocations();

	bool isMatched = false;
	std::vector<LocationContext>::const_iterator matched = locations.begin();
	std::string::size_type max = 0;
	for (std::vector<LocationContext>::const_iterator it = locations.begin(); it != locations.end(); ++it)
	{
		// ロケーションパスを取得
		std::string locationPath = it->getDirective("path");
		// 前方一致の最大の長さを取得する
		std::string::size_type currentMatch = getMaxPrefixLength(path, locationPath);
		if (currentMatch != std::string::npos)
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
		throw std::runtime_error("No matching LocationContext found for the given path.");
	}
	return *matched;
}

/**
 * @brief 前方一致する最大の長さを求める
 *
 * @detail
 * @param str1 比較する文字列1
 * @param str2 比較する文字列2
 * @return std::string::size_type 前方一致する最大の長さ
 */
std::string::size_type ServerContext::getMaxPrefixLength(const std::string& str1, const std::string& str2) const
{
	std::string::size_type i = 0;
	while (i < str1.size() && i < str2.size() && str1[i] == str2[i])
	{
		++i;
	}
	return i;
}
