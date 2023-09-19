// main.cpp
#include "Cgi.hpp"

int main() {
    Cgi cgi;

    // テストケース1: 有効なパスとリクエスト
    std::string path1 = "/Users/tasano/Desktop/42tokyo/NeuroSrv/bin-cgi/test_cgi.py";
    std::string request1 = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    cgi.CgiHandler(path1, request1);

    return 0;
}
