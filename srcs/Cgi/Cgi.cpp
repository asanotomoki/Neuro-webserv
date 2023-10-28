#include <string>
#include <vector>
#include <map>
#include <sys/socket.h>
#include "Cgi.hpp"

Cgi::Cgi()
{
}

//Cgi::Cgi(HttpRequest& req, ParseUrlResult &url) : 
Cgi::Cgi(HttpRequest& req) : 
    _request(req), _executable("/usr/bin/python3"){

    this->_args.push_back("/usr/bin/python3");
    this->_args.push_back("/Users/tasano/Desktop/42tokyo/new_NuroSrv/docs/cgi-bin/test_cgi.py");
    initEnv(req);
    //initEnv(req, url);
}

CgiResult Cgi::execCGI()
{
    int pipe_fd[2];
    int pipe_stdin[2];
    char **env = mapToChar(this->_env);
    char **args = vectorToChar(this->_args);
    CgiResult result;
    if (pipe(pipe_fd) < 0)
    {
        std::cerr << "pipe error" << std::endl;
        std::exit(1);
    }
    if (pipe(pipe_stdin) < 0)
    {
        std::cerr << "pipe error" << std::endl;
        std::exit(1);
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

        close(pipe_stdin[1]);
        dup2(pipe_stdin[0], 0);
        close(pipe_stdin[0]);
        
        execve(this->_executable, args, env);
        std::cerr << "execve error" << std::endl;
        std::cout << "Status: 500 Internal Server Error" << std::endl;
        std::exit(1);
    }
    else
    {
        close(pipe_fd[1]);
        close(pipe_stdin[0]);
        delete [] env;
        delete [] args;
    }
    result.fd = pipe_fd[0];
    result.inputFd = pipe_stdin[1];
    result.body = this->_request.body;
    return result;
}

Cgi::~Cgi()
{
}

// env 
void Cgi::initEnv(HttpRequest& req) {
	static std::map<std::string, std::string> env;
	env["REQUEST_METHOD"] = req.method;
	//env["PATH_INFO"] = url.pathInfo;
	env["CONTENT_LENGTH"] = req.headers["Content-Length"];
	env["CONTENT_TYPE"] = req.headers["Content-Type"];
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SERVER_PROTOCOL"] = "HTTP/1.1";
    env["SERVER_SOFTWARE"] = "webserv";
    //env["SCRIPT_NAME"] = url.fullpath;
    //env["QUERY_STRING"] = url.query;
	this->_env = env;
}

char** Cgi::mapToChar(const std::map<std::string, std::string>& map) {
    std::vector<std::string> env_strs;
    std::map<std::string, std::string>::const_iterator it;
    for (it = map.begin(); it != map.end(); ++it) {
        env_strs.push_back(it->first + "=" + it->second);
    }
    return vectorToChar(env_strs);
}

char** Cgi::vectorToChar(const std::vector<std::string>& vec) {
    char** envp = new char*[vec.size() + 1];
    for (size_t i = 0; i < vec.size(); ++i) {
        envp[i] = new char[vec[i].size() + 1];
        std::strcpy(envp[i], vec[i].c_str());
    }
    envp[vec.size()] = NULL;
    return envp;
}
