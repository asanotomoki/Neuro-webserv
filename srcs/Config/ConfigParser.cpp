#include "ConfigParser.hpp"
#include "ServerContext.hpp"
#include "LocationContext.hpp"
#include "ConfigError.hpp"
#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>

ConfigParser::ConfigParser(Config& config):
	_config(config),
	_lineNumber(0)
{
}

ConfigParser::~ConfigParser()
{
}

void ConfigParser::setContextType(ContextType context)
{
	_contextType = context;
}

void ConfigParser::setDirectiveType(const std::string& directive)
{
	if (directive == "server")
		_directiveType = SERVER;
	else if (directive == "listen")
		_directiveType = LISTEN;
	else if (directive == "server_name")
		_directiveType = SERVER_NAME;
	else if (directive == "client_max_body_size")
		_directiveType = MAX_BODY_SIZE;
	else if (directive == "error_page")
		_directiveType = ERROR_PAGE;
	else if (directive == "location")
		_directiveType = LOCATION;
	else if (directive == "alias")
		_directiveType = ALIAS;
	else if (directive == "name")
		_directiveType = NAME;
	else if (directive == "limit_except")
		_directiveType = LIMIT_EXCEPT;
	else
		_directiveType = UNKNOWN;
}

bool ConfigParser::isInHttpContext()
{
	return _directiveType == SERVER;
}

bool ConfigParser::isInServerContext()
{
	return _directiveType == LISTEN || _directiveType == SERVER_NAME
		|| _directiveType == MAX_BODY_SIZE || _directiveType == ERROR_PAGE
		|| _directiveType == LOCATION;
}

bool ConfigParser::isInLocationContext()
{
	return  _directiveType == ALIAS || _directiveType == NAME
		|| _directiveType == LIMIT_EXCEPT;
}

bool ConfigParser::isAllowedDirective()
{
	if (_contextType == HTTP_CONTEXT)
		return isInHttpContext();
	else if (_contextType == SERVER_CONTEXT)
		return isInServerContext();
	else if (_contextType == LOCATION_CONTEXT)
		return isInLocationContext();
	return false;
}

void ConfigParser::parseFile(const std::string& filepath)
{
	_filepath = filepath;

	// パスがディレクトリの場合はエラー
	if (!isFile(_filepath.c_str()))
	{
		std::cerr << "Given filepath is a directory" << std::endl;
		exit(EXIT_FAILURE);
	}
	std::ifstream ifs(_filepath.c_str());
	if (!ifs)
	{
		std::cerr << "Open Error" << std::endl;
		exit(EXIT_FAILURE);
	}
	getAndSplitLines(ifs);
	ifs.close();
	parseLines();
}

void ConfigParser::getAndSplitLines(std::ifstream& ifs)
{
	std::string line;
	while (std::getline(ifs, line))
	{
		const std::vector<std::string> words = splitLine(line);
		_lines.push_back(words);
	}
}

std::vector<std::string> ConfigParser::splitLine(const std::string& line)
{
	std::vector<std::string> words;
	size_t start = 0;
	size_t end = 0;

	while (line[start])
	{
		while (isspace(line[start]))
			start++;
		end = start;
		while (isprint(line[end]) && !isspace(line[end])
				&& (line[end] != '{' && line[end] != '}') && line[end] != ';')
			end++;
		if (end > start)
			words.push_back(line.substr(start, end - start));
		if (line[end] == '{' || line[end] == '}' || line[end] == ';')
		{
			words.push_back(line.substr(end, 1));
			end++;
		}
		start = end;
	}
	return words;
}

void ConfigParser::parseLines()
{
	for ( ; _lineNumber < _lines.size(); _lineNumber++)
	{
		setContextType(HTTP_CONTEXT);
		_oneLine.clear();
		_oneLine = _lines[_lineNumber];
		if (_oneLine.empty() || _oneLine[0] == "#")
			continue ;
		if (_oneLine[0] == "}")
			break ;
		setDirectiveType(_oneLine[0]);
		if (!isAllowedDirective())
		{
			throw ConfigError(NOT_ALLOWED_DIRECTIVE, _oneLine[0], _filepath, _lineNumber + 1);
		}
		else if (_directiveType == SERVER){
			ServerContext server_context = setServerContext();
			_config.addServerContext(server_context);
		}
		else
		{
			throw ConfigError(NEED_SERVER_CONTEXT, _oneLine[0], _filepath, _lineNumber + 1);
		}
	}
}

const ServerContext ConfigParser::setServerContext()
{
	ServerContext server_context = ServerContext();

	_lineNumber++;
	for ( ; _lineNumber < _lines.size(); _lineNumber++)
	{
		setContextType(SERVER_CONTEXT);
		_oneLine.clear();
		_oneLine = _lines[_lineNumber];
		if (_oneLine.empty() || _oneLine[0] == "#")
			continue ;
		if (_oneLine[0] == "}")
			break ;
		setDirectiveType(_oneLine[0]);
		if (!isAllowedDirective()) {
			std::cout << "DEBUG MSG" << std::endl;
			throw ConfigError(NOT_ALLOWED_DIRECTIVE, _oneLine[0], _filepath, _lineNumber + 1);
		}
		else if (_directiveType == LOCATION)
		{
			LocationContext location_context = setLocationContext();
			server_context.addLocationContext(location_context);
		}
		else
		{
			server_context.addDirectives(_oneLine[0], _oneLine[1], _filepath, _lineNumber + 1);
			if (_directiveType == LISTEN)
				server_context.setListen(_oneLine[1]);
			else if (_directiveType == SERVER_NAME)
				server_context.setServerName(_oneLine[1]);
			else if (_directiveType == MAX_BODY_SIZE)
				server_context.setMaxBodySize(_oneLine[1]);
			else if (_directiveType == ERROR_PAGE){
				server_context.setErrorPage(_oneLine[1], _oneLine[2]);
			}
		}
	}
	return server_context;
}

const LocationContext ConfigParser::setLocationContext()
{
	LocationContext location_context = LocationContext();

	location_context.addDirective("path", _oneLine[1], _filepath, _lineNumber + 1);
	_lineNumber++;
	for ( ; _lineNumber < _lines.size(); _lineNumber++)
	{
		setContextType(LOCATION_CONTEXT);
		_oneLine.clear();
		_oneLine = _lines[_lineNumber];
		if (_oneLine.empty() || _oneLine[0] == "#")
			continue ;
		if (_oneLine[0] == "}")
			break ;
		setDirectiveType(_oneLine[0]);
		if (!isAllowedDirective())
			throw ConfigError(NOT_ALLOWED_DIRECTIVE, _oneLine[0], _filepath, _lineNumber + 1);
		else if (_directiveType == LIMIT_EXCEPT){
			for (size_t i = 1; i < _oneLine.size(); ++i) {
        		location_context.addAllowedMethod(_oneLine[i]);
    		}
		}
		else
			location_context.addDirective(_oneLine[0], _oneLine[1], _filepath, _lineNumber + 1);
	}
	return location_context;
}

bool ConfigParser::isFile(const char *path)
{
	struct stat st;

	if (stat(path, &st) == 0)
	{
		// パスがファイルであるか
		if (S_ISREG(st.st_mode))
		{
			return true;
		}
	}
	return false;
}

