#include <common/api_info.hpp>
#include <common/os.hpp>

namespace common {

ApiInfo gApiInfo;

void ApiInfo::RegisterEntries(const EntryMap& entries)
{
    if (!mIdToFptrArr)
    {
        mIdToFptrArr = new void*[MaxSigId+1];
        memset(mIdToFptrArr, 0, sizeof(void*)*(MaxSigId+1));
    }

    for (auto it = entries.begin(); it != entries.end(); ++it)
    {
        unsigned short id = NameToId(it->first.c_str());
        if (id > 0)
            mIdToFptrArr[id] = it->second;
        else {
            DBG_LOG("Unsupported function: %s\n", it->first.c_str());
        }
    }
}

}
