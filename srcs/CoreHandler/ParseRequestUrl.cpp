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
    DIR* dir = opendir(path.c_str());
    if (dir) {
        closedir(dir);
        return 1;
    }
    return 0;
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
				result.message = "Forbidden";
			}
		}
	} else {
		locationContext = _serverContext.get404LocationContext();
		file = locationContext.getDirective("index");
		result.statusCode = 404;
		result.message = "Not Found";
	}
	return file;
}

bool isExistingFile(std::string path) {
	struct stat buffer;
	int ret = stat(path.c_str(), &buffer);
	if (ret == 0)
		return true;
	return false;
}

void CoreHandler::parseHomeDirectory(std::string url, ParseUrlResult& result)
{
	LocationContext location_context = _serverContext.getLocationContext("/");
	result.directory = "/";
	std::string alias = location_context.getDirective("alias");
	if (url != "/")
		result.file = url;
	else {
		if (location_context.hasDirective("autoindex")) {
			if (location_context.hasDirective("index")) {
				result.file = location_context.getDirective("index");
				if (location_context.getDirective("autoindex") == "on") {
					if (!isExistingFile(alias + result.file))
						result.autoindex = 1;
				}
			} else {
				if (location_context.getDirective("autoindex") == "on") {
					result.autoindex = 1;
					result.file = "";
				} else {
					location_context = _serverContext.get403LocationContext();
					result.file = location_context.getDirective("index");
					result.statusCode = 403;
					result.message = "Forbidden";
				}
				return ;
			}
		} else {
			result.file = location_context.getDirective("index");
		}
	}
	result.fullpath = alias + result.file;
	return;
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
		parseHomeDirectory(url, result);
		return result;
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
	if (path_tokens.size() == 1 && isFile(path_tokens[0])) {
		parseHomeDirectory(tokens[0], result);
		return result;
	}

	LocationContext locationContext;
	locationContext = _serverContext.getLocationContext(result.directory);
	std::string alias = locationContext.getDirective("alias");

	// fullpathの最後のスラッシュを削除
	result.fullpath = alias;
	result.file = getFile(path_tokens, locationContext, result);
	if (result.autoindex == 1 && result.file == "") {
		// 余計なパスや存在しないパスのため
		std::string arg = alias + path_tokens[path_tokens.size() - 1];
		if (path_tokens.size() > 2 && validatePath(arg) == -1 && result.errorflag == 1) {
			result.statusCode = 404;
			result.message = "Not Found";
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
	return result;
}
