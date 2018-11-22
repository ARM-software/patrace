#ifndef SHADERMOD_HPP
#define SHADERMOD_HPP

#include "retracer/retracer.hpp"

#include <string>
#include <vector>

// Forward declerations
namespace Json
{
    class Value;
}

namespace retracer
{
    template <class T> class hmap;
}


class ShaderMod
{
public:
    ShaderMod(retracer::Retracer& retracer, int programName, Json::Value& result, retracer::hmap<unsigned int>& shaderRevMap);

    void removeUnusedAttributes();

    std::vector<VertexArrayInfo> getUnusedAttributes(const Json::Value& originalAttributes);
    void removeAttributes(const std::vector<VertexArrayInfo>& vai);

    bool getError();
    std::string getErrorString();

private:
    std::string changeAttributesToConstants(const std::string& source, const std::vector<VertexArrayInfo>& attributesToRemove);
    int findShaderVersion(const std::string& source);
    
    retracer::Retracer& mRetracer;
    ProgramInfo mProgram;

    Json::Value& mResult;
    retracer::hmap<unsigned int>& mShaderRevMap;
    bool mError;
    std::string mErrorString;
};

#endif /* _RETRACE_HPP_ */
