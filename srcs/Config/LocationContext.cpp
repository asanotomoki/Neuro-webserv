#include "LocationContext.hpp"
#include "ConfigError.hpp"
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