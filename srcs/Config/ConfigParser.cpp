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
	_line_number(0)
{
}

ConfigParser::~ConfigParser()
{
}

void ConfigParser::setContextType(ContextType context)
{
	_context_type = context;
}

void ConfigParser::setDirectiveType(const std::string& directive)
{
	if (directive == "server")
		_directive_type = SERVER;
	else if (directive == "listen")
		_directive_type = LISTEN;
	else if (directive == "server_name")
		_directive_type = SERVER_NAME;
	else if (directive == "client_max_body_size")
		_directive_type = MAX_BODY_SIZE;
	else if (directive == "location")
		_directive_type = LOCATION;
	else if (directive == "alias")
		_directive_type = ALIAS;
	else if (directive == "index")
		_directive_type = INDEX;
	else
		_directive_type = UNKNOWN;
}

bool ConfigParser::isInHttpContext()
{
	return _directive_type == SERVER;
}

bool ConfigParser::isInServerContext()
{
	return _directive_type == LISTEN || _directive_type == SERVER_NAME
			|| _directive_type == MAX_BODY_SIZE || _directive_type == LOCATION;
}

bool ConfigParser::isInLocationContext()
{
	return  _directive_type == ALIAS || _directive_type == INDEX
			|| _directive_type == ERROR_PAGE;
}

bool ConfigParser::isAllowedDirective()
{
	if (_context_type == HTTP_CONTEXT)
		return isInHttpContext();
	else if (_context_type == SERVER_CONTEXT)
		return isInServerContext();
	else if (_context_type == LOCATION_CONTEXT)
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
	for ( ; _line_number < _lines.size(); _line_number++)
	{
		setContextType(HTTP_CONTEXT);
		_one_line.clear();
		_one_line = _lines[_line_number];
		if (_one_line.empty() || _one_line[0] == "#")
			continue ;
		if (_one_line[0] == "}")
			break ;
		setDirectiveType(_one_line[0]);
		if (!isAllowedDirective())
		{
			throw ConfigError(NOT_ALLOWED_DIRECTIVE, _one_line[0], _filepath, _line_number + 1);
		}
		else if (_directive_type == SERVER){
			ServerContext server_context = setServerContext();
			_config.addServerContext(server_context);
		}
		else
		{
			throw ConfigError(NEED_SERVER_CONTEXT, _one_line[0], _filepath, _line_number + 1);
		}
	}
}

const ServerContext ConfigParser::setServerContext()
{
	ServerContext server_context = ServerContext();

	_line_number++;
	for ( ; _line_number < _lines.size(); _line_number++)
	{
		setContextType(SERVER_CONTEXT);
		_one_line.clear();
		_one_line = _lines[_line_number];
		if (_one_line.empty() || _one_line[0] == "#")
			continue ;
		if (_one_line[0] == "}")
			break ;
		setDirectiveType(_one_line[0]);
		if (!isAllowedDirective())
			throw ConfigError(NOT_ALLOWED_DIRECTIVE, _one_line[0], _filepath, _line_number + 1);
		else if (_directive_type == LOCATION)
		{
			LocationContext location_context = setLocationContext();
			server_context.addLocationBlock(location_context);
		}
		else
		{
			server_context.addDirectives(_one_line[0], _one_line[1], _filepath, _line_number + 1);
			if (_directive_type == LISTEN)
				server_context.setListen(_one_line[1]);
			else if (_directive_type == SERVER_NAME)
				server_context.setServerName(_one_line[1]);
			else if (_directive_type == MAX_BODY_SIZE)
				server_context.setMaxBodySize(_one_line[1]);
		}
	}
	return server_context;
}

const LocationContext ConfigParser::setLocationContext()
{
	LocationContext location_context = LocationContext();

	location_context.addDirective("path", _one_line[1], _filepath, _line_number + 1);
	_line_number++;
	for ( ; _line_number < _lines.size(); _line_number++)
	{
		setContextType(LOCATION_CONTEXT);
		_one_line.clear();
		_one_line = _lines[_line_number];
		if (_one_line.empty() || _one_line[0] == "#")
			continue ;
		if (_one_line[0] == "}")
			break ;
		setDirectiveType(_one_line[0]);
		if (!isAllowedDirective())
			throw ConfigError(NOT_ALLOWED_DIRECTIVE, _one_line[0], _filepath, _line_number + 1);
		else if (_directive_type == ERROR_PAGE)
			location_context.addDirective(_one_line[1], _one_line[2], _filepath, _line_number + 1);
		else
			location_context.addDirective(_one_line[0], _one_line[1], _filepath, _line_number + 1);
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

