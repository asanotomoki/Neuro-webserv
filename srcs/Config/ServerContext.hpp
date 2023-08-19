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
        void setMaxBodySize(const std::string& max_body_size);
        void setErrorPage(std::string status_code, const std::string& filename);
        const std::string& getListen() const;
        const std::string& getServerName() const;
        const std::string& getMaxBodySize() const;
        const std::vector<std::string>& getAllowedMethods() const;
        const std::string& getErrorPage(std::string status_code) const; 
		void addLocationContext(const LocationContext& location);
        void addDirectives(const std::string& directive, const std::string& value,
                            const std::string& filepath, int line_number);
		const std::vector<LocationContext>& getLocations() const;
		const LocationContext& getLocationContext(const std::string& path) const;
        const LocationContext& get404LocationContext() const;
        const LocationContext& get405LocationContext() const;

    private:
        std::string _listen;
        std::string _server_name;
        std::string _max_body_size;
        std::map<std::string, std::string> _error_pages;
		std::vector<LocationContext> _locations;
        std::map<std::string, std::string> _directives;
        std::string::size_type getMaxPrefixLength(const std::string& str1, const std::string& str2) const;
        
        // error location context
        LocationContext _404LocationContext;
        LocationContext _405LocationContext;

};

#endif