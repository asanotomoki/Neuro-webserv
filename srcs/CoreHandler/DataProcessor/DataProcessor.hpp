#ifndef DATAPROCESSOR_H
#define DATAPROCESSOR_H

#include "CoreHandler.hpp"
#include <string>

//
class DataProcessor {
	public:
    	ProcessResult processPostData(const std::string& body, const std::string& url, 
										const ServerContext& serverContext);
		static std::string getAutoIndexHtml(std::string path, const ServerContext& serverContext);
};

#endif