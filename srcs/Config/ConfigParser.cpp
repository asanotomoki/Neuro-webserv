#include "ConfigParser.hpp"
#include "ServerContext.hpp"
#include "LocationContext.hpp"
#include "CGIContext.hpp"
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
	else if (directive == "cgi")
		_directiveType = CGI;
	else if (directive == "alias")
		_directiveType = ALIAS;
	else if (directive == "index")
		_directiveType = INDEX;
	else if (directive == "limit_except")
		_directiveType = LIMIT_EXCEPT;
	else if (directive == "command")
		_directiveType = COMMAND;
	else if (directive == "cgi_on")
		_directiveType = CGI_ON;
	else if (directive == "return")
		_directiveType = RETURN;
	else if (directive == "autoindex")
		_directiveType = AUTOINDEX;
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
		|| _directiveType == LOCATION || _directiveType == CGI;
}

bool ConfigParser::isInLocationContext()
{
	return  _directiveType == ALIAS || _directiveType == INDEX
		|| _directiveType == LIMIT_EXCEPT || _directiveType == COMMAND
		|| _directiveType == CGI_ON || _directiveType == RETURN
		|| _directiveType == AUTOINDEX;
}

bool ConfigParser::isInCgiContext()
{
	return _directiveType == ALIAS || _directiveType == INDEX
		|| _directiveType == LIMIT_EXCEPT || _directiveType == COMMAND;
}

bool ConfigParser::isAllowedDirective()
{
	if (_contextType == HTTP_CONTEXT)
		return isInHttpContext();
	else if (_contextType == SERVER_CONTEXT)
		return isInServerContext();
	else if (_contextType == LOCATION_CONTEXT)
		return isInLocationContext();
	else if (_contextType == CGI_CONTEXT)
		return isInCgiContext();
	return false;
}

void ConfigParser::parseFile(const std::string& filepath)
{
	_filepath = filepath;

	// パスがディレクトリの場合はエラー
	if (!isFile(_filepath.c_str()))
	{
		std::cerr << "Given filepath is not found" << std::endl;
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
			throw ConfigError(NOT_ALLOWED_DIRECTIVE, _oneLine[0], _filepath, _lineNumber + 1);
		else if (_directiveType == SERVER){
			ServerContext serverContext = setServerContext();
			_config.addServerContext(serverContext);
		}
		else
			throw ConfigError(NEED_SERVER_CONTEXT, _oneLine[0], _filepath, _lineNumber + 1);
	}
}

const ServerContext ConfigParser::setServerContext()
{
	ServerContext serverContext = ServerContext();

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
		if (!isAllowedDirective())
			throw ConfigError(NOT_ALLOWED_DIRECTIVE, _oneLine[0], _filepath, _lineNumber + 1);
		else if (_directiveType == LOCATION) {
			LocationContext locationContext = setLocationContext();
			serverContext.addLocationContext(locationContext);
		}
		else if (_directiveType == CGI) {
			CGIContext cgiContext = setCGIContext();
			serverContext.addCGIContext(cgiContext);
		}
		else {
			serverContext.addDirectives(_oneLine[0], _oneLine[1], _filepath, _lineNumber + 1);
			if (_directiveType == LISTEN)
				serverContext.setListen(_oneLine[1]);
			else if (_directiveType == SERVER_NAME)
				serverContext.setServerName(_oneLine[1]);
			else if (_directiveType == MAX_BODY_SIZE)
				serverContext.setMaxBodySize(_oneLine[1]);
			else if (_directiveType == ERROR_PAGE)
				serverContext.setErrorPage(_oneLine[1], _oneLine[2]);
		}
	}
	serverContext.setErrorPages();
	return serverContext;
}

const LocationContext ConfigParser::setLocationContext()
{
	LocationContext locationContext = LocationContext();

	locationContext.addDirective("path", _oneLine[1], _filepath, _lineNumber + 1);
	_lineNumber++;
	for ( ; _lineNumber < _lines.size(); _lineNumber++) {
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
		else if (_directiveType == LIMIT_EXCEPT) {
			for (size_t i = 1; i < _oneLine.size(); ++i) {
        		locationContext.addAllowedMethod(_oneLine[i]);
    		}
		}
		else
			locationContext.addDirective(_oneLine[0], _oneLine[1], _filepath, _lineNumber + 1);
	}
	if (!locationContext.hasDirective("alias") && !locationContext.hasDirective("return")) {
		std::cout << "debug" << std::endl;
		throw ConfigError(NEED_ALIAS, "location", _filepath);
	}
	return locationContext;
}

const CGIContext ConfigParser::setCGIContext()
{
	CGIContext cgiContext = CGIContext();

	cgiContext.addDirective("extension", _oneLine[1], _filepath, _lineNumber + 1);
	_lineNumber++;
	for ( ; _lineNumber < _lines.size(); _lineNumber++) {
		setContextType(CGI_CONTEXT);
		_oneLine.clear();
		_oneLine = _lines[_lineNumber];
		if (_oneLine.empty() || _oneLine[0] == "#")
			continue ;
		if (_oneLine[0] == "}")
			break ;
		setDirectiveType(_oneLine[0]);
		if (!isAllowedDirective())
			throw ConfigError(NOT_ALLOWED_DIRECTIVE, _oneLine[0], _filepath, _lineNumber + 1);
		//　必要があれば追加
		// else if (_directiveType == LIMIT_EXCEPT){
		// 	for (size_t i = 1; i < _oneLine.size(); ++i) {
        // 		CGIContext.addAllowedMethod(_oneLine[i]);
    	// 	}
		// }
		else
			cgiContext.addDirective(_oneLine[0], _oneLine[1], _filepath, _lineNumber + 1);
	}
	return cgiContext;
}

bool ConfigParser::isFile(const char *path)
{
	struct stat st;

	if (stat(path, &st) == 0) {
		// パスがファイルであるか
		if (S_ISREG(st.st_mode))
			return true;
	}
	return false;
}
