#ifndef SERVERCONTEXT_HPP
#define SERVERCONTEXT_HPP

#include "LocationContext.hpp"
#include <map>
#include <string>
#include <vector>

class ServerContext
{
    public:
        ServerContext();
        ~ServerContext();
        void setListen(const std::string& listen);
        void setServerName(const std::string& server_name);
        const std::string& getListen() const;
        const std::string& getServerName() const;
		void addLocationBlock(const LocationContext& location);
        void addDirectives(const std::string& directive, const std::string& value,
                            const std::string& filepath, int line_number);
		const std::vector<LocationContext>& getLocations() const;
		const LocationContext& getLocationContext(const std::string& path) const;

    private:
        std::string _listen;
        std::string _server_name;
		std::vector<LocationContext> _locations;
        std::map<std::string, std::string> _directives;
		std::string::size_type getMaxPrefixLength(const std::string &str1, const std::string &str2) const;
};

#endif