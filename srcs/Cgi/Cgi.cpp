#include "Cgi.hpp"

Cgi::Cgi()
{
}

void Cgi::CgiHandler(const std::string& path, const std::string& request)
{
    int pipe_fd[2];
    pid_t pid;
    char buf[1024];
    int status;
    std::string response;
    std::string header;
    std::string body;
    std::string cgi_path = path;
    std::string cgi_request = request;
    std::string cgi_response;

    if (pipe(pipe_fd) < 0)
    {
        std::cerr << "pipe error" << std::endl;
        exit(1);
    }
    if ((pid = fork()) < 0)
    {
        std::cerr << "fork error" << std::endl;
        exit(1);
    }
    else if (pid == 0)
    {
        close(pipe_fd[0]);
        dup2(pipe_fd[1], 1);
        close(pipe_fd[1]);
        execlp("/usr/bin/env", "/usr/bin/env", "python3", cgi_path.c_str(), NULL);
		std::cerr << "execlp error" << std::endl;
        exit(1);
    }
    else
    {
        close(pipe_fd[1]);
        while (int n = read(pipe_fd[0], buf, sizeof(buf)))
        {
            if(n < 0)
            {
                std::cerr << "read error" << std::endl;
                exit(1);
            }
            cgi_response.append(buf, n);
        }
        close(pipe_fd[0]);
        waitpid(pid, &status, 0);
    }

    std::cout << "cgi_response: " << cgi_response << std::endl;
    std::cout << "cgi_response.length(): " << cgi_response.length() << std::endl;
}

Cgi::~Cgi()
{
}

// env 
void Cgi::initEnv() {
	static std::map<std::string, std::string> env;
	
	this->_env = env;
}
