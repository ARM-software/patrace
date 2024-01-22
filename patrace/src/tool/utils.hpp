#ifndef UTILS_H
#define UTILS_H

#include <string>

#include "json/value.h"

void addConversionEntry(Json::Value& header, const std::string& type, const std::string& source, const Json::Value& info);
void addConversionEntry2(Json::Value& header, const std::string& type, std::vector<std::string>sources, const Json::Value& info);
std::string getUserName();
std::string getTimeStamp();
bool callNeedsContext(const std::string &callName);  // judge whether a call needs context or not

#endif
