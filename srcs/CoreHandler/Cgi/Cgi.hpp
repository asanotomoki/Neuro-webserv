#ifndef CGI_HPP
#define CGI_HPP

#include <iostream>
#include <unistd.h>
#include <map>
#include "RequestParser.hpp"
#include "ServerContext.hpp"
#include "CoreHandler.hpp"

// CGIの実行結果を格納する構造体
// status: OK or NG
// message: CGIの実行結果
// statusCode: CGIの実行結果のステータスコード
struct CgiResponse {
	std::string message;
	int status;
};
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
		Cgi(HttpRequest &request, std::string executable, ParseUrlResult &url);
    	~Cgi();
		
		CgiResponse CgiHandler();
		
		// env
		const std::map<std::string, std::string>& getEnv() const;
		void initEnv(HttpRequest &request, ParseUrlResult url);
};

#endif
