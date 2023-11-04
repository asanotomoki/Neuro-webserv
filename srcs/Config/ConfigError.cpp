#include "ConfigError.hpp"
#include <errno.h>
#include <cstring>
#include <iostream>

ConfigError::ConfigError(const ErrorType errorType, const std::string& errorWord,
						const std::string& filepath, int lineNumber):
	_errorMessage("Config Error: ")
{
	if (!filepath.empty() && lineNumber != -1)
		_fileInfo = " in " + filepath + ": " + itostr(lineNumber);
	else if (!filepath.empty())
		_fileInfo = " in " + filepath;
	std::cerr << "ConfigError :: " << _fileInfo << std::endl;
	setErrorMessage(errorType, errorWord);
}

ConfigError::~ConfigError() throw()
{
}

void ConfigError::setErrorMessage(const ErrorType errorType, const std::string& errorWord)
{
	switch (errorType)
	{
		case NEED_SERVER_CONTEXT:
			_errorMessage += "need server context" + _fileInfo;
			break;
		case DUPLICATE_DIRECTIVE:
			_errorMessage += "duplicate \"" + errorWord + "\" directive" + _fileInfo;
			break;
		case NOT_ALLOWED_DIRECTIVE:
			_errorMessage += "\"" + errorWord + "\" directive is not allowed here" + _fileInfo;
			break;
		case UNKOWN_DIRECTIVE:
			_errorMessage += "unknown directive \"" + errorWord + "\"" + _fileInfo;
			break;
		case INVALID_LISTEN:
			_errorMessage += "invalid listen directive \"" + errorWord + "\"" + _fileInfo;
			break;
		case INVALID_ERROR_PAGE:
			_errorMessage += "invalid error_page directive \"" + errorWord + "\"" + _fileInfo;
			break;
		case INVALID_PATH:
			_errorMessage += "invalid path \"" + errorWord + "\"" + _fileInfo;
			break;
		case DUPLICATE_PORT:
			_errorMessage += "duplicate port \"" + errorWord + "\"" + _fileInfo;
			break;
		case NEED_ALIAS:
			_errorMessage += "need alias directive" + _fileInfo;
			break;
		case NEED_INDEX:
			_errorMessage += "need index or autoindex direcitve" + _fileInfo;
			break;
		case INVALID_RETURN:
			_errorMessage += "invalid return directive" + _fileInfo;
			break;
		case SYSTEM_ERROR:
			_errorMessage += "system call error \"" + errorWord + ": " + strerror(errno) + "\"" + _fileInfo;
			break;
		default:
			break;
	}
}

const char* ConfigError::what() const throw()
{
	return _errorMessage.c_str();
}

std::string ConfigError::itostr(int num)
{
	std::string str;

	if (num == 0)
		return "0";
	while (num)
	{
		str = (char)(num % 10 + '0') + str;
		num /= 10;
	}
	return str;
}
