#ifndef PAT_SYSTEM_ENVIRONMENT_VARIABLE
#define PAT_SYSTEM_ENVIRONMENT_VARIABLE

#include "base/base.hpp"

namespace pat
{

#ifdef WIN32
const SInt8 PathSep = ';';
#else
const SInt8 PathSep = ':';
#endif

namespace EnvironmentVariable
{

extern const std::string ENVIRONMENT_VARIABLE_NAME_PATH;

bool GetVariableValue(const std::string &name, std::string &value);
bool GetVariableValue(const std::string &name, std::vector<std::string> &value);
bool GetVariableValue(const std::string &name, SInt32 &value);

bool SetVariableValue(const std::string &name, const std::string &value);
bool SetVariableValue(const std::string &name, SInt32 value);

// Look for executable under $PATH environment variable
bool SearchUnderSystemPath(const std::string &exe, std::string &abspath);

}

}

#endif // PAT_SYSTEM_ENVIRONMENT_VARIABLE
