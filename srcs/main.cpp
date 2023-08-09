#include "Config.hpp"
#include "SocketInterface.hpp"

int main(int argc, char** argv)
{
    std::string configFilePath = "./conf/default.conf"; // デフォルトの設定ファイルのパス

    if (argc > 1) {
        configFilePath = argv[1]; // コマンドライン引数から設定ファイルのパスを取得
    }

    // 設定ファイルを読み込む
    Config* config = new Config(configFilePath);

    // ソケットインターフェースを作成
    SocketInterface socketInterface(config->getPorts());

    // クライアントからの接続を待機
    socketInterface.listen();

    delete config;
    return 0;
}
