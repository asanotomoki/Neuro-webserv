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
#include <map>
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

std::string getContentType(const std::string& filepath) {
    // MIMEタイプのマッピング
    static std::map<std::string, std::string> mimeTypes;
	mimeTypes.insert(std::make_pair(".html", "text/html"));
	mimeTypes.insert(std::make_pair(".css", "text/css"));
	mimeTypes.insert(std::make_pair(".js", "application/javascript"));
	mimeTypes.insert(std::make_pair(".png", "image/png"));
	mimeTypes.insert(std::make_pair(".jpg", "image/jpeg"));
	mimeTypes.insert(std::make_pair(".jpeg", "image/jpeg"));
	mimeTypes.insert(std::make_pair(".gif", "image/gif"));
	mimeTypes.insert(std::make_pair(".ico", "image/x-icon"));
	mimeTypes.insert(std::make_pair(".json", "application/json"));
	mimeTypes.insert(std::make_pair(".xml", "application/xml"));
	mimeTypes.insert(std::make_pair(".pdf", "application/pdf"));
	mimeTypes.insert(std::make_pair(".zip", "application/zip"));
	mimeTypes.insert(std::make_pair(".tar", "application/x-tar"));
	mimeTypes.insert(std::make_pair(".txt", "text/plain"));

    // ファイル名から最後のドットを検索して拡張子を取得
    size_t pos = filepath.find_last_of(".");
    if (pos == std::string::npos) {
        return "application/octet-stream";  // 拡張子がない場合のデフォルトMIMEタイプ
    }

    std::string ext = filepath.substr(pos);
    if (mimeTypes.find(ext) != mimeTypes.end()) {
        return mimeTypes[ext];
    } else {
        return "application/octet-stream";  // 拡張子がマッピングにない場合のデフォルトMIMEタイプ
    }
}

std::string CoreHandler::getMethod(const std::string &fullpath, const LocationContext &locationContext,
					  bool isAutoIndex)
{
	// 静的ファイルを提供する場合
	StaticFileReader fileReader;
	std::cout << "fullpath: " << fullpath << "\n";

	// スラッシュが2個続く場合があるため取り除く

	std::string fileContent = fileReader.readFile(fullpath, locationContext,
												  _serverContext, isAutoIndex);
																								  
	std::string contentType = getContentType(fullpath);
	std::cout << "contentType: " << contentType << "\n"; 
	std::string response = successResponse(fileContent, contentType);
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

	std::cout << "httpRequest.url: " << httpRequest.url << "\n";
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
		return getMethod(parseUrlResult.fullpath, locationContext, parseUrlResult.isAutoIndex);
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