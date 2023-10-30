#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>
#include <ctime>

// default error file
std::string default_error_page(int statusCode);


// Time
std::time_t getNowTime();
bool isTimeout(std::time_t lastAccessTime, std::time_t TIMEOUT);

#endif // UTILS_HPP