#ifndef CGI_HPP
#define CGI_HPP

#include <iostream>
#include <unistd.h>
#include <map>

//CGIクラス
class Cgi
{
	private:
		std::map<std::string, std::string> _env;
	public:
		Cgi();
    	~Cgi();
		
		void CgiHandler(const std::string& path, const std::string& request);
		
		// env
		const std::map<std::string, std::string>& getEnv() const;
		void initEnv();
};

#endif
