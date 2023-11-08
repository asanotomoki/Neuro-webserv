#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>
#include <ctime>
#include <vector>
#include "ServerContext.hpp"

// default error file
std::string default_error_page(int statusCode);


// Time
std::time_t getNowTime();
bool isTimeout(std::time_t lastAccessTime, std::time_t TIMEOUT);
std::string error_page(int status_code, std::string page);
bool isCgi(std::vector<std::string>tokens, const ServerContext &server_context);
bool isCgiBlockPath(std::vector<std::string> tokens, const ServerContext &server_context);
bool isCgiDir(std::vector<std::string> tokens, const ServerContext &server_context);

std::vector<std::string> split(const std::string &s, char delimiter);
#endif // UTILS_HPP