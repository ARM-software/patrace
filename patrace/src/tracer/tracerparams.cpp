#include "tracerparams.hpp"
#include "helper/states.h"

#include <common/os.hpp>

#include <string>
#include <algorithm>

static bool get_env_bool(const char* name, int fallback)
{
    const char *tmpstr = getenv(name);
    if (tmpstr && strcasecmp(tmpstr, "true") == 0)
    {
       return true;
    }
    else if (tmpstr && strcasecmp(tmpstr, "false") == 0)
    {
       return false;
    }
    return fallback;
}

TracerParams::TracerParams()
{
#ifdef ANDROID
    const char *strFilePath = "/data/apitrace/tracerparams.cfg";
#else
    const char *strFilePath = "tracerparams.cfg";
#endif

    Support2xMSAA = (get_env_bool("PATRACE_SUPPORT2XMSAA", Support2xMSAA));

    m_file.open(strFilePath);

    const char* redOnBlack = "\033[31;40m";
    const char* resetColor = "\033[00;00m";

    if (m_file.is_open())
    {
        LoadParams();
        DBG_LOG("Timestamping: %s\n", Timestamping ? "true" : "false");
        DBG_LOG("ErrorOutOnBinaryShaders: %s\n", ErrorOutOnBinaryShaders ? "true" : "false");
        DBG_LOG("MaximumAnisotropicFiltering: %d\n", MaximumAnisotropicFiltering);
        DBG_LOG("EnableErrorCheck: %s\n", EnableErrorCheck ? "true" : "false");
        DBG_LOG("EnableActiveAttribCheck: %s\n", EnableActiveAttribCheck ? "true" : "false");
        DBG_LOG("InteractiveIntercept: %s\n", InteractiveIntercept ? "true" : "false");
        DBG_LOG("FlushTraceFileEveryFrame: %s\n", FlushTraceFileEveryFrame ? "true" : "false");
        DBG_LOG("DisableBufferStorage: %s\n", DisableBufferStorage ? "true" : "false");
        DBG_LOG("RendererName: %s\n", RendererName.c_str());
        DBG_LOG("EnableRandomVersion: %s\n", EnableRandomVersion ? "true": "false");
        DBG_LOG("CloseTraceFileByTerminate: %s\n", CloseTraceFileByTerminate ? "true": "false");
        if (Support2xMSAA) DBG_LOG("Support2xMSAA: true\n");
        if (DisableErrorReporting) DBG_LOG("DisableErrorReporting: true\n");
        if (StateDumpAfterSnapshot) DBG_LOG("StateDumpAfterSnapshot: true\n");
        if (StateDumpAfterDrawCall) DBG_LOG("StateDumpAfterDrawCall: true\n");
        if (FilterSupportedExtension) {
            DBG_LOG("%sFilterSupportedExtension true%s\n",redOnBlack, resetColor);
            for (unsigned int i = 0; i < SupportedExtensions.size(); ++i) {
                DBG_LOG("Support: %s\n", SupportedExtensions[i].c_str());
            }
        } else {
            DBG_LOG("FilterSupportedExtension false\n");
        }
    } else {
        DBG_LOG("Warning: %s does not exist!\n", strFilePath);
    }
}

TracerParams::~TracerParams()
{
    if (m_file.is_open())
    {
        m_file.close();
    }
}

// trim from start
static inline std::string &ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
static inline std::string &rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
static inline std::string &trim(std::string &s)
{
    return ltrim(rtrim(s));
}

void TracerParams::LoadParams()
{
    std::string strParamName;
    std::string strParamValue;
    std::string line;
    while (std::getline(m_file, line))
    {
        if (line[0] == '#')
        {
            continue;
        }

        size_t pos = line.find_first_of(" \t");
        if (pos == std::string::npos)
        {
            continue;
        }

        strParamName = line.substr(0, pos);
        strParamValue = line.substr(pos);
        trim(strParamName);
        trim(strParamValue);

        if (strParamName.compare("EnableErrorCheck") == 0) {
            EnableErrorCheck = (strParamValue.compare("true") == 0);
        } else if (strParamName.compare("UniformBufferOffsetAlignment") == 0) {
            UniformBufferOffsetAlignment = atoi(strParamValue.c_str()); // can't use std::stoi on android :(
        } else if (strParamName.compare("ShaderStorageBufferOffsetAlignment") == 0) {
            ShaderStorageBufferOffsetAlignment = atoi(strParamValue.c_str());
        } else if (strParamName.compare("MaximumAnisotropicFiltering") == 0) {
            MaximumAnisotropicFiltering = atoi(strParamValue.c_str());
        } else if (strParamName.compare("ErrorOutOnBinaryShaders") == 0) {
            ErrorOutOnBinaryShaders = (strParamValue.compare("true") == 0);
        } else if (strParamName.compare("EnableActiveAttribCheck") == 0) {
            EnableActiveAttribCheck = (strParamValue.compare("true") == 0);
        } else if (strParamName.compare("InteractiveIntercept") == 0) {
            InteractiveIntercept = (strParamValue.compare("true") == 0);
        } else if (strParamName.compare("FilterSupportedExtension") == 0) {
            FilterSupportedExtension = (strParamValue.compare("true") == 0);
        } else if (strParamName.compare("FlushTraceFileEveryFrame") == 0) {
            FlushTraceFileEveryFrame = (strParamValue.compare("true") == 0);
        } else if (strParamName.compare("StateDumpAfterSnapshot") == 0) {
            StateDumpAfterSnapshot = (strParamValue.compare("true") == 0);
        } else if (strParamName.compare("DisableErrorReporting") == 0) {
            DisableErrorReporting = (strParamValue.compare("true") == 0);
        } else if (strParamName.compare("StateDumpAfterDrawCall") == 0) {
            StateDumpAfterDrawCall = (strParamValue.compare("true") == 0);
            stateLoggingEnabled = StateDumpAfterDrawCall;
        } else if (strParamName.compare("DisableBufferStorage") == 0) {
            DisableBufferStorage = (strParamValue.compare("true") == 0);
        } else if (strParamName.compare("RendererName") == 0) {
            RendererName = strParamValue;
        } else if(strParamName.compare("EnableRandomVersion") == 0) {
            EnableRandomVersion = (strParamValue.compare("true") == 0);
        } else if(strParamName.compare("CloseTraceFileByTerminate") == 0) {
            CloseTraceFileByTerminate = (strParamValue.compare("true") == 0);
        } else if(strParamName.compare("Support2xMSAA") == 0) {
            Support2xMSAA = (strParamValue.compare("true") == 0);
        } else if(strParamName.compare("Timestamping") == 0) {
            Timestamping = (strParamValue.compare("true") == 0);
        } else if (strParamName.compare("SupportedExtension") == 0) {
            SupportedExtensions.push_back(strParamValue);
            if (SupportedExtensionsString.length() != 0)
                SupportedExtensionsString += " ";
            SupportedExtensionsString += strParamValue;
        }
    }
}

TracerParams tracerParams;
