#include "LocationContext.hpp"
#include "ConfigError.hpp"
#include <iostream>
#include <stdexcept>

LocationContext::LocationContext():
	_allowedMethods(),
	_directives()
{
}

LocationContext::~LocationContext()
{
}

void LocationContext::addDirective(const std::string& directive, const std::string& value,
	const std::string& filepath, int lineNumber)
{
	if (directive == "path") {
		// パスが"/dir1"のように"/"で終わっていない場合は例外を投げる
		if (value[value.size() - 1] != '/') {
			throw ConfigError(INVALID_PATH, value, filepath, lineNumber);
		}
		// パスに"/dir1/dir2/"のようにディレクトリが複数含まれる場合は例外を投げる
		// 条件は、２つ目の"/"の次の文字がnposでないこと
		// size_t secondSlashPos = value.find('/', value.find('/') + 1);
		// if (secondSlashPos != std::string::npos && secondSlashPos != value.size() - 1) {
		// 	throw ConfigError(INVALID_PATH, value, filepath, lineNumber);
		// }
	}
	// check if directive is not duplicated
	if (_directives.find(directive) != _directives.end()) {
		throw ConfigError(DUPLICATE_DIRECTIVE, directive, filepath, lineNumber);
	}
	_directives.insert(std::make_pair(directive, value));
}

void LocationContext::addAllowedMethod(const std::string& method)
{
	_allowedMethods.insert(method);
}

const std::string& LocationContext::getDirective(const std::string& directive) const
{
	std::map<std::string, std::string>::const_iterator it = _directives.find(directive);
	if (it == _directives.end())
		throw std::runtime_error("directive not found");
	return it->second;
}

bool LocationContext::isAllowedMethod(const std::string& method) const
{
	if (_allowedMethods.empty())
		return true;
	return _allowedMethods.find(method) != _allowedMethods.end();
}

bool LocationContext::getIsCgi() const
{
	std::map<std::string, std::string>::const_iterator it = _directives.find("cgi_on");
	if (it == _directives.end())
		return false;
	if (it->second == "on")
		return true;
	return false;
}

std::map<std::string, std::string> LocationContext::getDirectives() const
{
	return _directives;
}

bool LocationContext::hasDirective(const std::string& directive)
{
	return _directives.find(directive) != _directives.end();
}

void LocationContext::setPathPair(const std::string& alias, const std::string& path)
{
	_pathPair = std::make_pair(alias, path);
}

void LocationContext::setPathPairRev(const std::string& path, const std::string& alias)
{
	_pathPairRev = std::make_pair(path, alias);
}