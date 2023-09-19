#ifndef CGI_HPP
#define CGI_HPP

#include <iostream>
#include <unistd.h>
#include <map>
#include "RequestParser.hpp"
#include "ServerContext.hpp"

//CGIクラス
class Cgi
{
	private:
		// execveを実行するのに必要な変数
		Cgi();
		std::map<std::string, std::string> _env;
		const char* _executable;
		const char* _path;
		std::vector<std::string> _args;

		
		char** mapToChar(const std::map<std::string, std::string>& map);
		char** vectorToChar(const std::vector<std::string>& vector);

	public:
		Cgi(HttpRequest &request, std::string executable, std::string path);
    	~Cgi();
		
		std::string CgiHandler();
		
		// env
		const std::map<std::string, std::string>& getEnv() const;
		void initEnv(HttpRequest &request, std::string path);
};

#endif
