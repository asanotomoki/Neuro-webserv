#include "Config.hpp"
#include "SocketInterface.hpp"
#include <iostream> 

int main(int argc, char** argv)
{
    std::string configFilePath;

    configFilePath = "./conf/default.conf";
    if (argc == 2)
        configFilePath = argv[1];
    else if (argc > 2)
    {
        std::cerr << "Usage: ./NeuroSrv [config_file]" << std::endl;
        return 1;
    }
    // 設定ファイルを読み込む
    Config* config = new Config(configFilePath);

    // ソケットインターフェースを作成
    SocketInterface socketInterface(config);

    // クライアントからの接続を待機
    socketInterface.listen();

    delete config;
    return 0;
}
