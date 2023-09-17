#include "ConfigError.hpp"
#include <errno.h>
#include <cstring>

ConfigError::ConfigError(const ErrorType error_type, const std::string& error_word,
						const std::string& filepath, int line_number):
	_errorMessage("Config Error: "), 
	_fileInfo(" in " + filepath + ": " + itostr(line_number))
{
	setErrorMessage(error_type, error_word);
}

ConfigError::ConfigError(const ErrorType error_type, const std::string& error_word)
{
	setErrorMessage(error_type, error_word);
}



ConfigError::~ConfigError() throw()
{
}

void ConfigError::setErrorMessage(const ErrorType error_type, const std::string& error_word)
{
	switch (error_type)
	{
		case NEED_SERVER_CONTEXT:
			_errorMessage += "need server context" + _fileInfo;
			break;
		case DUPLICATE_DIRECTIVE:
			_errorMessage += "duplicate \"" + error_word + "\" directive" + _fileInfo;
			break;
		case NOT_ALLOWED_DIRECTIVE:
			_errorMessage += "\"" + error_word + "\" directive is not allowed here" + _fileInfo;
			break;
		case UNKOWN_DIRECTIVE:
			_errorMessage += "unknown directive \"" + error_word + "\"" + _fileInfo;
			break;
		case INVALID_PATH:
			_errorMessage += "invalid path \"" + error_word + "\"" + _fileInfo;
			break;
		case SYSTEM_ERROR:
			_errorMessage += "system call error \"" + error_word + ": " + strerror(errno) + "\"" + _fileInfo;
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
