#ifndef DATAPROCESSOR_H
#define DATAPROCESSOR_H

#include "CoreHandler.hpp"
#include <string>

//
class DataProcessor {
	public:
    	ProcessResult processPostData(const std::string& postData);
		static std::string getAutoIndexHtml(std::string path, const ServerContext& serverContext);
};

#endif