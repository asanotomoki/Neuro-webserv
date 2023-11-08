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

std::string successResponse(std::string fileContent, std::string contentType, 
					const std::string& statusCode, std::string postLocation = "")
{
	std::string message = "OK";
	if (statusCode == "201") {
		message = "Created";
	} else if (statusCode == "204") {
		message = "No Content";
	}
	std::string response = "HTTP/1.1 " + statusCode + " " + message + "\r\n";
	response += "Content-Type: " + contentType + "; charset=UTF-8\r\n";
	response += "Content-Length: " + std::to_string(fileContent.size()) + "\r\n";
	if (postLocation != "")
		response += "Location: " + postLocation + "\r\n";
	response += "\r\n";
	response += fileContent;
	return response;
}

std::string errorResponse(int statusCode, std::string message, const ServerContext &serverContext)
{
	StaticFileReader fileReader;
	std::string response =fileReader.readErrorFile(statusCode, serverContext, message);
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
	mimeTypes.insert(std::make_pair(".mp3", "audio/mpeg"));
	mimeTypes.insert(std::make_pair(".mp4", "video/mp4"));
	mimeTypes.insert(std::make_pair(".ico", "image/x-icon"));


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
	std::string fileContent;
	try {
		fileContent = fileReader.readFile(fullpath, locationContext, _serverContext, result);
	} catch (std::exception &e) {
		std::cerr << "ERROR: " << e.what() << std::endl;
		return errorResponse(404, "Not Found", _serverContext);
	}

  	std::string contentType = getContentType(fullpath);
	std::string response = successResponse(fileContent, contentType, "200");

	return response;
}

std::string CoreHandler::postMethod(const std::string& body, const std::string& url)
{
	DataProcessor dataProcessor;
	ProcessResult result = dataProcessor.processPostData(body, url, _serverContext);
	if (result.statusCode != 201)
		return errorResponse(result.statusCode, result.message, _serverContext);
	std::string response = successResponse(result.message, "text/html", "201", result.location);
	return response;
}

std::string CoreHandler::deleteMethod(const std::string& fullpath)
{
	if (std::remove(fullpath.c_str()) != 0)
	{
		std::cerr << "ERROR: File not found or delete failed.\n";
		std::cout << "DELETE FAILED\n";
		return errorResponse(404, "Not Found", _serverContext);
	}
	std::string response = successResponse("DELETE SUCCESS", "text/html", "204", fullpath);
	return response;
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


	ParseUrlResult parseUrlResult = parseUrl(httpRequest.url);
	if (parseUrlResult.statusCode >= 300 && parseUrlResult.statusCode < 400) {
		std::string location;
		if (parseUrlResult.fullpath[0] == '/')
			location = "http://" + hostPort.first + ":" + hostPort.second + parseUrlResult.fullpath;
		else
			location = parseUrlResult.fullpath;
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
		if (!locationContext.isAllowedMethod("GET"))
			return errorResponse(405, "Method Not Allowed", _serverContext);
		if (validatePath(parseUrlResult.fullpath) == -1 && parseUrlResult.autoindex == 0)
			return errorResponse(404, "Not found", _serverContext);
		return getMethod(parseUrlResult.fullpath, locationContext, parseUrlResult);
	}
	else if (httpRequest.method == "POST")
	{
		if (!locationContext.isAllowedMethod("POST"))
		{
			return errorResponse(405, "Method Not Allowed", _serverContext);
		}
		return postMethod(httpRequest.body, parseUrlResult.directory);
	}
	else if (httpRequest.method == "DELETE")
	{
		if (!locationContext.isAllowedMethod("DELETE"))
			return errorResponse(405, "Method Not Allowed", _serverContext);
		return deleteMethod(parseUrlResult.fullpath);
	}
	std::cerr << "ERROR: Invalid method." << std::endl;
	return errorResponse(405, "Method Not Allowed", _serverContext);
}

CoreHandler::~CoreHandler()
{
}