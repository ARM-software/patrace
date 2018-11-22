#ifndef COMMON_DATA
#define COMMON_DATA

#include <string>
#include "base/base.hpp"
#include "common/out_file.hpp"
#include "common/trace_model.hpp"

extern std::string value_type[19];
extern std::string opaque_value_type[4];
extern volatile int progress_num;
bool isDir(const std::string &path);
std::string filenameExtension(const std::string &name);
void makeProgress(int counter, int total, bool forcePrint = false);
void writeout(common::OutFile &file, common::CallTM *call);

class GlesFilePath : public std::string
{
    int id;
    void setId();
public:
    GlesFilePath(const std::string &p) : std::string(p) {
        setId();
    }
    bool operator<(const GlesFilePath &r) const {
        return id < r.id;
    }
};

#endif
