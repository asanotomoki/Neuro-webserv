#include "utils.hpp"

bool isCgiDir(std::vector<std::string> tokens, const ServerContext &server_context)
{
    if (tokens.size() == 0)
        return false;
    if (tokens[0] == "/")
        return false;
    LocationContext location_context;

    std::string dir = "/" + tokens[0] + "/";
    try
    {
        location_context = server_context.getLocationContext(dir);
        if (location_context.hasDirective("cgi_on") && location_context.getDirective("cgi_on") == "on")
            return true;
        else
            return false;
    }
    catch (std::exception &e)
    {
    }

    if (tokens.size() >= 1)
    {
        std::string dir = "/" + tokens[1] + "/";
        try
        {
            location_context = server_context.getLocationContext(dir);
            if (location_context.hasDirective("cgi_on") && location_context.getDirective("cgi_on") == "on")
                return true;
            else
                return false;
        }
        catch (std::exception &e)
        {
            std::cout << "not found : " << dir << std::endl;
            return false;
        }
    }
    return false;
}

bool isCgiBlockPath(std::vector<std::string> tokens, const ServerContext &server_context)
{
    if (!server_context.getIsCgi())
        return false;
    CGIContext cgi_context = server_context.getCGIContext();
    std::string exe = cgi_context.getDirective("extension");
    size_t i = 0;
    while (i < tokens.size())
    {
        // 拡張子を取得
        std::string ext = tokens[i].substr(tokens[i].find_last_of(".") + 1);
        if (ext == exe)
            return true;
        i++;
    }
    return false;
}

bool isCgi(std::vector<std::string> tokens, const ServerContext &server_context)
{
    if (tokens.size() == 0)
        return false;
    if (isCgiDir(tokens, server_context) || isCgiBlockPath(tokens, server_context))
        return true;
    return false;
}
