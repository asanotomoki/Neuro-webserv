#ifndef APPLICATION_SERVER_HPP
#define APPLICATION_SERVER_HPP

#include <string>

class ApplicationServer
{
public:
    ApplicationServer();
    std::string processRequest(const std::string& request);
    ~ApplicationServer();
};

#endif
