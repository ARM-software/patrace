#include "helper/shadermod.hpp"
#include "helper/eglstring.hpp"
#include "dispatch/eglproc_auto.hpp"
#include "jsoncpp/include/json/value.h"
#include "common/pa_exception.h"
#include <sstream>

ShaderMod::ShaderMod(retracer::Retracer& retracer, int programName, Json::Value& result, retracer::hmap<unsigned int>& shaderRevMap)
    : mRetracer(retracer)
    , mProgram(programName)
    , mResult(result)
    , mShaderRevMap(shaderRevMap)
    , mError(false)
    , mErrorString()
{
}

void ShaderMod::removeUnusedAttributes()
{
    const Json::Value& json = mRetracer.mFile.getJSONHeader();
    int tid = mRetracer.getCurTid();

    if (json["threads"][tid]["attributes"].isNull())
    {
        //DBG_LOG("attributes do not exist in the trace header\n");
        return;
    }

    int callCount = mRetracer.mCallCounter["glLinkProgram"];
    const Json::Value& originalAttributes = json["threads"][tid]["attributes"][callCount];

    if (originalAttributes.type() == Json::nullValue)
    {
        mError = true;
        std::stringstream ss;
        ss << "No information about this glLinkProgram call in the trace header! ";
        ss << "CallNo: " << mRetracer.GetCurCallId();
        mErrorString = ss.str();
    }

    std::vector<VertexArrayInfo> unusedAttributes = getUnusedAttributes(originalAttributes);
    removeAttributes(unusedAttributes);
}

std::vector<VertexArrayInfo> ShaderMod::getUnusedAttributes(const Json::Value& originalAttributes)
{
    std::vector<VertexArrayInfo> unusedAttributes;
    Json::Value unusedAttributeNames(Json::arrayValue);

    // TODO: O(n*m) complexity
    for (int i = 0; i < mProgram.activeAttributes; ++i)
    {
        VertexArrayInfo vai = mProgram.getActiveAttribute(i);
        bool found = false;
        for (Json::Value::iterator it = originalAttributes.begin(); it != originalAttributes.end(); ++it)
        {
            //DBG_LOG("TESTING: %s == %s\n", (*it).asString().c_str(), vai.name.c_str());
            if ((*it).asString() == vai.name)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            // I.e. active attribute found, that was not active during tracing
            unusedAttributes.push_back(vai);
            unusedAttributeNames.append(vai.name);
            //DBG_LOG("UNUSED ATTR: %s\n", vai.name.c_str());
        }
    }

    if (!unusedAttributeNames.empty())
    {
        mResult["unusedAttributes"] = unusedAttributeNames;
    }

    return unusedAttributes;
}

void ShaderMod::removeAttributes(const std::vector<VertexArrayInfo>& vai)
{
    if (vai.empty())
    {
        return;
    }

    std::vector<GLuint> shaderIds = mProgram.getAttachedShaders();

    for (std::vector<GLuint>::iterator it = shaderIds.begin(); it != shaderIds.end(); ++it)
    {
        ShaderInfo shader(*it);

        if (shader.type == GL_VERTEX_SHADER)
        {
            unsigned int originalShaderName = mShaderRevMap.RValue(*it);
            std::stringstream idSS;
            idSS << originalShaderName;
            std::string id = idSS.str();

            std::string origSource = shader.getSource();
            std::string newShader = changeAttributesToConstants(origSource, vai);
            shader.setSource(newShader);
            shader.compile();

            mResult["shaders"][id]["newCompileStatus"] = shader.compileStatus;
            if (shader.compileStatus == GL_FALSE)
            {
                mResult["shaders"][id]["newCompileLog"] = shader.getInfoLog();
            }

            mProgram.link();

            mResult["newLinkStatus"] = mProgram.linkStatus;
            if (mProgram.linkStatus == GL_FALSE)
            {
                mResult["newLinkLog"] = mProgram.getInfoLog();
            }

            break;
        }
    }
}

int ShaderMod::findShaderVersion(const std::string& source)
{
    // Find shader version
    std::string firstLine = source.substr(0, source.find('\n'));
    removeWhiteSpace(firstLine);

    int version = 100;

    std::string versionStr("#version");

    if (firstLine.substr(0, versionStr.size()) == versionStr)
    {
        // Has version declared, let's find the version number
        std::stringstream ss(firstLine.substr(versionStr.size()));
        ss >> version;
    }

    return version;
}

std::string ShaderMod::changeAttributesToConstants(const std::string& source, const std::vector<VertexArrayInfo>& attributesToRemove)
{
    // TODO: Find word only
    std::string returnSource = source;

    int version = findShaderVersion(source);
    std::string attributeKeywordStr;
    if (version < 300)
    {
        attributeKeywordStr = "attribute ";
    }
    else
    {
        attributeKeywordStr = "in ";
    }

    for (std::vector<VertexArrayInfo>::const_iterator it = attributesToRemove.begin();
         it != attributesToRemove.end();
         ++it)
    {
        bool changed = false;
        size_t searchPos = 0;
        while (!changed)
        {
            const VertexArrayInfo& vai = *it;

            size_t attributeNamePos = returnSource.find(vai.name, searchPos);
            if (attributeNamePos  == std::string::npos)
            {
                std::stringstream ss;
                ss << "Keyword " << vai.name << " at search pos " << searchPos << " not found in shader. Something is very wrong\n";
                ss << "Source:\n" << source;
                throw PA_EXCEPTION(ss.str());
            }

            size_t semicolonPos = returnSource.find(';', attributeNamePos);
            if (semicolonPos == std::string::npos)
            {
                std::stringstream ss;
                ss << "Parse error when modifying shader. Semicolon not found after " << vai.name << " at search pos " << attributeNamePos;
                ss << "Source:\n" << source;
                throw PA_EXCEPTION(ss.str());
            }

            size_t attributeKeywordPos = returnSource.rfind(attributeKeywordStr, semicolonPos);
            if (attributeKeywordPos == std::string::npos)
            {
                //DBG_LOG("Corresponding attribute keyword not found was not found for '%s', searching is continued...\n", vai.name.c_str());
                searchPos = attributeNamePos + 1;
                continue;
            }

            //DBG_LOG("POSITION FOUND: 0x%x %d %d %d\n", vai.type, attributeKeywordPos, attributeNamePos, semicolonPos);

            std::string initTemplate;

            initTemplate = "const {{type}} {{name}} = {{type}}(0.0)";
            std::string type = glTypeToString(vai.type);

            replaceAll(initTemplate, "{{name}}", vai.name);
            replaceAll(initTemplate, "{{type}}", type);

            returnSource.replace(attributeKeywordPos, semicolonPos - attributeKeywordPos, initTemplate);
            changed = true;
        }
    }

    //DBG_LOG("S:%s\n", returnSource.c_str());
    return returnSource;
}

bool ShaderMod::getError()
{
    return mError;
}

std::string ShaderMod::getErrorString()
{
    return mErrorString;
}

