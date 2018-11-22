#include "environment_variable.hpp"
#include "path.hpp"
#include "common/os.hpp"

namespace pat
{

namespace EnvironmentVariable
{

const std::string ENVIRONMENT_VARIABLE_NAME_PATH = "PATH";

bool GetVariableValue(const std::string &name, std::string &value)
{
    const char* internal_memory = getenv(name.c_str());
    if (internal_memory)
    {
        value = std::string(internal_memory);
        return true;
    }
    else
    {
        return false;
    }
}

bool GetVariableValue(const std::string &name, std::vector<std::string> &value)
{
    std::string whole_string;
    if (GetVariableValue(name, whole_string) == false)
        return false;

    size_t begin_pos = 0;
    size_t sep_pos = whole_string.find(PathSep, begin_pos);
    while (sep_pos != std::string::npos)
    {
        const std::string part = whole_string.substr(begin_pos, sep_pos - begin_pos);
        value.push_back(part);
        begin_pos = sep_pos + 1;
        sep_pos = whole_string.find(PathSep, begin_pos);
    }
    return true;
}

bool GetVariableValue(const std::string &name, SInt32 &value)
{
    const char* internal_memory = getenv(name.c_str());
    if (internal_memory)
    {
        value = atoi(internal_memory);
        return true;
    }
    return false;
}

bool SetVariableValue(const std::string &name, const std::string &value)
{
#ifdef WIN32
    DBG_LOG("not impl");
    os::abort();
    return false;
#else
    return setenv(name.c_str(), value.c_str(), true) == 0;
#endif
}

bool SetVariableValue(const std::string &name, SInt32 value)
{
    char value_buffer[256];
    sprintf(value_buffer, "%d", value);

#ifdef WIN32
    DBG_LOG("not impl");
    os::abort();
    return false;
#else
    return setenv(name.c_str(), value_buffer, true) == 0;
#endif
}

bool SearchUnderSystemPath(const std::string &exe, std::string &abspath)
{
    std::vector<std::string> paths;
    if (GetVariableValue(ENVIRONMENT_VARIABLE_NAME_PATH, paths) == false)
        return false;

    const UInt32 num = paths.size();
    for (UInt32 i = 0; i < num; ++i)
    {
        const std::string &path = paths[i];
        if (SearchUnderPath(path, exe))
        {
            abspath = Path(path) + exe;
            return true;
        }
    }
    return false;
}

}

}
