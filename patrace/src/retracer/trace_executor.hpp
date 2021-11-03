#ifndef _TRACE_EXECUTOR_HPP_
#define _TRACE_EXECUTOR_HPP_

#include "json/value.h"
#include "helper/states.h"

#include <stdint.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>

// Forward declerations
namespace retracer
{
    template <class T> class hmap;
}

typedef std::set<std::string> AttributeList_t;
typedef std::unordered_map<int, AttributeList_t> ProgramAttributeListMap_t;
typedef std::vector<Json::Value> ProgramInfoList_t;

struct SingleResult {
    enum ResultType {
        RESULT_TYPE_INT,
        RESULT_TYPE_FLOAT,
    };

    union {
        int64_t intValue;
        float floatValue;
    };
    ResultType type;
    const char* name;

    SingleResult(const char* n, int64_t i) : intValue(i), type(RESULT_TYPE_INT), name(n) { }
    SingleResult(const char* n, float f) : floatValue(f), type(RESULT_TYPE_FLOAT), name(n) { }
};

struct TraceExecutorFrameResult{
    SingleResult result;
    const char* category;
    unsigned int frameNo;

    TraceExecutorFrameResult(const SingleResult& r, const char* cat, unsigned int fno)
        : result(r), category(cat), frameNo(fno) { }
};

class TraceExecutor{
    // TraceExecutor provides an alternative way play back tracefiles using the global Retracer object.
    // More information is collected and a result file is generated
    // there can only be one TraceExector, so all its methods and variable are static

    public:
        static void initFromJson(const std::string& json_data, const std::string& trace_dir, const std::string& result_file);
        static void writeError(const std::string &error_description = std::string());
        static bool writeData(Json::Value result_data_value, int frames, float duration);
        static void clearResult();
        static void clearError();
        static bool isSetup();

        static void addDisabledButActiveAttribute(int program, const std::string& attributeName);
        static Json::Value& addProgramInfo(int program, int originalProgramName, retracer::hmap<unsigned int>& shaderRevMap);

        static const char* ErrorNames[];

    private:
        static std::string mResultFile;
        static std::vector<std::string> mErrorList;
        struct ResultFile
        {
            ResultFile(const std::string& name, const std::string& path, bool shouldDelete) :
                mName(name),
                mPath(path),
                mShouldDelete(shouldDelete)
            {}

            std::string mName;
            std::string mPath;
            bool mShouldDelete;
        };
        // Map that describes vertex attributes that are active but not enabled
        // The key is the program id, and the value is a list of attributes for
        // that program that are problematic.
        static ProgramAttributeListMap_t mProgramAttributeListMap;

        static ProgramInfoList_t mProgramInfoList;

        static void overrideDefaultsWithJson(Json::Value &value);
};

#endif
