#include "iostream"

std::string getErrorMessage(int status_code) {
	std::string message;
	if (400 <= status_code && status_code < 500)
	{
		if (status_code == 401)
			message = "Unauthorized";
		else if (status_code == 403)
			message = "Forbidden";
		else if(status_code == 404)
			message = "Not Found";
		else if (status_code == 405)
			message = "Method Not Allowed";
		else if (status_code == 411)
			message = "Length Required";
		else if (status_code == 413)
			message = "Payload Too Large";
		else
			message = "Bad Request";
	}
	else if (500 <= status_code && status_code < 600)
	{
		if (status_code == 501)
			message = "Not Implemented";
		else if (status_code == 502)
			message = "Bad Gateway";
		else if (status_code == 503)
			message = "Service Unavailable";
		else if (status_code == 504)
			message = "Gateway Timeout";
		else if (status_code == 505)
			message = "HTTP Version Not Supported";
		else if (status_code == 507)
			message = "Insufficient Storage";
		else if (status_code == 510)
			message = "Not Extended";
		else
			message = "Internal Server Error";
	}
	else
		message = "Unknown Error";
	return message;
}

std::string errorHeader(int status_code) 
{
	std::string message = getErrorMessage(status_code);
	std::string str_code = std::to_string(status_code);
	std::string response = "HTTP/1.1 " + str_code + " " + message;
	response += "\r\n";
	response += "content-type: text/html";
	response += "\r\n";
	response += "Connection: close";
	response += "\r\n";
	return response;
}

std::string default_error_page(int status_code)
{
	std::string header = errorHeader(status_code);
	std::string str_code = std::to_string(status_code);
	std::string errorBodyContent = "<!DOCTYPE html>\n<html>\n<head>\n<meta charset = 'UTF-8'>\n<meta name = 'viewport'>\n<title> Error " + str_code + "</title>\n</head>\n<body>\n<h1> "+ str_code + " " + getErrorMessage(status_code) + "</h1>\n</body>\n</html>\n";
	std::string response = header;
	response += "Content-Length: " + std::to_string(errorBodyContent.length());
	response += "\r\n\r\n";
	response += errorBodyContent;

	return response;
}

std::string error_page(int status_code, std::string page)
{
	std::string response = errorHeader(status_code);
	response += "Content-Length: " + std::to_string(page.size());
	response += "\r\n\r\n";
	response += page;

	return response;
}