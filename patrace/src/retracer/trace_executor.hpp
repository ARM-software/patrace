#ifndef _TRACE_EXECUTOR_HPP_
#define _TRACE_EXECUTOR_HPP_

#include "jsoncpp/include/json/value.h"
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

// TODO: Remove this. It is not used for anything useful.
enum TraceExecutorErrorCode
{
            TRACE_ERROR_FILE_NOT_FOUND,
            TRACE_ERROR_INVALID_JSON,
            TRACE_ERROR_INVALID_PARAMETER,
            TRACE_ERROR_MISSING_PARAMETER,
            TRACE_ERROR_PARAMETER_OUT_OF_BOUNDS,
            TRACE_ERROR_OUT_OF_MEMORY,
            TRACE_ERROR_MEMORY_BUDGET,
            TRACE_ERROR_INITIALISING_INSTRUMENTATION,
            TRACE_ERROR_CAPTURING_INSTRUMENTATION_DATA,
            TRACE_ERROR_INCONSISTENT_TRACE_FILE,
            TRACE_ERROR_GENERIC,
            TRACE_ERROR_COUNT,
};

struct TraceErrorType
{
    TraceErrorType(TraceExecutorErrorCode code, const std::string &descr) { error_description = descr; error_code = code; }
    std::string error_description;
    TraceExecutorErrorCode error_code;
};

class TraceExecutor{
    // TraceExecutor provides an alternative way play back tracefiles using the global Retracer object.
    // More information is collected and a result file is generated
    // there can only be one TraceExector, so all its methods and variable are static

    public:
        static void initFromJson(const std::string& json_data, const std::string& trace_dir, const std::string& result_file);
        static void addError(TraceExecutorErrorCode code, const std::string &error_description = std::string());
        static void writeError(TraceExecutorErrorCode code, const std::string &error_description = std::string());
        static bool writeData(int frames, double time, long long startTime, long long endTime);
        static void clearResult();
        static void clearError();
        static bool isSetup();

        static void addDisabledButActiveAttribute(int program, const std::string& attributeName);
        static Json::Value& addProgramInfo(int program, int originalProgramName, retracer::hmap<unsigned int>& shaderRevMap);

        static const char* ErrorNames[];

    private:
        static std::string mResultFile;
        static std::vector<TraceErrorType> mErrorList;
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
