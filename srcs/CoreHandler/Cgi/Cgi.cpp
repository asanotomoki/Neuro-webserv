#include "Cgi.hpp"
#include <cstring> // for std::strcpy
#include <string>
#include <vector>
#include <map>

Cgi::Cgi()
{
}

Cgi::Cgi(HttpRequest& req, std::string executable, std::string path) : 
_executable(executable.c_str()), _path(path.c_str()){
    this->_args.push_back(executable);
    this->_args.push_back(path);
    initEnv(req, path);
}

CgiResponse Cgi::CgiHandler()
{
    int pipe_fd[2];
    pid_t pid;
    char buf[1024];
    int status;
    CgiResponse cgi_response;
//   return ("Content-Type: text/html\n\n<html><body>\n<p>Hello, world!</p>\n</body></html>");
    if (pipe(pipe_fd) < 0)
    {
        std::cerr << "pipe error" << std::endl;
        std::exit(1);
    }
    if ((pid = fork()) < 0)
    {
        std::cerr << "fork error" << std::endl;
        std::exit(1);
    }
    else if (pid == 0)
    {
        close(pipe_fd[0]);
        dup2(pipe_fd[1], 1);
        close(pipe_fd[1]);
        char **env = mapToChar(this->_env);
        char **args = vectorToChar(this->_args);
        execve(this->_executable, args, env);
        delete [] env;
        delete [] args;
		std::cerr << "execve error" << std::endl;
        cgi_response.status = 500;
        cgi_response.message = "Internal Server Error";
        std::exit(1);
    }
    else
    {
        close(pipe_fd[1]);
        while (int n = read(pipe_fd[0], buf, sizeof(buf)))
        {
            if(n < 0)
            {
                std::cerr << "read error" << std::endl;
                std::exit(1);
            }
            cgi_response.message.append(buf, n);
        }
        close(pipe_fd[0]);
        waitpid(pid, &status, 0);
    }
    if (status != 0)
    {
        cgi_response.status = 500;
        cgi_response.message = "Internal Server Error";
    }
    else
    {
        cgi_response.status = 200;
    }

    return cgi_response;
}

Cgi::~Cgi()
{
}

// env 
void Cgi::initEnv(HttpRequest& req, std::string path) {
	static std::map<std::string, std::string> env;
	env["REQUEST_METHOD"] = req.method;
	env["PATH_INFO"] = path;
	env["CONTENT_LENGTH"] = req.headers["Content-Length"];
	env["CONTENT_TYPE"] = req.headers["Content-Type"];
	this->_env = env;
}

// char** Cgi::mapToChar(const std::map<std::string, std::string>& map) {
//     std::vector<std::string> env_strs;
//     for (const auto& pair : map) { 
//         env_strs.push_back(pair.first + "=" + pair.second);
//     }
//     return vectorToChar(env_strs);
// }

// char** Cgi::vectorToChar(const std::vector<std::string>& vec) {
//     char** envp = new char*[vec.size() + 1]; 
//     size_t i = 0;
//     for (const auto& str : vec) {
//         envp[i] = new char[str.size() + 1];
//         std::strcpy(envp[i], str.c_str());
//         i++;
//     }
//     envp[vec.size()] = NULL;
//     return envp;
// }

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
