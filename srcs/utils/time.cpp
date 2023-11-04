#include <ctime>
#include <iostream>

std::time_t getNowTime()
{
	return std::time(NULL);
}

bool isTimeout(std::time_t lastAccessTime, std::time_t TIMEOUT)
{
	std::time_t nowTime = getNowTime();
	if (lastAccessTime == 0)
		return false;
	if (nowTime - lastAccessTime > TIMEOUT)
		return true;
	return false;
}