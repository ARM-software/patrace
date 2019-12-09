#pragma once

// This module includes utilities for writing collectors

#include <vector>
#include <sys/stat.h>

#include "interface.hpp"

bool file_exists(const std::string& name);
void splitString(const char* s, char delimiter, std::vector<std::string>& tokens);
bool DDKHasInstrCompiledIn(const std::string &keyword);
std::string getMidgardInstrConfigPath();
std::string getMidgardInstrOutputPath();


// Hack to workaround strange missing support for std::to_string in Android
#ifdef __ANDROID__
#include <string>
#include <sstream>

template <typename T>
std::string _to_string(T value)
{
    std::ostringstream os;
    os << value;
    return os.str();
}



#else

#define _to_string(_x) std::to_string(_x)

#endif

int32_t _stoi(const std::string& str);
uint64_t _stol(const std::string& str);
