#include "CoreHandler.hpp"
#include "ServerContext.hpp"
#include <sstream>
#include <vector>
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>

std::vector<std::string> split(const std::string &s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);

	if (s.empty())
		return tokens;
	if (s == "/")
	{
		tokens.push_back("/");
		return tokens;
	}
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

bool fileExists(const std::string& path) {

  struct stat buffer;   
  return (stat (path.c_str(), &buffer) == 0); 

}

int CoreHandler::isFile(const std::string& token, std::string fullpath)
{
	if (token.find('.') != std::string::npos)
		return 1;
	else if (fullpath != "" && !fileExists(fullpath))
		return 1;
	return 0;
}

int isDirectory_x(std::string path) {
	std::cout << "isdir path: " << path << std::endl;
    DIR* dir = opendir(path.c_str());
    if (dir) {
        closedir(dir);
        return 1;
    }
    return 0;
}

bool isCgiBlockPath(const ServerContext& server_context, std::vector<std::string> tokens)
{
	if (!server_context.getIsCgi())
		return false;
	CGIContext cgi_context = server_context.getCGIContext();
	std::string exe = cgi_context.getDirective("extension");
	size_t i = 0;
	while (i < tokens.size())
	{
		// 拡張子を取得
		std::string ext = tokens[i].substr(tokens[i].find_last_of(".") + 1);
		if (ext == exe)
			return true;
		i++;
	}
	return false;
}

ParseUrlResult parseCgiBlock(std::vector<std::string> tokens, const ServerContext& server_context)
{
	ParseUrlResult result;
	result.directory = "/cgi-bin/";
	CGIContext cgi_context = server_context.getCGIContext();
	std::string exe = cgi_context.getDirective("extension");
	size_t i = 0;
	while (i < tokens.size())
	{
		result.file += "/" + tokens[i];
		std::string ext = tokens[i].substr(tokens[i].find_last_of(".") + 1);
		if (ext == exe)
			break;
		i++;
	}
	result.fullpath = "./docs/cgi-bin" + result.file;
	result.pathInfo = "";
	// それ以降がpathinfo
	for (size_t j = i + 1; j < tokens.size(); j++)
	{
		result.pathInfo += "/" + tokens[j];
	}
	return result;
}

bool isCgiDir(std::vector<std::string> tokens)
{
	if (tokens.size() < 2)
		return false;
	if (tokens[0] == "cgi-bin")
		return true;
	return false;
}

ParseUrlResult getCgiPath(std::vector<std::string> tokens)
{
	ParseUrlResult result;
	result.directory = "/cgi-bin/";
	// 必ずcgi-binが含まれているので、その次の要素がファイル名
	result.file = tokens[1];
	// その以降の要素がパスインフォ
	for (size_t i = 2; i < tokens.size(); i++)
	{
		result.pathInfo += "/" + tokens[i];
	}
	result.fullpath =  + "./docs/cgi-bin/" + result.file;
	return result;
}

// bool getIsAutoIndex(LocationContext &location_context, std::string path)
// {
// 	bool autoindexEnabled = true;
// 	bool res = true;
// 	if (location_context.hasDirective("autoindex"))
// 	{
//         autoindexEnabled = location_context.getDirective("autoindex") == "on";
// 	}
// 	if (!isFile(path) && !autoindexEnabled)
// 	{
// 		res = false;
// 	}
// 	return res;
// }

bool CoreHandler::isFileIncluded(std::vector<std::string> tokens) {
	//最後のトークン以外にファイルが含まれるか調べる
	for (size_t i = 0; i < tokens.size() - 1 ; i++) {
		if (isFile(tokens[i]))
			return true;
	}
	return false;
}

std::string CoreHandler::getFile(std::vector<std::string> tokens, LocationContext &locationContext,
								ParseUrlResult &result)
{
	std::string file;
	if (isFile(tokens[tokens.size() - 1], result.fullpath)) {
		file = tokens[tokens.size() - 1];
	} else if (isDirectory_x(result.fullpath)) {
		if (locationContext.hasDirective("index"))
			file = locationContext.getDirective("index");
		else { //設計上、必ずautoindexディレクティブが存在する
			if (locationContext.getDirective("autoindex") == "on") {
				if (isFileIncluded(tokens))
					result.errorflag = 1;
				result.autoindex = 1;
				file = "";
			} else { //autoindexがoffの場合
				locationContext = _serverContext.get403LocationContext();
				file = locationContext.getDirective("index");
				result.statusCode = 403;
			}
		}
	} else {
		locationContext = _serverContext.get404LocationContext();
		file = locationContext.getDirective("index");
		result.statusCode = 404;
	}
	return file;
}

ParseUrlResult parseHomeDirectory(std::string url, const ServerContext& server_context)
{
	ParseUrlResult result;
	LocationContext location_context = server_context.getLocationContext("/");
	result.directory = "/";
	std::string alias = location_context.getDirective("alias");
	if (url != "/")
	{
		result.file = url;
	} else {
		try {
			result.file = location_context.getDirective("index");
		} catch (const std::exception& e) {
			result.file = "index.html";
		}
	}
	// alias.erase(alias.size() - 1, 1);
	result.fullpath = alias + result.file;
	std::cout << "result.fullpath: " << result.fullpath << std::endl;
	// result.fullpath.erase(result.fullpath.size() - 1, 1);
	// result.isAutoIndex = getIsAutoIndex(location_context, url);
	return result;
}

ParseUrlResult CoreHandler::parseUrl(std::string url)
{
	ParseUrlResult result;
	result.statusCode = 200;
	result.autoindex = 0;
	result.errorflag = 0;
	std::vector<std::string> tokens = split(url, '?');
	if (tokens.size() == 1) {
		result.query = "";
	} else {
		result.query = tokens[1];
		result.query.erase(result.query.size() - 1, 1);
	}	
	// home directory
	if (tokens[0] == "/") {
		ParseUrlResult res = parseHomeDirectory(url, _serverContext);
		res.query = result.query;
		res.statusCode = result.statusCode;
		return res;
	}
	tokens[0].erase(0, 1);
	// cgi-bin
	std::vector<std::string> path_tokens = split(tokens[0], '/');
	result.directory = "/" + path_tokens[0] + "/";
	for (size_t i = 1; i < path_tokens.size(); i++) {
		if (!isFile(path_tokens[i]))
			result.directory += path_tokens[i] + "/";
		else
			break ;
	}
	std::cout << "result.directory1: " << result.directory << std::endl;
	std::string redirectPath = _serverContext.getReturnPath(result.directory);
    if (!redirectPath.empty()) {
		result.statusCode = 302;
		result.fullpath = redirectPath;
		size_t i = 1;
		while (i < path_tokens.size())
		{
			result.fullpath += path_tokens[i++] + "/";
		}
		return result;
	}
	if (isCgiDir(path_tokens))
		return getCgiPath(path_tokens);
	if (isCgiBlockPath(_serverContext, path_tokens))
		return parseCgiBlock(path_tokens, _serverContext);
	if (path_tokens.size() == 1 && isFile(path_tokens[0])) {
		ParseUrlResult res = parseHomeDirectory(url, _serverContext);
		res.query = result.query;
		return res;
	}
	LocationContext locationContext;
	std::cout << "result.directory2: " << result.directory << std::endl;
	locationContext = _serverContext.getLocationContext(result.directory);
	std::string alias = locationContext.getDirective("alias");
	result.fullpath = alias;
	std::cout << "alias: " << alias << std::endl;
	result.file = getFile(path_tokens, locationContext, result);
	std::cout << "result.file: " << result.file << std::endl;
	if (result.autoindex == 1 && result.file == "") {
		std::cout << "debuuuuuuuug 1" << std::endl;
		// 余計なパスや存在しないパスのため		
		if (path_tokens.size() > 2 && validatePath(alias + path_tokens[path_tokens.size() - 1]) == -1
			&& result.errorflag == 1) {
			std::cout << "debuuuuuuuug 2" << std::endl;
			result.statusCode = 404;
			return result;
		}
		return result;
	}
	// result.isAutoIndex = getIsAutoIndex(location_context, path_tokens[path_tokens.size() - 1]);

	result.fullpath.erase(result.fullpath.size() - 1, 1);
	// size_t path_size = path_tokens.size() - isFile(path_tokens[path_tokens.size() - 1]);
	// for (size_t i = 1; i < path_size; i++)
	// {
	// 	result.fullpath += "/" + path_tokens[i];
	// }
	result.fullpath += "/" + result.file;
	std::cout << "result.fullpath: " << result.fullpath << std::endl;
	std::cout << "ステータスコード: " << result.statusCode << std::endl;
	return result;
}
