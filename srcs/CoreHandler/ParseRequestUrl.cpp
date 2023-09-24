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
	// 最初のスラッシュを削除
	if (tokens[0] != "/")
		tokens[0].erase(0, 1);
	std::vector<std::string> path_tokens = split(tokens[0], '/');
	bool is_cgi = false;
	for (size_t i = 0; i < path_tokens.size(); i++)
	{
		if (path_tokens[i] == "cgi-bin")
		{
			is_cgi = true;
			break;
		}
	}
	if (is_cgi)
	{
		return getCgiPath(path_tokens);
	}
	
	LocationContext location_context;
	bool autoindexEnabled = true;
	result.isAutoIndex = true;
    if (location_context.hasDirective("autoindex"))
        autoindexEnabled = location_context.getDirective("autoindex") == "on";
	if (path_tokens[0] == "/")
	{
		std::cout << std::endl << std::endl << "HELLO" << std::endl << std::endl;
		location_context = server_context.getLocationContext("/");
	}
	else
	{
		location_context = server_context.getLocationContext("/" + path_tokens[0] + "/");
	}
	std::cout << "path_tokens[0]: " << path_tokens[0] << std::endl;
	std::string alias = location_context.getDirective("alias");
	if (path_tokens[0] == "/")
		result.directory = "/";
	else
		result.directory = "/" + path_tokens[0] + "/";
	if (path_tokens.size() == 1)
	{
		try {
			result.file = location_context.getDirective("index");
		} catch (const std::exception& e) {
			result.file = "index.html";
		}
	} else {
		if (path_tokens[path_tokens.size() - 1].find('.') != std::string::npos)
		{
			result.file = path_tokens[path_tokens.size() - 1];
			path_tokens.pop_back();
		} else {
			std::cout << "aaaaaaaaaaaa" << std::endl;
			if (autoindexEnabled == false)
				result.isAutoIndex = false;
			result.file = "index.html";
		}
	}
	result.fullpath = alias;
	if (path_tokens.size() == 1)
	{
		result.fullpath += result.file;
		return result;
	}
	// fullpathの最後のスラッシュを削除
	result.fullpath.erase(result.fullpath.size() - 1, 1);
	for (size_t i = 1; i < path_tokens.size(); i++)
	{
		result.fullpath += "/" + path_tokens[i];
	}
	result.fullpath += "/" + result.file;
	return result;
}