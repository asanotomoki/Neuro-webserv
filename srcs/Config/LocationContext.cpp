#include "LocationContext.hpp"
#include "ConfigError.hpp"
#include <stdexcept>

LocationContext::LocationContext():
	_directives()
{
}

LocationContext::~LocationContext()
{
}

void LocationContext::addDirective(const std::string& directive, const std::string& value,
	const std::string& filepath, int line_number)
{
	// check if directive is not duplicated
	if (_directives.find(directive) != _directives.end())
		throw ConfigError(DUPRICATE_DIRECTIVE, directive, filepath, line_number);

	_directives.insert(std::make_pair(directive, value));
}

const std::string& LocationContext::getDirective(const std::string& directive) const
{
	std::map<std::string, std::string>::const_iterator it = _directives.find(directive);
	if (it == _directives.end())
		throw std::runtime_error("directive not found");
	return it->second;
}
