#ifndef LOCATIONCONTEXT_HPP
#define LOCATIONCONTEXT_HPP

#include <string>
#include <vector>
#include <map>

class LocationContext
{
	public:
		LocationContext();
		~LocationContext();
		void addDirective(const std::string& directive, const std::string& value,
							const std::string& filepath, int line_number);
		const std::string& getDirective(const std::string& directive) const;
		
	private:
		std::map<std::string, std::string> _directives;

};

#endif
