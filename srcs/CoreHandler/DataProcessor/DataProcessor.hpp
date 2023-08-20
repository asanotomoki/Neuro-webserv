#ifndef DATAPROCESSOR_H
#define DATAPROCESSOR_H

#include "LocationContext.hpp"
#include "CoreHandler.hpp"
#include <string>

//
class DataProcessor {
	public:
    	ProcessResult processPostData(const std::string& postData, const LocationContext& locationContext);
};

#endif