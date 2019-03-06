#pragma once

// This module includes utilities for writing collectors

#include "interface.hpp"

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
