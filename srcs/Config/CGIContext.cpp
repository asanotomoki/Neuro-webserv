#include "CGIContext.hpp"
#include "ConfigError.hpp"

CGIContext::CGIContext()
{
}

CGIContext::~CGIContext()
{
}

void CGIContext::addDirective(const std::string& directive, const std::string& value,
                            const std::string& filepath, int lineNumber)
{
    std::map<std::string, std::string>::iterator found = _directives.find(directive);
    if (found != _directives.end())
    {
        throw ConfigError(DUPLICATE_DIRECTIVE, directive, filepath, lineNumber);
    }
    _directives[directive] = value;
}

const std::string& CGIContext::getDirective(const std::string& directive) const
{
    std::map<std::string, std::string>::const_iterator found = _directives.find(directive);
    if (found == _directives.end())
    {
        throw ConfigError(DUPLICATE_DIRECTIVE, directive);
    }
    return found->second;
}
