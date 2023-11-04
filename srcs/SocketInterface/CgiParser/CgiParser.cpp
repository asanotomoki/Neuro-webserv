#include "CgiParser.hpp"

CgiParser::CgiParser()
{
}

CgiParser::CgiParser(std::string header, std::string body, std::string method) : _body(body), _method(method)
{
	setHeaders(header);
	setContentLength();
	setStatusCode();
	setCgiResponseType();
	setStatusMessage();
	setLocation();
}

CgiParser::~CgiParser()
{
}

CgiResponseType CgiParser::setCgiResponseType()
{
	// Content-Typeがあり、BodyがあるならDocument
	if (_headers.find("Content-Type") != _headers.end() && _contentLength > 0)
	{
		_cgiResponseType = Document;
		return Document;
	}
	// Locationがあり、相対パスならServer_Redirect
	if (_headers.find("Location") != _headers.end() && _headers["Location"][0] == '/')
	{
		_cgiResponseType = Server_Redirect;
		return Server_Redirect;
	}
	// Locationがあり、絶対パスならClient_Redirect
	if (_headers.find("Location") != _headers.end() && _headers["Location"][0] != '/')
	{
		_cgiResponseType = Client_Redirect;
		return Client_Redirect;
	}
	// それ以外はDocument
	_cgiResponseType = Document;
	return Document;
}

void CgiParser::setHeaders(std::string header)
{
	
	std::istringstream headerStream(header);
	std::string headerLine;
	while (std::getline(headerStream, headerLine))
	{
		std::istringstream headerStream(headerLine);
		std::string key;
		std::string value;
		std::getline(headerStream, key, ':');
		std::getline(headerStream, value);
		if (!value.empty() && value[0] == ' ')
		{
			value = value.substr(1); // 先頭のスペースを削除
		}
		_headers[key] = value;
	}
}

std::string CgiParser::generateDocumentResponse()
{
	std::string response;
	response += "HTTP/1.1 " + std::to_string(_statusCode) + " " + _statusMessage + "\r\n";
	response += "Content-Length: " + std::to_string(_contentLength) + "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		if (it->first != "Content-Length" && it->first != "Status")
		{
			response += it->first + ": " + it->second + "\r\n";
		}
	}
	response += "\r\n";
	response += _body;
	return response;
}

std::string CgiParser::generateClientRedirectResponse()
{
	std::string response;
	response += "HTTP/1.1 302 Found\r\n";
	response += "Location: " + _location + "\r\n";
	response += "\r\n";
	return response;
}

std::string CgiParser::generateServerRedirectResponse()
{
	std::string response;
	response += _method + " " + _location + " HTTP/1.1\r\n";
	response += "Content-Length: " + std::to_string(_contentLength) + "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		if (it->first != "Method" && it->first != "Location" && it->first != "Content-Length" && it->first != "Status")
		{
			response += it->first + ": " + it->second + "\r\n";
		}
	}
	response += "\r\n";
	response += _body;
	return response;
}

std::string CgiParser::generateCgiResponse()
{
	if (_cgiResponseType == Document)
	{
		return generateDocumentResponse();
	}
	if (_cgiResponseType == Client_Redirect)
	{
		return generateClientRedirectResponse();
	}
	if (_cgiResponseType == Server_Redirect)
	{
		return generateServerRedirectResponse();
	}
	return ""; // ここには来ないはず
}

void CgiParser::setContentLength()
{
	if (_headers.find("Content-Length") != _headers.end())
	{
		try
		{
			_contentLength = std::stoi(_headers["Content-Length"]);
		}
		catch (std::invalid_argument e)
		{
			_contentLength = _body.length();
		}
	}
	else
	{
		_contentLength = _body.length();
	}
}

void CgiParser::setStatusMessage()
{
	if (_headers.find("Status") != _headers.end())
	{
		try
		{
			_statusMessage = _headers["Status"].substr(4);
		}
		catch (std::invalid_argument e)
		{
			_statusMessage = "OK";
		}
	}
	else
	{
		_statusMessage = "OK";
	}
}

void CgiParser::setStatusCode()
{
	if (_headers.find("Status") != _headers.end())
	{
		try
		{
			_statusCode = std::stoi(_headers["Status"]);
		}
		catch (std::invalid_argument e)
		{
			_statusCode = 200;
		}
	}
	else
	{
		_statusCode = 200;
	}
}

void CgiParser::setBody(std::string body)
{
	_body = body;
}

void CgiParser::setLocation()
{
	if (_headers.find("Location") != _headers.end())
	{
		_location = _headers["Location"];
	}
	else
	{
		_location = "";
	}
}

CgiResponseType CgiParser::getCgiResponseType() const
{
	return _cgiResponseType;
}