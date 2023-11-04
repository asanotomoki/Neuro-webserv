#ifndef CGI_PARSER_HPP
#define CGI_PARSER_HPP

#include "ServerContext.hpp"
#include "Config.hpp"
#include <string>
#include <map>
#include <sstream>
#include <iostream>

enum CgiResponseType
{
	Document,
	Client_Redirect,
	Server_Redirect,
};

class CgiParser
{
public:
	CgiParser();
	CgiParser(std::string header, std::string body, std::string method);
	~CgiParser();
	std::string generateCgiResponse();

	CgiResponseType getCgiResponseType() const;
	

private:
	CgiResponseType _cgiResponseType;
	std::map<std::string, std::string> _headers;
	std::string _body;
	int _contentLength;
	int _statusCode;
	std::string _statusMessage;
	std::string _location;
	std::string _method;

	CgiResponseType setCgiResponseType(); 
	void setHeaders(std::string header);
	void setContentLength();
	void setStatusCode();
	void setStatusMessage();
	void setLocation();
	void setBody(std::string body);

	std::string generateDocumentResponse();
	std::string generateClientRedirectResponse();
	std::string generateServerRedirectResponse();

};

#endif 
