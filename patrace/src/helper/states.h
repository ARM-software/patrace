#if !defined(STATES_H)
#define STATES_H
#include "dispatch/eglimports.hpp"
#include "dispatch/eglproc_auto.hpp"

#include <mutex>
#include <ostream>
#include <sstream>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

const bool CFG_TRACE_PLATFORM = false; // State log things that are expected to vary between platforms.

// Forward declerations
namespace Json
{
    class Value;
}

/// ugly global for higher performance
extern bool stateLoggingEnabled;

typedef std::map<std::string, int> FunctionCountMap_t;

const char* bufferName(GLenum target);
GLint getBoundBuffer(GLenum target);
GLint getCurrentProgram();

int getMaxVertexAttribs();

typedef std::vector<GLuint> IndexList_t;

class StateLogger
{
public:
    StateLogger();
    void logFunction(unsigned char tid, const std::string& functionName, unsigned callNo, unsigned drawNo);
    void logState(unsigned char tid, GLsizei count, GLenum type, const GLvoid* indices, GLsizei instancecount);
    void logState(unsigned char tid, GLint first, GLsizei count, GLsizei instancecount);
    void logState(unsigned char tid);
    void logDrawElementsIndirect(unsigned char tid, GLenum type, const void *indirect);
    void logDrawArraysIndirect(unsigned char tid, const void *indirect);
    void logComputeIndirect(unsigned char tid, GLintptr offset);
    void logCompute(unsigned char tid, GLuint x, GLuint y, GLuint z);
    void open(const std::string& fileName);
    void close();
    void flush() { mLog.flush(); }

private:
    void checkIfOpen();
    void _logState(std::stringstream& ss, unsigned char tid, GLsizei instancecount, const IndexList_t& indices, uint64_t flags);
    std::mutex mStateMutex;
    // Function call count
    std::vector<FunctionCountMap_t> mPerThreadCallCount;
    bool mLogOpen;
    std::string mFileName;
    std::ofstream mLog;
};

struct VertexArrayInfo
{
    VertexArrayInfo();

    static std::string title()
    {
        return ""
            "Binding "
            "Enabled "
            "Active "
            "Location "
            "Size "
            "Type "
            "RW "
            "M "
            "Length "
            "MOffset "
            "BOffset "
            "BufSize "
            "Buffer-MD5Sum "
            "ElemSz "
            "Stride "
            "TypeV "
            "Norm "
            "I "
            "D "
            "Name "
            ;
    }

    void updateEnabled();

    // The id of the array buffer
    GLint binding;
    // If the array buffer is enabled with
    GLint enabled;
    // If it is used in the shader
    bool active;
    // The slot in the available array buffer list
    GLint location;
    // The number of elements of array types. E.g. 4 for float[4]
    GLsizei size;
    // The type as defined in shader, e.g. GL_FLOAT_VEC4
    GLenum type;
    // The variable string, as it appears in the shader
    std::string name;

    GLint accessFlags;
    GLint isMapped;
    GLint64 mapLength;
    GLint64 mapOffset;
    intptr_t bufOffset;
    GLvoid* bufOffsetPtr;
    GLint bufferSize;
    std::string bufferMd5;
    // The number of elements (e.g. 4 for a vec4)
    GLint elemSz;
    GLint stride;
    // The element type, e.g. GL_FLOAT for a float vec4
    GLint typeV;
    GLint normalized;
    GLint integer;
    GLint divisor;

    bool operator<(const VertexArrayInfo &rhs) const { return name < rhs.name; }
};
typedef std::vector<VertexArrayInfo> VertexArray_t;


struct ProgramInfo
{
    ProgramInfo(GLuint _id, bool lazy = false);

    static std::string title()
    {
        return ""
            "ID "
            "actAttrs "
            "actUnifs "
            "actUBlks "
            "actAB "
            "attShdr "
            "delSt "
            "linkSt "
            "pbrHint "
            "TFMode "
            "TFVar "
            "actShStBlks "
            "actPrInp "
            "actPrOut "
            "actTFBuf "
            "actBufV "
            "iLogL "
            "TFVarML "
            "valStatus "
            "actAttrML "
            "actUBML "
            "actUML "
            ;
    }

    void update();
    void restore();
    void serialize(Json::Value& root);

    VertexArrayInfo getActiveAttribute(GLint i);
    std::string getInfoLog();
    std::vector<GLuint> getAttachedShaders();
    void link();

    GLint id;
    GLint activeAttributes;
    GLint activeUniforms;
    GLint activeUniformBlocks;
    GLint activeAtomicCounterBuffers;
    GLint attachedShaders;
    GLint deleteStatus;
    GLint linkStatus;
    GLint programBinaryRetrievableHint;
    GLint transformFeedbackBufferMode;
    GLint transformFeedbackVaryings;
    GLint infoLogLength;
    GLint transformFeedbackVaryingMaxLength;
    GLint validateStatus;
    GLint activeAttributeMaxLength;
    GLint activeUniformBlockMaxNameLength;
    GLint activeUniformMaxLength;
    std::vector<GLuint> shaderNames;
    GLint activeShaderStorageBlocks;
    GLint activeProgramInputs;
    GLint activeProgramOutputs;
    GLint activeTransformFeedbackBuffers;
    GLint activeBufferVariables;
};

struct ShaderInfo
{
    ShaderInfo(GLuint _id, bool lazy = false);

    void update();
    std::string getSource();
    void setSource(const std::string& source);
    std::string getInfoLog();
    void compile();

    GLint id;
    GLint type;
    GLint deleteStatus;
    GLint compileStatus;
    GLint infoLogLength;
    GLint sourceLength;
};

namespace {

inline std::ostream& operator << (std::ostream& o, const ProgramInfo& pi)
{
    std::ios::fmtflags f = std::cout.flags();
    o << std::hex;

    o << pi.id << ' ';
    o << pi.activeAttributes << ' ';
    o << pi.activeUniforms << ' ';
    o << pi.activeUniformBlocks << ' ';
    o << pi.activeAtomicCounterBuffers << ' ';
    o << pi.attachedShaders << ' ';
    o << pi.deleteStatus << ' ';
    o << pi.linkStatus << ' ';
    o << pi.programBinaryRetrievableHint << ' ';
    o << pi.transformFeedbackBufferMode << ' ';
    o << pi.transformFeedbackVaryings << ' ';
    o << pi.activeShaderStorageBlocks << ' ';
    o << pi.activeProgramInputs << ' ';
    o << pi.activeProgramOutputs << ' ';
    o << pi.activeTransformFeedbackBuffers << ' ';
    o << pi.activeBufferVariables << ' ';
    if (CFG_TRACE_PLATFORM)
    {
        o << pi.infoLogLength << ' ';
        o << pi.transformFeedbackVaryingMaxLength << ' ';
        o << pi.validateStatus << ' ';
        o << pi.activeAttributeMaxLength << ' ';
        o << pi.activeUniformBlockMaxNameLength << ' ';
        o << pi.activeUniformMaxLength << ' ';
    }
    else
    {
        o << "- ";
        o << "- ";
        o << "- ";
        o << "- ";
        o << "- ";
        o << "- ";
    }

    std::cout.flags(f);
    return o;
}

inline std::ostream& operator << (std::ostream& o, const VertexArrayInfo& vai)
{
    std::ios::fmtflags f = std::cout.flags();
    o << std::hex;

    o << vai.binding << ' ';
    o << (vai.enabled ? "Yes" : "No") << ' ';
    o << (vai.active ? "Yes" : "No") << ' ';
    if (vai.location == -1)
    {
        o << "-1 ";
    }
    else if (!CFG_TRACE_PLATFORM)
    {
        o << "- ";
    }
    else
    {
        o << vai.location << ' ';
    }
    o << vai.size << ' ';
    o << vai.type << ' ';
    o << vai.accessFlags << ' ';
    o << vai.isMapped << ' ';
    o << vai.mapLength << ' ';
    o << vai.mapOffset << ' ';
    o << vai.bufOffset << ' ';
    o << vai.bufferSize << ' ';
    o << vai.bufferMd5 << ' ';
    o << vai.elemSz << ' ';
    o << vai.stride << ' ';
    o << vai.typeV << ' ';
    o << vai.normalized << ' ';
    o << vai.integer << ' ';
    o << vai.divisor << ' ';
    o << vai.name << ' ';

    std::cout.flags(f);
    return o;
}
}

// RAII class that stores GL state upon creation and
// restores the state upon destruction
class State
{
public:
    // The constructor takes three different lists as parameters: boolsToSave,
    // intsToSave, and floatsToSave. These lists should contain GLenums.
    // The enums are looked up with the glGet[Boolean|Integer|Float]v functions.
    // See the documentation of glGet to know which enums are available.
    //
    // Please note, this class has not implemented all possible enums. Enums
    // are added to this class' set functions when needed.
    // Using an unhandled enum will result in a warning.
    //
    // If saveVertexArrays is true, the state of all vertex arrays are also
    // saved and restored.
    State(const std::vector<GLenum>& boolsToSave = std::vector<GLenum>(),
          const std::vector<GLenum>& intsToSave = std::vector<GLenum>(),
          const std::vector<GLenum>& floatsToSave = std::vector<GLenum>(),
          bool saveVertexArrays = false);

    // The saved state is restored when the object is destroyed.
    ~State();

    // Wrapper function for glGetFloatv
    static std::vector<GLfloat> getFloatv(GLenum pname);

    // Wrapper function for glGetIntegerv
    static std::vector<GLint> getIntegerv(GLenum pname);

    // Wrapper function for glGetBooleanv
    static std::vector<GLboolean> getBooleanv(GLenum pname);

    // Wrapper function for glGetTexParameteri
    static std::vector<GLint> getTexParameteri(GLenum target, GLenum pname);

    // Sets values by specifying an enum. See the documenation of glGet to know
    // which enums are available. These functions call the corresponding
    // gl function for setting the value.
    //
    // Example:
    // setInteger(GL_ACTIVE_TEXTURE. GL_TEXTURE0)
    // , results in
    // glActiveTexture(GL_TEXTURE0)
    static void setIntegerv(GLenum pname, const std::vector<GLint>& value);
    static void setInteger(GLenum pname, GLint value);
    static void setBooleanv(GLenum cap, const std::vector<GLboolean>& state);
    static void setFloatv(GLenum pname, const std::vector<GLfloat>& value);

private:
    void storeVertexArrays();
    void restoreVertexArrays();

    ProgramInfo mProgramInfo;
    VertexArray_t mVertexArrays;
    std::map<GLenum, std::vector<GLboolean> > mBoolMap;
    std::map<GLenum, std::vector<GLint> > mIntMap;
    std::map<GLenum, std::vector<GLfloat> > mFloatMap;
    bool mSaveVertexArrays;
};


inline std::vector<GLfloat> State::getFloatv(GLenum pname)
{
    std::vector<GLfloat> v(4, 0.0f);
    _glGetFloatv(pname, &v[0]);
    return v;
}

inline std::vector<GLboolean> State::getBooleanv(GLenum pname)
{
    std::vector<GLboolean> params(4, GL_FALSE);
    _glGetBooleanv(pname, &params[0]);

    if (pname == GL_DEPTH_TEST)
    {
        _glGetError(); // for a driver bug on Galaxy tablet 7.7
    }

    return params;
}

inline std::vector<GLint> State::getIntegerv(GLenum pname)
{
    std::vector<GLint> v(4, 0);
    _glGetIntegerv(pname, &v[0]);
    return v;
}

inline std::vector<GLint> State::getTexParameteri(GLenum target, GLenum pname)
{
    std::vector<GLint> v(4, 0);
    _glGetTexParameteriv(target, pname, &v[0]);
    return v;
}

inline void State::setInteger(GLenum pname, GLint value)
{
    setIntegerv(pname, {value});
}

inline void State::setBooleanv(GLenum cap, const std::vector<GLboolean>& state)
{
    if (state[0])
    {
        glEnable(cap);
    }
    else
    {
        glDisable(cap);
    }
}

#endif  /* STATES_H */
