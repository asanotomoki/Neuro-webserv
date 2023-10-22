#include "CoreHandler.hpp"
#include "RequestParser.hpp"
#include "StaticFileReader.hpp"
#include "DataProcessor.hpp"
#include "LocationContext.hpp"
#include <iostream>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
CoreHandler::CoreHandler()
{
	ServerContext serverContext;
	_serverContext = serverContext;
}

CoreHandler::CoreHandler(const ServerContext &serverContext) : _serverContext(const_cast<ServerContext &>(serverContext))
{
}

std::string redirectResponse(std::string location)
{
	std::string response = "HTTP/1.1 302 Moved Permanently\r\n";
	response += "Location: " + location + "\r\n";
	response += "\r\n";
	return response;
}

std::string successResponse(std::string fileContent, std::string contentType)
{
	std::string response = "HTTP/1.1 200 OK\r\n";
	response += "Content-Type: " + contentType + "; charset=UTF-8\r\n";
	response += "Content-Length: " + std::to_string(fileContent.size()) + "\r\n";
	response += "\r\n";
	response += fileContent;
	return response;
}

std::string errorResponse(int statusCode, std::string message, const LocationContext &locationContext)
{
	StaticFileReader fileReader;
	std::string fileContent = fileReader.readErrorFile(locationContext, statusCode);
	std::string response = "HTTP/1.1 " + std::to_string(statusCode) + " " + message + "\r\n";
	response += "Content-Type: text/html\r\n";
	response += "Content-Length: " + std::to_string(fileContent.size()) + "\r\n";
	response += "\r\n";
	response += fileContent;
	return response;
}

std::string CoreHandler::getMethod(const std::string &fullpath, const LocationContext &locationContext)
{
	// 静的ファイルを提供する場合
	StaticFileReader fileReader;

	std::string fileContent = fileReader.readFile(fullpath, locationContext, _serverContext);
	std::string response = successResponse(fileContent, "text/html");
	return response;
}

std::string CoreHandler::postMethod(std::string body)
{
	DataProcessor dataProcessor;
	ProcessResult result = dataProcessor.processPostData(body);
	std::string response = successResponse(result.message, "text/html");
	return response;
}

std::string CoreHandler::deleteMethod(const std::string &filename)
{
	std::cout << "deleteMethod :: filename: " << filename << "\n";
	if (std::remove(("./docs/upload/" + filename).c_str()) != 0)
	{
		std::cerr << "deleteMethod :: ERROR: File not found or delete failed.\n";
		std::cout << "deleteMethod :: DELETE FAILED\n";
		LocationContext locationContext = _serverContext.get404LocationContext();
		return errorResponse(404, "Not Found", locationContext);
	}
	return "HTTP/1.1 204 No Content\r\n\r\n"; // 成功のレスポンス
}

std::string CoreHandler::processRequest(HttpRequest httpRequest)
{
	if (httpRequest.url == "/favicon.ico")
	{
		std::cout << "WARNING: favicon.ico request, ignoring\n";
		return "";
	}

	// httpRequest.urlが"/"で終わっていない場合に、"/"を追加
	if (httpRequest.url[httpRequest.url.size() - 1] != '/')
	{
		httpRequest.url += '/';
	}

	ParseUrlResult parseUrlResult = parseUrl(httpRequest.url);

	LocationContext locationContext = _serverContext.getLocationContext(parseUrlResult.directory);

	// directoryからLocationContextを取得
	locationContext = _serverContext.getLocationContext(parseUrlResult.directory);
	// if (parseUrlResult.statusCode >= 300 && parseUrlResult.statusCode < 400) {
	//     std::string location = "http://" + hostPort.first + ":" + hostPort.second + parseUrlResult.fullpath;
	//     return redirectResponse(location);
	// }
	if (httpRequest.method == "GET")
	{
		if (!locationContext.isAllowedMethod("GET"))
		{
			locationContext = _serverContext.get405LocationContext();
			return errorResponse(405, "Method Not Allowed", locationContext);
		}
		return getMethod(parseUrlResult.fullpath, locationContext);
	}
	else if (httpRequest.method == "POST")
	{
		if (!locationContext.isAllowedMethod("POST"))
		{
			locationContext = _serverContext.get405LocationContext();
			return errorResponse(405, "Method Not Allowed", locationContext);
		}
		return postMethod(httpRequest.body);
	}
	else if (httpRequest.method == "DELETE")
	{
		if (!locationContext.isAllowedMethod("DELETE"))
		{
			locationContext = _serverContext.get405LocationContext();
			return errorResponse(405, "Method Not Allowed", locationContext);
		}
		return deleteMethod(parseUrlResult.file);
	}
	locationContext = _serverContext.get501LocationContext();
	return errorResponse(501, "Not Implemented", locationContext);
}

CoreHandler::~CoreHandler()
{
}