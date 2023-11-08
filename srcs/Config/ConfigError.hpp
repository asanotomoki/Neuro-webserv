#ifndef CONFIGERROR_HPP
#define CONFIGERROR_HPP

#include <string>
#include <exception>

enum ErrorType
{
	NEED_SERVER_CONTEXT,
	DUPLICATE_DIRECTIVE,
	NOT_ALLOWED_DIRECTIVE,
	UNKOWN_DIRECTIVE,
	INVALID_LISTEN,
	INVALID_ERROR_PAGE,
	INVALID_PATH,
	INVALID_RETURN,
	NEED_ALIAS,
	NEED_INDEX,
	DUPLICATE_PORT_AND_HOST,
	SYSTEM_ERROR
};

class ConfigError : public std::exception
{
	public:
		ConfigError(const ErrorType error_type, const std::string& error_word,
					const std::string& filepath = "", int lineNumber = -1);
		~ConfigError() throw();
		void setErrorMessage(const ErrorType errorType, const std::string& errorWord);
		const char* what() const throw();

	private:
		std::string itostr(int num);
		std::string _errorMessage;
		std::string _fileInfo;
};

#endif
