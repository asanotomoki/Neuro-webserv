#include "Config.hpp"
#include "SocketInterface.hpp"
#include <iostream> 

int main(int argc, char** argv)
{
    std::string configFilePath;

    configFilePath = "./conf/default.conf";
    if (argc == 2)
        configFilePath = argv[1];
    else if (argc > 2) {
        std::cerr << "Usage: ./webserv [config_file]" << std::endl;
        return 1;
    }
    try {
        Config* config = new Config(configFilePath);
        SocketInterface socketInterface(config);
        socketInterface.eventLoop();
        delete config;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
