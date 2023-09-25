#include "CoreHandler.hpp"
#include <sstream>
#include <vector>

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

int isFile(std::string path)
{
	if (path.find('.') != std::string::npos)
		return 1;
	return 0;
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
	result.directory = "./docs/cgi-bin";
	// 必ずcgi-binが含まれているので、その次の要素がファイル名
	result.file = tokens[1];
	// その以降の要素がパスインフォ
	for (size_t i = 2; i < tokens.size(); i++)
	{
		result.pathInfo += "/" + tokens[i];
	}
	result.fullpath = result.directory + "/" + result.file;
	return result;
}

bool getIsAutoIndex(LocationContext &location_context, std::string path)
{
	bool autoindexEnabled = true;
	bool res = true;
	if (location_context.hasDirective("autoindex"))
	{
        autoindexEnabled = location_context.getDirective("autoindex") == "on";
	}
	std::cout << "getIsAutoIndex path :: " << path << std::endl << std::endl;
	if (!isFile(path) && !autoindexEnabled)
	{
		res = false;
	}
	return res;
}

std::string getFile(std::vector<std::string> tokens, LocationContext &location_context)
{
	std::string file;
	if (isFile(tokens[tokens.size() - 1]))
	{
		file = tokens[tokens.size() - 1];
	} else {
		try {
			file = location_context.getDirective("index");
		} catch (const std::exception& e) {
			file = "index.html";
		}
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
	
	alias.erase(alias.size() - 1, 1);
	result.fullpath = alias + result.file;
	result.fullpath.erase(result.fullpath.size() - 1, 1);
	result.isAutoIndex = getIsAutoIndex(location_context, url);
	return result;
}


ParseUrlResult CoreHandler::parseUrl(std::string url, const ServerContext& server_context)
{
	ParseUrlResult result;
	std::vector<std::string> tokens = split(url, '?');
	if (tokens.size() == 1)
	{
		result.query = "";
	} else {
		result.query = tokens[1];
	}	
	// home directory
	if (tokens[0] == "/")
	{
		std::cout << std::endl << std::endl << "home directory" << std::endl << std::endl;
		ParseUrlResult res = parseHomeDirectory(url, server_context);
		res.query = result.query;
		return res;
	}
	tokens[0].erase(0, 1);
	// cgi-bin
	std::vector<std::string> path_tokens = split(tokens[0], '/');
	if (isCgiDir(path_tokens))
	{
		return getCgiPath(path_tokens);
	}
	if (path_tokens.size() == 1 && isFile(path_tokens[0]))
	{
		ParseUrlResult res = parseHomeDirectory(url, server_context);
		res.query = result.query;
		return res;
	}
	LocationContext location_context;

	result.directory = "/" + path_tokens[0] + "/";
	location_context = server_context.getLocationContext(result.directory);
	std::string alias = location_context.getDirective("alias");
	result.file = getFile(path_tokens, location_context);
	result.isAutoIndex = getIsAutoIndex(location_context, path_tokens[path_tokens.size() - 1]);


	// fullpathの最後のスラッシュを削除
	result.fullpath = alias;

	
	result.fullpath.erase(result.fullpath.size() - 1, 1);

	std::cout << "fullpath alias: " << result.fullpath << std::endl;
	size_t path_size = path_tokens.size() - isFile(path_tokens[path_tokens.size() - 1]);
	for (size_t i = 1; i < path_size; i++)
	{
		result.fullpath += "/" + path_tokens[i];
	}
	result.fullpath +=  "/" + result.file;
	return result;
}