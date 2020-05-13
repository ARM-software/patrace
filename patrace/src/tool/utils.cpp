#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <ctime>
#include <utility>
#include <sstream>

#include <common/memory.hpp>
#include <common/os.hpp>
#include <retracer/config.hpp>

#include "tool/utils.hpp"

std::string getUserName()
{
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw)
    {
        return pw->pw_name;
    }
    return "(unknown)";
}

static std::pair<bool, std::string> fileMD5(const std::string& filePath)
{
    FILE *inFile = fopen(filePath.c_str(), "rb");

    if (inFile == NULL)
    {
        DBG_LOG ("File '%s' can't be opened.\n", filePath.c_str());
        return std::make_pair(false, "");
    }

    md5_state_t mdContext;
    md5_init (&mdContext);

    int bytes;
    unsigned char data[1024];
    while ((bytes = fread(data, 1, sizeof(data), inFile)) != 0)
    {
        md5_append (&mdContext, data, bytes);
    }

    fclose (inFile);

    md5_byte_t digest[16];
    md5_finish (&mdContext, digest);

    // Make hex string
    char buf[32+1] = {0};
    for (int i = 0; i < 16; i++)
    {
        sprintf(buf + i * 2, "%02x", digest[i]);
    }

    return std::make_pair(true, buf);
}

std::string getTimeStamp()
{
    // Add timestamp (ISO format)
    char timeBuf[256];
    time_t now;
    time(&now);
    strftime(timeBuf, sizeof(timeBuf), "%FT%TZ", gmtime(&now));
    return timeBuf;
}

void addConversionEntry(Json::Value& header, const std::string& type, const std::string& source, const Json::Value& info)
{
    Json::Value conversions = Json::arrayValue;
    if (header.isMember("conversions"))
    {
        conversions = header["conversions"];
    }
    Json::Value conversion;
    conversion["version"] = PATRACE_VERSION;
    conversion["tool"] = type;
    if (!info.empty())
    {
        conversion["info"] = info;
    }
    Json::Value input;
    input["file"] = source;
    std::pair<bool, std::string> md5Result = fileMD5(source);
    if (md5Result.first)
    {
        input["md5"] = md5Result.second;
    }
    conversion["input"] = input;
    conversion["timestamp"] = getTimeStamp();
    conversion["author"] = getUserName();
    conversions.append(conversion);
    header["conversions"] = conversions;

}
