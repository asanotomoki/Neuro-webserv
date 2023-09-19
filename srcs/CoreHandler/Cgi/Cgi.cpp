#include "Cgi.hpp"

Cgi::Cgi()
{
}

Cgi::Cgi(HttpRequest& req, std::string executable, std::string path) : 
_executable(executable.c_str()), _path(path.c_str()){
    this->_args.push_back(executable);
    this->_args.push_back(path);
    initEnv(req, path);
}

std::string Cgi::CgiHandler()
{
    int pipe_fd[2];
    pid_t pid;
    char buf[1024];
    int status;
    std::string cgi_response;

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
        //execlp("/usr/bin/env", "/usr/bin/env", "python3", cgi_path.c_str(), NULL);
        execve(this->_executable, args, env);
        delete [] env;
        delete [] args;
		std::cerr << "execlp error" << std::endl;
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
            cgi_response.append(buf, n);
        }
        close(pipe_fd[0]);
        waitpid(pid, &status, 0);
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


char** Cgi::mapToChar(const std::map<std::string, std::string>& map){
    std::vector<std::string> env_strs;
    for(const auto& pair : this->_env) {
        env_strs.push_back(pair.first + "=" + pair.second);
    }

    std::vector<char*> envp;
    for(auto& str : env_strs) {
        envp.push_back(const_cast<char*>(str.c_str()));
    }
    envp.push_back(NULL);
    return envp.data();
}

char** Cgi::vectorToChar(const std::vector<std::string>& vec){
    std::vector<char*> envp;
    for(auto& str : vec) {
        envp.push_back(const_cast<char*>(str.c_str()));
    }
    envp.push_back(NULL);
    return envp.data();
}