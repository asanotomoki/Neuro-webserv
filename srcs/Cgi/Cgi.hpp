#ifndef CGI_HPP
#define CGI_HPP

#include <iostream>
#include <unistd.h>
#include <map>

// CGIの実行結果を格納する構造体

struct CgiResult
{
	int fd;
	std::string body;
	pid_t pid;
	int statusCode;
};



struct ParseUrlCgiResult
{
    std::string file;
    std::string directory;
    std::string fullpath;
    std::string query;
    std::string pathInfo;
	std::string command;
    int statusCode;
    std::string message;
    int autoindex;
    int errorflag;
};
#include "RequestParser.hpp"
//CGIクラス
class Cgi
{
	private:
		// execveを実行するのに必要な変数
		std::map<std::string, std::string> _env;
		HttpRequest _request;
		const char* _executable;
		const char* _path;
		ParseUrlCgiResult _parseUrlCgiResult;
		int pipe_stdin[2];
		std::vector<std::string> _args;
		std::string _method;

		
		char** mapToChar(const std::map<std::string, std::string>& map);
		char** vectorToChar(const std::vector<std::string>& vector);
		ParseUrlCgiResult getCgiPath(std::vector<std::string> tokens, ServerContext &serverContext, ParseUrlCgiResult &result);
		ParseUrlCgiResult parseCgiBlock(std::vector<std::string> tokens, const ServerContext &server_context, ParseUrlCgiResult &result);
	public:
		//Cgi(HttpRequest &request, std::string executable, ParseUrlResult &url);
		Cgi();
		Cgi(HttpRequest &request, ServerContext &server_context);
    	~Cgi();
		
		void execCGI(CgiResult &result);
		void parseUrl(std::string url, ServerContext &server_context);
		
		// env
		const std::map<std::string, std::string>& getEnv() const;
		//void initEnv(HttpRequest &request, ParseUrlResult url);
		void initEnv(HttpRequest &request);
		int* getPipeStdin();
};

#endif
