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
	std::string response;
	if (location[0] == '/') {
		response = "HTTP/1.1 302 Moved Temporarily\r\n";
		response += "Location: " + location + "\r\n";
		response += "\r\n";
	} else
		// リダイレクト先が外部のURLの場合
		response = "HTTP/1.1 302 Moved Temporarily\r\n";
		response += "Location: http://" + location + "\r\n";
		response += "\r\n";
	return response;
}

std::string successResponse(std::string fileContent, std::string contentType, const std::string& statusCode)
{
	std::string response = "HTTP/1.1 " + statusCode + " OK\r\n";
	response += "Content-Type: " + contentType + "; charset=UTF-8\r\n";
	response += "Content-Length: " + std::to_string(fileContent.size()) + "\r\n";
	response += "\r\n";
	response += fileContent;
	return response;
}

std::string errorResponse(int statusCode, std::string message, const ServerContext &serverContext)
{
	StaticFileReader fileReader;
	std::string fileContent = fileReader.readErrorFile(statusCode, serverContext);
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
        return "text/html";
    }

    std::string ext = filepath.substr(pos);
    if (mimeTypes.find(ext) != mimeTypes.end()) {
        return mimeTypes[ext];
    } else {
        return "text/html";  // 拡張子がマッピングにない場合のデフォルトMIMEタイプ
    }
}

std::string CoreHandler::getMethod(const std::string &fullpath, const LocationContext &locationContext, 
									const ParseUrlResult& result)
{
	// 静的ファイルを提供する場合
	StaticFileReader fileReader;

	// スラッシュが2個続く場合があるため取り除く
	std::string fileContent = fileReader.readFile(fullpath, locationContext, _serverContext, result);
  	std::string contentType = getContentType(fullpath);
	std::string response = successResponse(fileContent, contentType, "200");

	return response;
}

std::string CoreHandler::postMethod(const std::string& body, const std::string& url)
{
	DataProcessor dataProcessor;
	ProcessResult result = dataProcessor.processPostData(body, url, _serverContext);
	if (result.statusCode != 200)
		return errorResponse(result.statusCode, result.message, _serverContext);
	std::string response = successResponse(result.message, "text/html", "201");
	return response;
}

std::string CoreHandler::deleteMethod(const std::string& directory, const std::string& file)
{
	std::cout << "directory + file: " << directory << file << std::endl;
	if (std::remove(("." + directory + file).c_str()) != 0)
	{
		std::cerr << "ERROR: File not found or delete failed.\n";
		std::cout << "DELETE FAILED\n";
		return errorResponse(404, "Not Found", _serverContext);
	}
	return "HTTP/1.1 204 No Content\r\n\r\n"; // 成功のレスポンス
}

int CoreHandler::validatePath(std::string& path)
{
	if (path[path.size() - 1] == '/')
		path.erase(path.size() - 1, 1);
	struct stat buffer;
	int ret = stat(path.c_str(), &buffer);
	return ret;
}

std::string CoreHandler::processRequest(HttpRequest httpRequest,
									const std::pair<std::string, std::string>& hostPort)
{


	// httpRequest.urlが"/"で終わっていない場合に、"/"を追加
	if (httpRequest.url[httpRequest.url.size() - 1] != '/')
	{
		httpRequest.url += '/';
	}
	std::cout << "----------------------------------" << std::endl;
	std::cout << "Request URL: ";
	std::cout << httpRequest.url << std::endl;
	std::cout << "----------------------------------" << std::endl;


	ParseUrlResult parseUrlResult = parseUrl(httpRequest.url);
	std::cout << "parseUrlResult.statusCode: " << parseUrlResult.statusCode << std::endl;
	std::cout << "parseUrlResult.directory: " << parseUrlResult.directory << std::endl;
	if (parseUrlResult.statusCode >= 300 && parseUrlResult.statusCode < 400) {
		std::cout << "===== process redirect =====" << std::endl;
		std::string location;
		if (parseUrlResult.fullpath[0] == '/')
			location = "http://" + hostPort.first + ":" + hostPort.second + parseUrlResult.fullpath;
		else
			location = parseUrlResult.fullpath;
		std::cout << "location: " << location << std::endl;
		return redirectResponse(location);
	} else if (parseUrlResult.statusCode != 200)
		return errorResponse(parseUrlResult.statusCode, parseUrlResult.message, _serverContext);

	LocationContext locationContext;
	try {
		locationContext = _serverContext.getLocationContext(parseUrlResult.directory);
	} catch (std::exception& e) {
		return errorResponse(404, "Not found", _serverContext);
	}
	if (httpRequest.method == "GET")
	{
		if (validatePath(parseUrlResult.fullpath) == -1 && parseUrlResult.autoindex == 0) {
			std::cout << "===== process 404 =====" << std::endl;
			return errorResponse(404, "Not found", _serverContext);
		} if (parseUrlResult.autoindex == 1) {
			std::cout << "===== process autoindex =====" << std::endl;
			std::cout << "parseUrlResult.fullpath: " << parseUrlResult.fullpath << std::endl;
			return getMethod(parseUrlResult.fullpath, locationContext, parseUrlResult);
		}
		if (!locationContext.isAllowedMethod("GET"))
			return errorResponse(405, "Method Not Allowed", _serverContext);
		std::cout << "===== process get method =====" << std::endl;
		return getMethod(parseUrlResult.fullpath, locationContext, parseUrlResult);
	}
	else if (httpRequest.method == "POST")
	{
		if (locationContext.hasDirective("limit_except") && !locationContext.isAllowedMethod("POST"))
		{
			std::cout << "===== process 405 =====" << std::endl;
			return errorResponse(405, "Method Not Allowed", _serverContext);
		}
		std::cout << "===== process post method =====" << std::endl;
		return postMethod(httpRequest.body, parseUrlResult.directory);
	}
	else if (httpRequest.method == "DELETE")
	{
		std::cout << "===== process delete method =====" << std::endl;
		if (locationContext.hasDirective("limit_except") && !locationContext.isAllowedMethod("DELETE"))
			return errorResponse(405, "Method Not Allowed", _serverContext);
		return deleteMethod(parseUrlResult.directory, parseUrlResult.file);
	}
	return errorResponse(501, "Not Implemented", _serverContext);
}

CoreHandler::~CoreHandler()
{
}