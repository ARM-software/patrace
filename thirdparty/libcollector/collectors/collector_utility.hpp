#pragma once

// This module includes utilities for writing collectors

#include <vector>

#include "interface.hpp"


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

int32_t _stoi(const std::string& str);
uint64_t _stol(const std::string& str);

#else

#define _to_string(_x) std::to_string(_x)
#define _stoi(x) std::stoi(x)
#define _stol(x) std::stol(x)

#endif
