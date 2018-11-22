#ifndef UTILS_H
#define UTILS_H

#include <string>

#include "jsoncpp/include/json/value.h"

void addConversionEntry(Json::Value& header, const std::string& type, const std::string& source, const Json::Value& info);

#endif
