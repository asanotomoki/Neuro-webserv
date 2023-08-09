#include "ConfigError.hpp"
#include <errno.h>
#include <cstring>

ConfigError::ConfigError(const ErrorType error_type, const std::string& error_word,
						const std::string& filepath, int line_number):
	_error_message("Config Error: "), 
	_file_info(" in " + filepath + ": " + itostr(line_number))
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
		case DUPRICATE_DIRECTIVE:
			_error_message += "duplicate \"" + error_word + "\" directive" + _file_info;
			break;
		case NOT_ALLOWED_DIRECTIVE:
			_error_message += "\"" + error_word + "\" directive is not allowed here" + _file_info;
			break;
		case UNKOWN_DIRECTIVE:
			_error_message += "unknown directive \"" + error_word + "\"" + _file_info;
			break;
		case SYSTEM_ERROR:
			_error_message += "system call error \"" + error_word + ": " + strerror(errno) + "\"" + _file_info;
			break;
		default:
			break;
	}
}

const char* ConfigError::what() const throw()
{
	return _error_message.c_str();
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
