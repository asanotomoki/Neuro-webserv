#include "CoreHandler.hpp"
#include "ServerContext.hpp"
#include "utils.hpp"
#include <sstream>
#include <vector>
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>

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
		if (locationContext.hasDirective("index")) {
			file = locationContext.getDirective("index");
			if (!fileExists(result.fullpath + file) && locationContext.hasDirective("autoindex"))
				result.autoindex = 1;
		}
		else { //設計上、必ずautoindexディレクティブが存在する
			if (locationContext.getDirective("autoindex") == "on") {
				if (isFileIncluded(tokens))
					result.errorflag = 1;
				result.autoindex = 1;
				file = "";
			} else { //autoindexがoffの場合
				file = _serverContext.getErrorPage(403);
				result.statusCode = 403;
				result.message = "Forbidden";
			}
		}
	} else {
		file = _serverContext.getErrorPage(404);
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
	LocationContext location_context;
	try {
		location_context = _serverContext.getLocationContext("/");
	} catch (std::exception& e) {
		result.statusCode = 404;
		result.message = "Not Found";
		return;
	}
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
					result.file = _serverContext.getErrorPage(403);
					result.statusCode = 403;
					result.message = "Forbidden";
				}
				return;
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
	std::cout << "directory1: " << result.directory << std::endl;
	for (size_t i = 1; i < path_tokens.size(); i++) {
		if (!isFile(path_tokens[i]))
			result.directory += path_tokens[i] + "/";
		else
			break ;
	}
	std::cout << "directory2: " << result.directory << std::endl;
	if (isFile(result.directory)) {
		result.file = result.directory.substr(1, result.directory.size());
		result.directory = "/";
		LocationContext location_context;
		try {
			location_context = _serverContext.getLocationContext("/");
		} catch (std::exception& e) {
			result.statusCode = 404;
			result.message = "Not Found";
			return result;
		}
		result.fullpath = location_context.getDirective("alias") + result.file;
		return result;
	}
	std::string redirectPath;
	try {
		redirectPath = _serverContext.getReturnPath(result.directory);
	} catch (std::exception& e) {
		result.statusCode = 404;
		result.message = "Not Found";
		return result;
	}
    if (!redirectPath.empty()) {
		result.statusCode = 302;
		result.fullpath = redirectPath;
		// size_t i = 1;
		// while (i < path_tokens.size())
		// {
		// 	result.fullpath += path_tokens[i++] + "/";
		// }
		return result;
	}
	if (path_tokens.size() == 1 && isFile(path_tokens[0])) {
		parseHomeDirectory(tokens[0], result);
		return result;
	}
	LocationContext locationContext;
	try {
		locationContext = _serverContext.getLocationContext(result.directory);
	} catch (std::exception& e) {
		result.statusCode = 404;
		result.message = "Not Found";
		return result;
	}

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

	result.fullpath.erase(result.fullpath.size() - 1, 1);
	result.fullpath += "/" + result.file;
	return result;
}
