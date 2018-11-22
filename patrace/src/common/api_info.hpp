#ifndef _COMMON_API_INFO_HPP_
#define _COMMON_API_INFO_HPP_

#include <string.h>
#include <stdint.h>
#include <map>
#include <string>

namespace common {

typedef std::map<std::string, void*> EntryMap;

class ApiInfo
{
public:
    ApiInfo():mIdToFptrArr(NULL) {}

    ~ApiInfo()
    {
        delete [] mIdToFptrArr;
        mIdToFptrArr = NULL;
    }

    int NameToLen(const char* name)
    {
        unsigned short id = NameToId(name);
        if (id)
            return IdToLenArr[id];

        return 0;
    }

    void* NameToFptr(const char* name)
    {
        if (!mIdToFptrArr)
            return NULL;

        unsigned short id = NameToId(name);
        if (id)
            return mIdToFptrArr[id];

        return NULL;
    }

    void RegisterEntries(const EntryMap& entries);

    // These are determined when the source code is generated
    // How many gles/egl functions are supported
    // Their names
    // Their serialized lengths
    static unsigned short   MaxSigId;
    static const char*      IdToNameArr[];
    static int              IdToLenArr[];

    inline unsigned short NameToId(const char* name)
    {
        if (name == NULL)
            return 0;

        for (unsigned short id = 1; id <= MaxSigId; ++id)
        {
            if (!IdToNameArr[id])
                continue;
            if (strcmp(IdToNameArr[id], name) == 0)
                return id;
        }

        return 0;
    }

private:
    void** mIdToFptrArr;
};

extern ApiInfo gApiInfo;

}

#endif
