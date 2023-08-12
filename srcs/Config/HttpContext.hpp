#ifndef HTTPCONTEXT_HPP
#define HTTPCONTEXT_HPP

#include "ServerContext.hpp"
#include <string>
#include <map>

class HttpContext
{
	public:
		HttpContext();
		~HttpContext();
		void setAccessLogFile(const std::string& accessLogFile);
		void setErrorLogFile(const std::string& errorLogFile);
		void addServerBlock(const ServerContext& server);
		void addDirective(const std::string& directive, const std::string& value,
							const std::string& filepath, int line_number);
		const std::string& getAccessLogFile() const;
		const std::string& getErrorLogFile() const;
		const std::map<std::string, std::vector<ServerContext> >& getServers() const;
		const ServerContext& getServerContext(const std::string& port, const std::string& host) const;

	private:
		std::string _accessLogFile;
		std::string _errorLogFile;
		std::map<std::string, std::vector<ServerContext> > _servers;
		std::map<std::string, std::string> _directives;
};

#endif