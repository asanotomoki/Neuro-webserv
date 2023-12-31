#ifndef LOCATIONCONTEXT_HPP
#define LOCATIONCONTEXT_HPP

#include <string>
#include <map>
#include <set>

class LocationContext
{
	public:
		LocationContext();
		~LocationContext();
		void addDirective(const std::string& directive, const std::string& value,
							const std::string& filepath = "", int line_number = -1);
		void addAllowedMethod(const std::string& method);
		const std::string& getDirective(const std::string& directive) const;
		bool isAllowedMethod(const std::string& method) const;
		bool getIsCgi() const;
		std::map<std::string, std::string> getDirectives() const;
		bool hasDirective(const std::string& directive);
		void setPathPair(const std::string& alias, const std::string& path);
		void setPathPairRev(const std::string& path, const std::string& alias);
		std::pair<std::string, std::string> _pathPair;
		std::pair<std::string, std::string> _pathPairRev;
		
	private:
		std::set<std::string> _allowedMethods;
		std::map<std::string, std::string> _directives;
		// std::pair<std::string, std::string> _pathPair;
};

#endif
