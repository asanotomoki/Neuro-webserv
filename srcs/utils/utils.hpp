#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>
#include <ctime>

// default error file
std::string default_error_page(int statusCode);


// Time
std::time_t getNowTime();
bool isTimeout(std::time_t lastAccessTime, std::time_t TIMEOUT);
std::string error_page(int status_code, std::string page);
#endif // UTILS_HPP