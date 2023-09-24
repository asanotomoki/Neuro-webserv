#ifndef SERVERCONTEXT_HPP
#define SERVERCONTEXT_HPP

#include "LocationContext.hpp"
#include "CGIContext.hpp"
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
        void setIsCgi(bool is_cgi);
        void setErrorPages();
        void setErrorPage(std::string status_code, const std::string& filename);
        const std::string& getListen() const;
        const std::string& getServerName() const;
        const std::string& getMaxBodySize() const;
        bool getIsCgi() const;
        //const std::vector<std::string>& getAllowedMethods() const;
        const std::string& getErrorPage(std::string status_code) const; 
		void addLocationContext(const LocationContext& location);
        void addCGIContext(const CGIContext& cgi);
        void addDirectives(const std::string& directive, const std::string& value,
                            const std::string& filepath, int line_number);
		const std::vector<LocationContext>& getLocations() const;
		const LocationContext& getLocationContext(const std::string& path) const;
        const CGIContext& getCGIContext() const;
        const LocationContext& get403LocationContext() const;
        const LocationContext& get404LocationContext() const;
        const LocationContext& get405LocationContext() const;
        const LocationContext& get501LocationContext() const; 
        const std::string& getReturnPath(const std::string& path) const;

    private:
        std::string _listen;
        std::string _serverName;
        std::string _maxBodySize;
        bool _is_Cgi;
        std::map<std::string, std::string> _errorPages;
		std::vector<LocationContext> _locations;
        CGIContext _cgi;
        std::map<std::string, std::string> _directives;
        std::string::size_type getMaxPrefixLength(const std::string& str1, const std::string& str2) const;
        // error location context
        LocationContext _403LocationContext; 
        LocationContext _404LocationContext;
        LocationContext _405LocationContext;
        LocationContext _501LocationContext;
};

#endif