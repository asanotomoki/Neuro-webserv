#include <string>
#include <vector>
#include <map>
#include <sys/socket.h>
#include "Cgi.hpp"
#include "RequestParser.hpp"
#include <iostream>
#include <sstream>

Cgi::Cgi()
{
}

std::vector<std::string> splitCgi(const std::string &s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);

    if (s.empty())
        return tokens;
    if (s == "/")
    {
        tokens.push_back("/");
        return tokens;
    }
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}

bool isCgiBlockPath(const ServerContext &server_context, std::vector<std::string> tokens)
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

ParseUrlCgiResult parseCgiBlock(std::vector<std::string> tokens, const ServerContext &server_context, ParseUrlCgiResult &result)
{
    result.directory = "/cgi-bin/";
    CGIContext cgi_context = server_context.getCGIContext();
    std::string exe = cgi_context.getDirective("extension");
    result.command = cgi_context.getDirective("command");
    size_t i = 0;
    while (i < tokens.size())
    {
        result.file += "/" + tokens[i];
        std::string ext = tokens[i].substr(tokens[i].find_last_of(".") + 1);
        if (ext == exe)
            break;
        i++;
    }
    result.fullpath = "./docs/cgi-bin" + result.file;
    result.pathInfo = "";
    // それ以降がpathinfo
    for (size_t j = i + 1; j < tokens.size(); j++)
    {
        result.pathInfo += "/" + tokens[j];
    }
    return result;
}

bool isCgiDir(std::vector<std::string> tokens)
{
    if (tokens.size() < 1)
        return false;
    std::cout << "tokens[0]: " << tokens[0] << std::endl;
    if (tokens[0] == "cgi-bin")
        return true;
    return false;
}

ParseUrlCgiResult getCgiPath(std::vector<std::string> tokens, ServerContext &serverContext, ParseUrlCgiResult &result)
{
    std::cout << "getCgiPath" << std::endl;
    result.directory = "/cgi-bin/";
    LocationContext location_context = serverContext.getLocationContext("/cgi-bin/");
    result.command = location_context.getDirective("command");
    std::cout << "command: " << result.command << std::endl;
    // ファイルが指定されていない場合
    bool isFile = false;
    size_t i = 0;
    for (; i < tokens.size(); i++)
    {
        if (tokens[i].find(".") != std::string::npos)
        {
            isFile = true;
            break;
        }
    }
    if (!isFile)
    {
        result.file = location_context.getDirective("index");
        result.fullpath = "./docs/cgi-bin/" + result.file;
        i = 1;
    }
    else
    {
        i = 0;
        for (; i < tokens.size(); i++)
        {
            result.file += "/" + tokens[i];
            if (tokens[i].find(".") != std::string::npos)
                break;
        }
        i++;
        result.fullpath = "./docs/" + result.file;
        // "//"を"/"に変換
        result.fullpath = result.fullpath.replace(result.fullpath.find("//"), 2, "/");
    }
    // その以降の要素がパスインフォ
    
    for (size_t j = i; j < tokens.size(); j++)
    {
        result.pathInfo += "/" + tokens[j];
    }
    return result;
}

void Cgi::parseUrl(std::string url, ServerContext &serverContext)
{
    std::cout << "url: " << url << std::endl;

    ParseUrlCgiResult result;
    result.statusCode = 200;
    result.autoindex = 0;
    result.errorflag = 0;
    std::vector<std::string> tokens = splitCgi(url, '?');
    if (tokens.size() == 1)
    {
        result.query = "";
    }
    else
    {
        result.query = tokens[1];
        result.query.erase(result.query.size() - 1, 1);
    }
    tokens[0].erase(0, 1);
    // cgi-bin
    std::vector<std::string> path_tokens = splitCgi(tokens[0], '/');
    std::cout << " tokens" << path_tokens[0] << std::endl;
    if (isCgiDir(path_tokens))
    {
        std::cout << "isCgiDir" << std::endl;
        getCgiPath(path_tokens, serverContext, result);
    }
    else if (isCgiBlockPath(serverContext, path_tokens))
        parseCgiBlock(path_tokens, serverContext, result);
    this->_parseUrlCgiResult = result;
}

// Cgi::Cgi(HttpRequest& req, ParseUrlCgiResult &url) :
Cgi::Cgi(HttpRequest &req, ServerContext &server_context) : _request(req)
{
    parseUrl(req.url, server_context);
    std::string exec = this->_parseUrlCgiResult.command;
    this->_executable = exec.c_str();
    std::cout << "executable: " << this->_executable << std::endl;
    std::cout << "fullpath: " << this->_parseUrlCgiResult.fullpath << std::endl;
    this->_args.push_back(this->_executable);
    this->_args.push_back(this->_parseUrlCgiResult.fullpath.c_str());
    if (pipe(this->pipe_stdin) < 0)
    {
        std::cerr << "pipe error" << std::endl;
    }
    initEnv(req);
    std::cout << "exec: " << this->_executable << std::endl;
    // initEnv(req, url);
}

int *Cgi::getPipeStdin()
{
    return this->pipe_stdin;
}

CgiResult Cgi::execCGI()
{
    int pipe_fd[2];
    char **env = mapToChar(this->_env);
    char **args = vectorToChar(this->_args);
    CgiResult result;
    std::cerr << this->_parseUrlCgiResult.command << std::endl;
    if (pipe(pipe_fd) < 0)
    {
        std::cerr << "pipe error" << std::endl;
    }
    if ((result.pid = fork()) < 0)
    {
        std::cerr << "fork error" << std::endl;
        std::exit(1);
    }
    else if (result.pid == 0)
    {
        close(pipe_fd[0]);
        dup2(pipe_fd[1], 1);
        close(pipe_fd[1]);

        close(this->pipe_stdin[1]);
        dup2(this->pipe_stdin[0], 0);
        close(this->pipe_stdin[0]);

        execve(this->_parseUrlCgiResult.command.c_str(), args, env);
        std::cout << "exec: " << this->_executable << std::endl;
        std::cerr << "execve error" << std::endl;
        std::cout << "Status: 500 Internal Server Error" << std::endl;
        perror("execve");
        std::exit(1);
    }
    else
    {
        close(pipe_fd[1]);
        close(pipe_stdin[0]);
        close(pipe_stdin[1]);
        delete[] env;
        delete[] args;
    }
    result.fd = pipe_fd[0];
    result.body = this->_request.body;
    return result;
}

Cgi::~Cgi()
{
}

// env
void Cgi::initEnv(HttpRequest &req)
{
    static std::map<std::string, std::string> env;
    env["REQUEST_METHOD"] = req.method;
    env["PATH_INFO"] = _parseUrlCgiResult.pathInfo;
    env["CONTENT_LENGTH"] = req.headers["Content-Length"];
    env["CONTENT_TYPE"] = req.headers["Content-Type"];
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SERVER_PROTOCOL"] = "HTTP/1.1";
    env["SERVER_SOFTWARE"] = "webserv";
    env["SCRIPT_NAME"] = _parseUrlCgiResult.fullpath;
    env["QUERY_STRING"] = _parseUrlCgiResult.query;
    this->_env = env;
}

char **Cgi::mapToChar(const std::map<std::string, std::string> &map)
{
    std::vector<std::string> env_strs;
    std::map<std::string, std::string>::const_iterator it;
    for (it = map.begin(); it != map.end(); ++it)
    {
        env_strs.push_back(it->first + "=" + it->second);
    }
    return vectorToChar(env_strs);
}

char **Cgi::vectorToChar(const std::vector<std::string> &vec)
{
    char **envp = new char *[vec.size() + 1];
    for (size_t i = 0; i < vec.size(); ++i)
    {
        envp[i] = new char[vec[i].size() + 1];
        std::strcpy(envp[i], vec[i].c_str());
    }
    envp[vec.size()] = NULL;
    return envp;
}
