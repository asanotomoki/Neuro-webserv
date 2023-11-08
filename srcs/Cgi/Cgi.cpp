#include "Cgi.hpp"
#include "RequestParser.hpp"
#include "utils.hpp"
#include <string>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <sys/stat.h>
#include <iostream>
#include <sstream>

Cgi::Cgi()
{
}

ParseUrlCgiResult Cgi::parseCgiBlock(std::vector<std::string> tokens, const ServerContext &server_context, ParseUrlCgiResult &result)
{
    result.directory = tokens[0] + "/";

    CGIContext cgi_context = server_context.getCGIContext();
    std::string exe = cgi_context.getDirective("extension");
    try
    {
        result.command = cgi_context.getDirective("command");
    }
    catch (std::exception &e)
    {
        // ない場合は実行ファイル
        result.errorflag = 1;
    }
    std::string alias = cgi_context.getDirective("alias");
    size_t i = 0;
    while (i < tokens.size())
    {
        result.file += "/" + tokens[i];
        std::string ext = tokens[i].substr(tokens[i].find_last_of(".") + 1);
        if (ext == exe)
            break;
        i++;
    }
    result.fullpath = alias + result.file;
    result.pathInfo = "";
    // それ以降がpathinfo
    for (size_t j = i + 1; j < tokens.size(); j++)
    {
        result.pathInfo += "/" + tokens[j];
    }
    return result;
}

ParseUrlCgiResult Cgi::getCgiPath(std::vector<std::string> tokens, ServerContext &serverContext, ParseUrlCgiResult &result)
{
    if (tokens.size() < 1)
        return result;
    result.directory = "/" + tokens[0] + "/";
    LocationContext location_context = serverContext.getLocationContext(result.directory);
    std::cout << "method : " << _method << std::endl;
    if (!location_context.isAllowedMethod(_method))
    {
        result.statusCode = 405;
        return result;
    }
    std::string alias = location_context.getDirective("alias");
    try
    {
        result.command = location_context.getDirective("command");
    }
    catch (std::exception &e)
    {
        // ない場合は実行ファイル
        result.errorflag = 1;
    }
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
        try {
            result.file = location_context.getDirective("index");
        } catch (std::exception& e) {
            result.file = "";
        }
        result.fullpath += location_context.getDirective("alias") + "/";
        result.fullpath += result.file;
        i = 1;
    }
    else
    {
        i = 1;
        for (; i < tokens.size(); i++)
        {
            result.file += "/" + tokens[i];
            if (tokens[i].find(".") != std::string::npos)
                break;
        }
        i++;
        result.fullpath = alias + result.file;
        result.fullpath = result.fullpath.replace(result.fullpath.find("//"), 2, "/");
    }
    // その以降の要素がパスインフォ

    for (size_t j = i; j < tokens.size(); j++)
    {
        result.pathInfo += "/" + tokens[j];
    }
    if (result.errorflag == 1)
    {
        result.command = result.fullpath;
    }
    return result;
}

void Cgi::parseUrl(std::string url, ServerContext &serverContext)
{
    ParseUrlCgiResult result;
    result.statusCode = 200;
    result.autoindex = 0;
    result.errorflag = 0;
    std::vector<std::string> tokens = split(url, '?');
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
    std::vector<std::string> path_tokens = split(tokens[0], '/');
    if (isCgiDir(path_tokens, serverContext))
        getCgiPath(path_tokens, serverContext, result);
    else if (isCgiBlockPath(path_tokens, serverContext))
        parseCgiBlock(path_tokens, serverContext, result);
    this->_parseUrlCgiResult = result;
}

// Cgi::Cgi(HttpRequest& req, ParseUrlCgiResult &url) :
Cgi::Cgi(HttpRequest &req, ServerContext &server_context) : _request(req), _method(req.method)
{

    parseUrl(req.url, server_context);
    std::string exec = this->_parseUrlCgiResult.command;
    this->_executable = exec.c_str();
    this->_args.push_back(this->_executable);
    this->_args.push_back(this->_parseUrlCgiResult.fullpath.c_str());
    if (pipe(this->pipe_stdin) < 0)
    {
        std::cerr << "pipe error" << std::endl;
    }
    initEnv(req);
}

int *Cgi::getPipeStdin()
{
    return this->pipe_stdin;
}

bool validatePath(std::string &path)
{
    struct stat buffer;
    stat(path.c_str(), &buffer);
    // isFile
    if (S_ISREG(buffer.st_mode))
        return true;
    if (access(path.c_str(), X_OK) == -1)
    {
        std::cerr << "access error Not X_OK  : " << path << std::endl;
    }
    else if (access(path.c_str(), F_OK) == -1) // ない場合は404
    {
        std::cerr << "access error" << std::endl;
        // result.statusCode = 404;
        // std::exit(1);
    }
    return false;
}

void Cgi::execCGI(CgiResult &result)
{
    int pipe_fd[2];
    char **env = mapToChar(this->_env);
    char **args = vectorToChar(this->_args);
    if (this->_parseUrlCgiResult.statusCode == 405)
    {
        result.statusCode = 405;
        return;
    }
    if (!validatePath(_parseUrlCgiResult.fullpath))
    {
        result.statusCode = 404;
        return;
    }

    if (pipe(pipe_fd) < 0)
    {
        std::cerr << "pipe error" << std::endl;
    }
    pid_t pid = fork();
    if (pid < 0)
    {
        std::cerr << "fork error" << std::endl;
        std::exit(1);
    }
    else if (pid == 0)
    {
        close(pipe_fd[0]);
        dup2(pipe_fd[1], 1);
        close(pipe_fd[1]);

        close(this->pipe_stdin[1]);
        dup2(this->pipe_stdin[0], 0);
        close(this->pipe_stdin[0]);

        execve(this->_parseUrlCgiResult.command.c_str(), args, env);
        std::cerr << "execve error" << std::endl;
        result.statusCode = 500;
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
    result.pid = pid;
    return;
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
