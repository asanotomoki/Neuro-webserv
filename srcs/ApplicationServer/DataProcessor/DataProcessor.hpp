#ifndef DATAPROCESSOR_H
#define DATAPROCESSOR_H

#include "ServerContext.hpp"
#include <string>

//
class DataProcessor {
	public:
    	std::string processPostData(const std::string& postData, const ServerContext& server_context);
};

#endif