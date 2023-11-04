#ifndef CGICONTEXT_HPP
#define CGICONTEXT_HPP

#include <string>
#include <map>

// 小脳クラス
class CGIContext 
{
    public:
        CGIContext();
        ~CGIContext();
        void addDirective(const std::string& directive, const std::string& value,
                            const std::string& filepath = "", int line_number = -1);
        const std::string& getDirective(const std::string& directive) const;

    private:
        std::map<std::string, std::string> _directives;     
};

#endif