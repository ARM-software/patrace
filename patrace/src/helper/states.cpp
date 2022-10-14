#include "states.h"
#include "helper/eglsize.hpp"
#include "helper/eglstring.hpp"
#include "common/os.hpp"
#include "common/memory.hpp"
#include "common/gl_utility.hpp"
#include "common/gl_extension_supported.hpp"
#include "json/value.h"
#include "specs/khronos_enums.hpp"

#ifdef RETRACE
#include "retracer/retracer.hpp"
#define callNumber (unsigned)retracer::gRetracer.GetCurCallId()
#define frameNumber (unsigned)retracer::gRetracer.GetCurFrameId()
#else
#include "tracer/egltrace.hpp"
#define callNumber (unsigned)gTraceOut->callNo
#define frameNumber (unsigned)gTraceOut->frameNo
#endif
#define STATE_LOG(...) do { DBG_LOG("ERROR (frame %u, call %u): ", frameNumber, callNumber); DBG_LOG(__VA_ARGS__); } while(0)

#include <sstream>
#include <vector>
#include <algorithm>

/// Set this to >= 0 and a call range to only state log dump particular calls
const long first_call = -1;
const long last_call = -1;
static inline bool call_in_range()
{
    return first_call < 0 || (callNumber <= last_call && callNumber >= first_call);
}

/// Enable this to print out almost human readable headers for statelogging lines.
const bool PRINT_TITLES = false;

/// Enable this to output call numbers. These will vary between tracing and retracing,
/// but should be identical between two retrace cases.
const bool PRINT_CALLNO = true;

// Also see CFG_TRACE_PLATFORM in the header

bool stateLoggingEnabled = false;

enum stateCfg {
	CFG_HASH_VBO = 1,	// Create MD5 sums of VBO content
	CFG_HASH_CS = 2,	// Create MD5 sums of client side content
	CFG_HASH_UNIFORM = 4,	// Create MD5 sums of uniform buffer content
	CFG_TRACE_CS = 8,	// Log the actual content of the client side buffer
	CFG_TRACE_CS_FL = 16,   // Log only the first and last data element in a CSB.
	CFG_TRACE_DATA_VBO = 32, // Log the actual content of an VBO
	CFG_TRACE_DATA_IBO = 64, // Log the actual content of index buffers
	CFG_TRACE_DATA_UB = 128, // Log the actual content of (uniform) buffer blocks
};

//const uint64_t CONFIG = CFG_HASH_VBO | CFG_HASH_CS | CFG_HASH_UNIFORM;
const uint64_t CONFIG_FLAGS = 0;

static std::ostream& line(std::ostream& os, const std::string& prefix, unsigned char tid)
{
    os << "@" << prefix << ": [" << static_cast<int>(tid) << "] ";
    return os;
}

int getMaxVertexAttribs()
{
    static GLint maxAttribs = 0;
    if (maxAttribs == 0)
    {
        _glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);
    }
    return maxAttribs;
}

GLint getBoundBuffer(GLenum target)
{
    GLint bufferId = 0;
    switch (target)
    {
    case PA_GL_TEXTURE_BUFFER_EXT:
        _glGetIntegerv(PA_GL_TEXTURE_BUFFER_BINDING_EXT, &bufferId);
        break;
    case GL_SHADER_STORAGE_BUFFER:
        _glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, &bufferId);
        break;
    case GL_DRAW_INDIRECT_BUFFER:
        _glGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, &bufferId);
        break;
    case GL_DISPATCH_INDIRECT_BUFFER:
        _glGetIntegerv(GL_DISPATCH_INDIRECT_BUFFER_BINDING, &bufferId);
        break;
    case GL_ARRAY_BUFFER:
        _glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bufferId);
        break;
    case GL_COPY_READ_BUFFER:
        _glGetIntegerv(GL_COPY_READ_BUFFER_BINDING, &bufferId);
        break;
    case GL_COPY_WRITE_BUFFER:
        _glGetIntegerv(GL_COPY_WRITE_BUFFER_BINDING, &bufferId);
        break;
    case GL_ELEMENT_ARRAY_BUFFER:
        _glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &bufferId);
        break;
    case GL_PIXEL_PACK_BUFFER:
        _glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &bufferId);
        break;
    case GL_PIXEL_UNPACK_BUFFER:
        _glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &bufferId);
        break;
    case GL_TRANSFORM_FEEDBACK_BUFFER:
        _glGetIntegerv(GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, &bufferId);
        break;
    case GL_UNIFORM_BUFFER:
        _glGetIntegerv(GL_UNIFORM_BUFFER_BINDING, &bufferId);
        break;
    case GL_ATOMIC_COUNTER_BUFFER:
        _glGetIntegerv(GL_ATOMIC_COUNTER_BUFFER_BINDING, &bufferId);
        break;
    default:
        STATE_LOG("Invalid target: 0x%x\n", target);
    }

    return bufferId;
}

const char* bufferName(GLenum target)
{
    switch (target)
    {
    case PA_GL_TEXTURE_BUFFER_EXT: return "GL_TEXTURE_BUFFER_EXT";
    case GL_SHADER_STORAGE_BUFFER: return "GL_SHADER_STORAGE_BUFFER";
    case GL_DRAW_INDIRECT_BUFFER: return "GL_DRAW_INDIRECT_BUFFER";
    case GL_DISPATCH_INDIRECT_BUFFER: return "GL_DISPATCH_INDIRECT_BUFFER";
    case GL_ARRAY_BUFFER: return "GL_ARRAY_BUFFER";
    case GL_COPY_READ_BUFFER: return "GL_COPY_READ_BUFFER";
    case GL_COPY_WRITE_BUFFER: return "GL_COPY_WRITE_BUFFER";
    case GL_ELEMENT_ARRAY_BUFFER: return "GL_ELEMENT_ARRAY_BUFFER";
    case GL_PIXEL_PACK_BUFFER: return "GL_PIXEL_PACK_BUFFER";
    case GL_PIXEL_UNPACK_BUFFER: return "GL_PIXEL_UNPACK_BUFFER";
    case GL_TRANSFORM_FEEDBACK_BUFFER: return "GL_TRANSFORM_FEEDBACK_BUFFER";
    case GL_UNIFORM_BUFFER: return "GL_UNIFORM_BUFFER";
    case GL_ATOMIC_COUNTER_BUFFER: return "GL_ATOMIC_COUNTER_BUFFER";
    }
    return "UNKNOWN BUFFER TYPE";
}

GLint getCurrentProgram()
{
    GLint program = 0;
    _glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    return program;
}

template<class T>
struct Vec
{
    Vec(T* _buffer, int _size)
     : buffer(_buffer)
     , size(_size)
    {}

    std::ostream& toStream (std::ostream& o) const
    {
        for (int i = 0; i < size; ++i) {
            o << buffer[i];
            if (i < size - 1)
            {
                o << ", ";
            }
        }
        return o;
    }

    T* buffer;
    int size;
};

std::ostream& operator << (std::ostream& o, const Vec<GLfloat>& v)
{
    return v.toStream(o);
}

std::ostream& operator << (std::ostream& o, const Vec<GLubyte>& v)
{
    for (int i = 0; i < v.size; ++i)
    {
        o << static_cast<int>(v.buffer[i]);
        if (i < v.size - 1)
        {
            o << ", ";
        }
    }
    return o;
}

std::ostream& operator << (std::ostream& o, const Vec<GLbyte>& v)
{
    std::ios::fmtflags f = std::cout.flags();
    o << std::dec;

    for (int i = 0; i < v.size; ++i)
    {
        o << static_cast<int>(v.buffer[i]);
        if (i < v.size - 1)
        {
            o << ", ";
        }
    }
    std::cout.flags(f);
    return o;
}

ProgramInfo::ProgramInfo(GLuint _id, bool lazy)
    : id(_id)
    , activeAttributes(0)
    , activeUniforms(0)
    , activeUniformBlocks(0)
    , activeAtomicCounterBuffers(0)
    , attachedShaders(0)
    , deleteStatus(0)
    , linkStatus(GL_FALSE)
    , programBinaryRetrievableHint(0)
    , transformFeedbackBufferMode(0)
    , transformFeedbackVaryings(0)
    , infoLogLength(0)
    , transformFeedbackVaryingMaxLength(0)
    , validateStatus(0)
    , activeAttributeMaxLength(0)
    , activeUniformBlockMaxNameLength(0)
    , activeUniformMaxLength(0)
    , shaderNames()
    , activeShaderStorageBlocks(0)
    , activeProgramInputs(0)
    , activeProgramOutputs(0)
    , activeTransformFeedbackBuffers(0)
    , activeBufferVariables(0)
{
    if (!lazy && _id != 0)
    {
        update();
    }
}

void ProgramInfo::update()
{
    _glGetProgramiv(id, GL_LINK_STATUS                          , &linkStatus);
    _glGetProgramiv(id, GL_VALIDATE_STATUS                      , &validateStatus);
    _glGetProgramiv(id, GL_INFO_LOG_LENGTH                      , &infoLogLength);
    if (linkStatus == GL_FALSE)
    {
        return; // at this point, the rest are invalid
    }
    _glGetProgramiv(id, GL_ACTIVE_ATTRIBUTES                    , &activeAttributes);
    _glGetProgramiv(id, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH          , &activeAttributeMaxLength);
    _glGetProgramiv(id, GL_ACTIVE_UNIFORMS                      , &activeUniforms);
    _glGetProgramiv(id, GL_ACTIVE_UNIFORM_BLOCKS                , &activeUniformBlocks);
    _glGetProgramiv(id, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH , &activeUniformBlockMaxNameLength);
    _glGetProgramiv(id, GL_ACTIVE_UNIFORM_MAX_LENGTH            , &activeUniformMaxLength);
    _glGetProgramiv(id, GL_ACTIVE_ATOMIC_COUNTER_BUFFERS        , &activeAtomicCounterBuffers);
    _glGetProgramiv(id, GL_ATTACHED_SHADERS                     , &attachedShaders);
    _glGetProgramiv(id, GL_DELETE_STATUS                        , &deleteStatus);
    _glGetProgramiv(id, GL_PROGRAM_BINARY_RETRIEVABLE_HINT      , &programBinaryRetrievableHint);
    _glGetProgramiv(id, GL_TRANSFORM_FEEDBACK_BUFFER_MODE       , &transformFeedbackBufferMode);
    _glGetProgramiv(id, GL_TRANSFORM_FEEDBACK_VARYINGS          , &transformFeedbackVaryings);
    _glGetProgramiv(id, GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH, &transformFeedbackVaryingMaxLength);
    if (gGlesFeatures.isProgramInterfaceSupported())
    {
        _glGetProgramInterfaceiv(id, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &activeShaderStorageBlocks);
        _glGetProgramInterfaceiv(id, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &activeProgramInputs);
        _glGetProgramInterfaceiv(id, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &activeProgramOutputs);
        _glGetProgramInterfaceiv(id, GL_TRANSFORM_FEEDBACK_VARYING, GL_ACTIVE_RESOURCES, &activeTransformFeedbackBuffers);
        _glGetProgramInterfaceiv(id, GL_BUFFER_VARIABLE, GL_ACTIVE_RESOURCES, &activeBufferVariables);
    }

    GLuint sn[256];
    _glGetAttachedShaders(id, 256, 0, sn);
    for (int i = 0; i < attachedShaders; ++i)
    {
        shaderNames.push_back(sn[i]);
    }
}

void ProgramInfo::restore()
{
    _glUseProgram(id);
}

void ProgramInfo::serialize(Json::Value& root)
{
    root["id"] =                                id;
    root["activeAttributes"] =                  activeAttributes;
    root["activeAttributeMaxLength"] =          activeAttributeMaxLength;
    root["activeUniforms"] =                    activeUniforms;
    root["activeUniformBlocks"] =               activeUniformBlocks;
    root["activeUniformBlockMaxNameLength"] =   activeUniformBlockMaxNameLength;
    root["activeUniformMaxLength"] =            activeUniformMaxLength;
    root["activeAtomicCounterBuffers"] =        activeAtomicCounterBuffers;
    root["attachedShaders"] =                   attachedShaders;
    root["deleteStatus"] =                      deleteStatus;
    root["infoLogLength"] =                     infoLogLength;
    root["linkStatus"] =                        linkStatus;
    root["programBinaryRetrievableHint"] =      programBinaryRetrievableHint;
    root["transformFeedbackBufferMode"] =       transformFeedbackBufferMode;
    root["transformFeedbackVaryings"] =         transformFeedbackVaryings;
    root["transformFeedbackVaryingMaxLength"] = transformFeedbackVaryingMaxLength;
    root["validateStatus"] =                    validateStatus;
    root["activeShaderStorageBlocks"] =         activeShaderStorageBlocks;
    root["activeProgramInputs"] =               activeProgramInputs;
    root["activeProgramOutputs"] =              activeProgramOutputs;
    root["activeTransformFeedbackBuffers"] =    activeTransformFeedbackBuffers;
}

std::vector<GLuint> ProgramInfo::getAttachedShaders()
{
    std::vector<GLuint> ret(attachedShaders);
    _glGetAttachedShaders(id, attachedShaders, 0, &ret[0]);
    return ret;
}

VertexArrayInfo ProgramInfo::getActiveAttribute(GLint i)
{
    VertexArrayInfo vai;

    GLchar name[128];
    GLsizei length = 0;
    _glGetActiveAttrib(id, i, 128, &length, &vai.size, &vai.type, name);

    vai.location = _glGetAttribLocation(id, name);
    vai.name = length ? name : "<built-in>";
    vai.active = true;

    return vai;
}

std::string ProgramInfo::getInfoLog()
{
    std::vector<char> log(infoLogLength);
    _glGetProgramInfoLog(id, infoLogLength, 0, &log[0]);
    return std::string(&log[0]);
}

void ProgramInfo::link()
{
    _glLinkProgram(id);
    update();
}

ShaderInfo::ShaderInfo(GLuint _id, bool lazy)
    : id(_id)
    , type()
    , deleteStatus()
    , compileStatus()
    , infoLogLength()
    , sourceLength()
{
    if (!lazy)
    {
        update();
    }
}

void ShaderInfo::update()
{
    _glGetShaderiv(id, GL_SHADER_TYPE, &type);
    _glGetShaderiv(id, GL_DELETE_STATUS, &deleteStatus);
    _glGetShaderiv(id, GL_COMPILE_STATUS, &compileStatus);
    _glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
    _glGetShaderiv(id, GL_SHADER_SOURCE_LENGTH, &sourceLength);
}

std::string ShaderInfo::getSource()
{
    std::vector<char> src(sourceLength);
    _glGetShaderSource(id, sourceLength, 0, &src[0]);
    return std::string(&src[0]);
}

void ShaderInfo::setSource(const std::string& source)
{
    const GLchar* s = source.c_str();
    _glShaderSource(id, 1, &s, 0);
}

std::string ShaderInfo::getInfoLog()
{
    std::vector<char> log(infoLogLength);
    _glGetShaderInfoLog(id, infoLogLength, 0, &log[0]);
    return std::string(&log[0]);
}

void ShaderInfo::compile()
{
    _glCompileShader(id);
    update();
}

struct UniformInfo
{
    bool operator<(const UniformInfo &rhs) const { return name < rhs.name; }

    UniformInfo(GLuint program, GLuint index_)
     : index(index_)
     , location(0)
     , block(-1)
     , offset(0)
     , arrayStride(0)
     , matrixStride(0)
     , isRowMajor(0)
     , size(0)
     , type(0)
     , name()
     , value()
    {
        GLchar cname[128];
        _glGetActiveUniform(program, index_, 128, 0, &size, &type, cname);
        name = cname;

        location = _glGetUniformLocation(program, name.c_str());
        _glGetActiveUniformsiv(program, 1, &index, GL_UNIFORM_BLOCK_INDEX, &block);
        _glGetActiveUniformsiv(program, 1, &index, GL_UNIFORM_OFFSET, &offset);
        _glGetActiveUniformsiv(program, 1, &index, GL_UNIFORM_ARRAY_STRIDE, &arrayStride);
        _glGetActiveUniformsiv(program, 1, &index, GL_UNIFORM_MATRIX_STRIDE, &matrixStride);
        _glGetActiveUniformsiv(program, 1, &index, GL_UNIFORM_IS_ROW_MAJOR, &isRowMajor);

        std::stringstream ss;
        GLenum elemType;
        GLint numElems;
        _gl_uniform_size(type, elemType, numElems);
        if (location >= 0)
        {
            switch (elemType)
            {
            case GL_FLOAT:
                {
                    GLfloat v[16] = { 0.0f };
                    _glGetUniformfv(program, location, v);
                    for (int i = 0; i < numElems; ++i)
                    {
                        ss << v[i] << ",";
                    }
                }
                break;
            case GL_INT:
            case GL_BOOL:
                {
                    GLint v[16] = { 0 };
                    _glGetUniformiv(program, location, v);
                    for (int i = 0; i < numElems; ++i)
                    {
                        ss << v[i] << ",";
                    }
                }
                break;
            case GL_UNSIGNED_INT:
                {
                    GLuint v[16] = { 0 };
                    _glGetUniformuiv(program, location, v);
                    for (int i = 0; i < numElems; ++i)
                    {
                        ss << v[i] << ",";
                    }
                }
                break;
            }

            value = ss.str();
        }
        else
        {
            // Uniform is inside an uniform block
            value = "BufferStore";
        }

    }

    static std::string title()
    {
        return ""
            "Loc "
            "Block "
            "Offset "
            "ArrStride "
            "MatStride "
            "RowMaj "
            "Size "
            "Type "
            "Name "
            "Value "
            ;
    }
    GLuint index;
    GLint location;
    GLint block;
    GLint offset;
    GLint arrayStride;
    GLint matrixStride;
    GLint isRowMajor;
    GLint size;
    GLenum type;
    std::string name;
    std::string value;
};

std::ostream& operator << (std::ostream& o, const UniformInfo& ui)
{
    std::ios::fmtflags f = std::cout.flags();
    o << std::hex;

    if (CFG_TRACE_PLATFORM)
    {
        o << ui.location << ' ';
        o << ui.block << ' ';
        o << ui.offset << ' ';
    }
    else
    {
        o << "- ";
        o << "- ";
        o << "- ";
    }
    o << ui.arrayStride << ' ';
    o << ui.matrixStride << ' ';
    o << ui.isRowMajor << ' ';
    o << ui.size << ' ';
    o << ui.type << ' ';
    o << ui.name << ' ';
    o << ui.value << ' ';

    std::cout.flags(f);
    return o;
}

struct GenericBufferInfo // requires GLES 3.1
{
    bool operator<(const GenericBufferInfo &rhs) const { return name < rhs.name; }

    GenericBufferInfo(GLenum bufferType, GLuint program, GLuint index_)
     : index(index_)
     , name()
     , binding(0)
     , dataSize(0)
     , refByVS(0)
     , refByFS(0)
     , refByCS(0)
     , refByGS(0)
     , refByTCS(0)
     , refByTES(0)
     , activeVars(0)
     , refs()
    {
        GLchar cname[256];
        GLsizei name_len = 0;
        _glGetProgramResourceName(program, bufferType, index, sizeof(cname), &name_len, cname);
        name = cname;
        const int len = 9;
        const GLenum properties[len] = { GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE,
                                         GL_REFERENCED_BY_VERTEX_SHADER, GL_REFERENCED_BY_FRAGMENT_SHADER,
                                         GL_REFERENCED_BY_COMPUTE_SHADER, GL_REFERENCED_BY_GEOMETRY_SHADER,
                                         GL_REFERENCED_BY_TESS_CONTROL_SHADER, GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
                                         GL_NUM_ACTIVE_VARIABLES };
        std::vector<GLint> values(len, 0);
        _glGetProgramResourceiv(program, bufferType, index, len, properties, len, NULL, values.data());
        binding = values[0];
        dataSize = values[1];
        refByVS = values[2];
        refByFS = values[3];
        refByCS = values[4];
        refByGS = values[5];
        refByTCS = values[6];
        refByTES = values[7];
        activeVars = values[8];
        refs.resize(activeVars, 0);
        const GLenum refprop = GL_ACTIVE_VARIABLES;
        _glGetProgramResourceiv(program, bufferType, index, 1, &refprop, activeVars, NULL, refs.data());
    }

    static std::string title()
    {
        return ""
            "Binding "
            "Size "
            "VS "
            "FS "
            "CS "
            "GS "
            "TCS "
            "TES "
            "Active "
            "Name "
            "Indices";
    }

    GLint index;
    std::string name;
    GLint binding;
    GLint dataSize;
    GLint refByVS;
    GLint refByFS;
    GLint refByCS;
    GLint refByGS;
    GLint refByTCS;
    GLint refByTES;
    GLint activeVars;
    std::vector<GLint> refs;
};

std::ostream& operator << (std::ostream& o, const GenericBufferInfo& gbi)
{
    std::ios::fmtflags f = std::cout.flags();
    o << std::hex;

    o << gbi.binding << ' ';
    o << gbi.dataSize << ' ';
    o << gbi.refByVS << ' ';
    o << gbi.refByFS << ' ';
    o << gbi.refByCS << ' ';
    o << gbi.refByGS << ' ';
    o << gbi.refByTCS << ' ';
    o << gbi.refByTES << ' ';
    o << gbi.activeVars << ' ';
    o << gbi.name << ' ';
    for (unsigned i = 0; i < gbi.refs.size(); ++i)
    {
        o << gbi.refs[i] << ' ';
    }

    std::cout.flags(f);
    return o;
}

struct ProgramOutputInfo // requires GLES 3.1
{
    bool operator<(const ProgramOutputInfo &rhs) const { return name < rhs.name; }

    ProgramOutputInfo(GLuint program, GLuint index_)
     : index(index_)
     , name()
     , type(GL_NONE)
     , location(0)
     , arraySize(0)
    {
        GLchar cname[256];
        GLsizei name_len = 0;
        _glGetProgramResourceName(program, GL_PROGRAM_OUTPUT, index, sizeof(cname), &name_len, cname);
        name = cname;
        const int len = 3;
        const GLenum properties[len] = { GL_LOCATION, GL_TYPE, GL_ARRAY_SIZE };
        GLint values[len] = { 0 };
        _glGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, index, len, properties, len, NULL, values);
        location = values[0];
        type = values[1];
        arraySize = values[2];
    }

    static std::string title()
    {
        return ""
            "Loc "
            "Type "
            "ArrSize "
            "Name";
    }

    GLint index;
    std::string name;
    GLenum type;
    GLint location;
    GLint arraySize;
};

std::ostream& operator << (std::ostream& o, const ProgramOutputInfo& poi)
{
    std::ios::fmtflags f = std::cout.flags();
    o << std::hex;

    o << poi.location << ' ';
    o << poi.type << ' ';
    o << poi.arraySize << ' ';
    o << poi.name << ' ';

    std::cout.flags(f);
    return o;
}

struct TransformFeedbackInfo
{
    bool operator<(const TransformFeedbackInfo &rhs) const { return name < rhs.name; }

    TransformFeedbackInfo(GLuint program, GLuint index_)
     : index(index_)
     , type(GL_NONE)
     , size(0)
     , name()
    {
        GLchar cname[256];
        _glGetTransformFeedbackVarying(program, index, sizeof(cname), NULL, &size, &type, cname);
        name = cname;
    }

    static std::string title()
    {
        return ""
            "Type "
            "Size "
            "Name";
    }

    GLuint index;
    GLenum type;
    GLsizei size;
    std::string name;
};

std::ostream& operator << (std::ostream& o, const TransformFeedbackInfo& tfbi)
{
    std::ios::fmtflags f = std::cout.flags();
    o << std::hex;

    o << tfbi.type << ' ';
    o << tfbi.size << ' ';
    o << tfbi.name << ' ';

    std::cout.flags(f);
    return o;
}

struct UniformBlockInfo
{
    bool operator<(const UniformBlockInfo &rhs) const { return name < rhs.name; }

    UniformBlockInfo(GLuint program, GLuint index_, uint64_t flags)
     : index(index_)
     , binding(0)
     , dataSize(0)
     , bufSize(0)
     , activeUniforms(0)
     , activeUniformIndices()
     , refByVS(0)
     , refByFS(0)
     , md5("-")
     , boundSize(0)
     , boundStart(0)
     , name()
    {
        _glGetActiveUniformBlockiv(program, index, GL_UNIFORM_BLOCK_BINDING, &binding);
        _glGetActiveUniformBlockiv(program, index, GL_UNIFORM_BLOCK_DATA_SIZE, &dataSize);
        _glGetActiveUniformBlockiv(program, index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &activeUniforms);
        activeUniformIndices.resize(activeUniforms);
        _glGetActiveUniformBlockiv(program, index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, activeUniformIndices.data());
        _glGetActiveUniformBlockiv(program, index, GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER, &refByVS);
        _glGetActiveUniformBlockiv(program, index, GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER, &refByFS);

        char cname[128];
        _glGetActiveUniformBlockName(program, index, 128, 0, cname);
        name = cname;

        GLint bufferId;
        _glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, binding, &bufferId);
        _glGetIntegeri_v(GL_UNIFORM_BUFFER_SIZE, binding, &boundSize);
        _glGetIntegeri_v(GL_UNIFORM_BUFFER_START, binding, &boundStart);

        if (flags & CFG_HASH_UNIFORM)
        {
            if (bufferId)
            {
                GLint mapSize = boundSize;
                GLint mapStart = boundStart;
                GLint originalBoundBuffer = 0;
                _glGetIntegerv(GL_UNIFORM_BUFFER_BINDING, &originalBoundBuffer);
                _glBindBuffer(GL_UNIFORM_BUFFER, bufferId);
                _glGetBufferParameteriv(GL_UNIFORM_BUFFER, GL_BUFFER_SIZE, &bufSize);
                if (boundSize <= 0) // assume using whole buffer
                {
                    mapSize = bufSize;
                    mapStart = 0;
                }
                void* mapPtr = _glMapBufferRange(GL_UNIFORM_BUFFER, mapStart, mapSize, GL_MAP_READ_BIT);
                if (mapPtr)
                {
                    std::stringstream md5ss;
                    md5ss << common::MD5Digest(mapPtr, mapSize);
                    md5 = md5ss.str();
                }
                else
                {
                    STATE_LOG("Pointer should not be 0!\n");
                }

                _glUnmapBuffer(GL_UNIFORM_BUFFER);
                _glBindBuffer(GL_UNIFORM_BUFFER, originalBoundBuffer);
            }
            else
            {
                STATE_LOG("No buffer bound to binding point!\n");
            }
        }
    }

    static std::string title()
    {
        return ""
            "DataSize "
            "BufSize "
            "NumActive "
            "Binding "
            "Indices "
            "RefByVS "
            "RefByFS "
            "MD5 "
            "BoundSize "
            "BoundStart "
            "Name "
            ;
    }
    GLuint index;
    GLint binding;
    GLint dataSize;
    GLint bufSize;
    GLint activeUniforms;
    std::vector<GLint> activeUniformIndices;
    GLint refByVS;
    GLint refByFS;
    std::string md5;
    GLint boundSize;
    GLint boundStart;
    std::string name;
};

std::ostream& operator << (std::ostream& o, const UniformBlockInfo& ubi)
{
    std::ios::fmtflags f = std::cout.flags();
    o << std::hex;

    o << ubi.dataSize << ' ';
    o << ubi.bufSize << ' ';
    o << ubi.activeUniforms << ' ';

    if (CFG_TRACE_PLATFORM) // depends on compiler optimizations
    {
        o << ubi.binding << ' ';
        for (auto it = ubi.activeUniformIndices.cbegin(); it != ubi.activeUniformIndices.cend(); ++it)
        {
            o << *it << ',';
        }
        o << ' ';
        o << ubi.refByVS << ' ';
        o << ubi.refByFS << ' ';
    }
    else
    {
        o << "- ";
        o << "- ";
        o << "- ";
        o << "- ";
    }
    o << ubi.md5 << ' ';
    o << ubi.boundSize << ' ';
    o << ubi.boundStart << ' ';
    o << ubi.name << ' ';

    std::cout.flags(f);
    return o;
}

void VertexArrayInfo::updateEnabled()
{
    _glGetVertexAttribiv(location, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
}

template <class T>
class BufferInfo
{
public:
    BufferInfo(GLenum _target, uintptr_t _offset, GLenum _elementType, GLuint drawCount = 0)
        : target(_target)
        , accessFlags(0)
        , mapped(0)
        , mapLength(0)
        , mapOffset(0)
        , size(0)
        , usage(0)
        , id(getBoundBuffer(_target))
        , count(0)
        , elementType(_elementType)
        , elementSize(_gl_type_size(elementType))
        , elementDrawCount(drawCount)
        , data()
        , dataUnique()
        , offset(_offset)
    {
        _glGetBufferParameteriv(target, GL_BUFFER_ACCESS_FLAGS, &accessFlags);
        _glGetBufferParameteriv(target, GL_BUFFER_MAPPED, &mapped);
        _glGetBufferParameteriv(target, GL_BUFFER_MAP_LENGTH, &mapLength);
        _glGetBufferParameteriv(target, GL_BUFFER_MAP_OFFSET, &mapOffset);
        _glGetBufferParameteriv(target, GL_BUFFER_SIZE, &size);
        _glGetBufferParameteriv(target, GL_BUFFER_USAGE, &usage);

        if (id && !size)
        {
            return;
        }

        count = size / elementSize;
        bool wasOriginallyMapped = mapped;

        T* mapPtr = 0;

        if (mapped && (accessFlags & GL_MAP_READ_BIT))
        {
            _glGetBufferPointerv(target, GL_BUFFER_MAP_POINTER, reinterpret_cast<void**>(&mapPtr));
        }
        else if (id)
        {
            mapPtr = static_cast<T*>(_glMapBufferRange(target, 0, size, GL_MAP_READ_BIT));
            if (!mapPtr)
            {
                return;
            }
        }
        else
        {
            mapPtr = 0;
        }

        char* mapBytePtr = reinterpret_cast<char*>(mapPtr);
        mapBytePtr += offset;
        mapPtr = reinterpret_cast<T*>(mapBytePtr);

        // If elementDrawCount is 0, then use entire buffer
        if (!elementDrawCount)
        {
            elementDrawCount = count;
        }

        data.assign(mapPtr, mapPtr + elementDrawCount);

        // Build a new sorted vector, dataUnique, that contains all distinct elements in data
        dataUnique = data;
        std::sort(dataUnique.begin(), dataUnique.end());
        typename std::vector<T>::iterator it = std::unique(dataUnique.begin(), dataUnique.end());
        dataUnique.resize(std::distance(dataUnique.begin(), it));

        if (!wasOriginallyMapped)
        {
            _glUnmapBuffer(target);
        }
    }

    std::ostream& dataToStr(std::ostream& os)
    {
        os << "[";
        for (auto it = data.cbegin(); it != data.cend(); ++it)
        {
            os << (*it) << ", ";
        }
        os << "]";

        return os;
    }

    std::ostream& dataUniqueToStr(std::ostream& os)
    {
        os << "[";
        for (auto it = dataUnique.cbegin(); it != dataUnique.cend(); ++it)
        {
            os << (*it) << ", ";
        }
        os << "]";

        return os;
    }

    template <typename U>
    void convertData(std::vector<U>& indexArray)
    {
        if ((CONFIG_FLAGS & CFG_TRACE_CS_FL) && !dataUnique.empty())
        {
            indexArray.push_back(static_cast<U>(dataUnique.front()));
            indexArray.push_back(static_cast<U>(dataUnique.back()));
        }
        else
        {
            for (size_t i = 0; i < dataUnique.size(); ++i)
            {
                indexArray.push_back(static_cast<U>(dataUnique[i]));
            }
        }
    }

    std::ostream& toStr(std::ostream& o, unsigned char tid, uint64_t flags)
    {
        line(o, "I", tid);

        std::ios::fmtflags f = std::cout.flags();
        o << std::hex;

        o << id << ' ';
        o << target << ' ';
        o << accessFlags << ' ';
        o << (mapped ? "Yes" : "No") << ' ';
        o << mapLength<< ' ';
        o << mapOffset << ' ';
        o << size << ' ';
        o << usage << ' ';
        o << count << ' ';
        o << elementType << ' ';
        o << elementSize << ' ';
        o << elementDrawCount << ' ';
        if (dataUnique.empty())
        {
            o << "- - ";
        }
        else
        {
            o << dataUnique.front() << ' ';
            o << dataUnique.back() << ' ';
        }
        o << std::endl;

        std::cout.flags(f);

        if (flags & CFG_TRACE_DATA_IBO)
        {
            line(o, "DI", tid);
            dataToStr(o) << std::endl;

            line(o, "DIU", tid);
            dataUniqueToStr(o) << std::endl;
        }

        return o;
    }

    static std::string title()
    {
        return ""
            "ID "
            "Target "
            "Access "
            "Mapped "
            "MapLength "
            "MapOffset "
            "Size "
            "Usage "
            "Count "
            "ElemType "
            "ElemSize "
            "ElemCount "
            "MinIndex "
            "MaxIndex "
            ;
    }

    GLenum target;
    GLint accessFlags;
    GLint mapped;
    GLint mapLength;
    GLint mapOffset;
    GLint size;
    GLint usage;
    GLint id;
    size_t count;
    GLenum elementType;
    size_t elementSize;
    size_t elementDrawCount;
    std::vector<T> data;
    std::vector<T> dataUnique;
    uintptr_t offset;
};

static void dumpProgramOutputs(unsigned char tid, GLuint program, int outputActive, std::stringstream &ss)
{
    // Dump program outputs
    if (outputActive && PRINT_TITLES)
    {
        line(ss, "O", tid) << ProgramOutputInfo::title() << std::endl;
    }
    std::vector<ProgramOutputInfo> poi;
    for (int i = 0; i < outputActive; ++i)
    {
        poi.push_back(ProgramOutputInfo(program, i));
    }
    std::sort(poi.begin(), poi.end());
    for (auto it = poi.cbegin(); it != poi.cend(); ++it)
    {
        line(ss, "O", tid) << *it << std::endl;
    }
}

static void dumpGenericBufferInfo(unsigned char tid, GLenum bufferType, GLuint program, int count, std::stringstream &ss, const char *prefix)
{
    if (count && PRINT_TITLES)
    {
        line(ss, prefix, tid) << GenericBufferInfo::title() << std::endl;
    }
    std::vector<GenericBufferInfo> gbi;
    for (int i = 0; i < count; ++i)
    {
        gbi.push_back(GenericBufferInfo(bufferType, program, i));
    }
    std::sort(gbi.begin(), gbi.end());
    for (auto it = gbi.cbegin(); it != gbi.cend(); ++it)
    {
        line(ss, prefix, tid) << *it << std::endl;
    }
}

static void dumpUniformBlockInfo(unsigned char tid, GLuint program, int count, std::stringstream &ss, int flags)
{
    // Dump uniform blocks
    if (count && PRINT_TITLES)
    {
        line(ss, "UB", tid) << UniformBlockInfo::title() << std::endl;
    }
    std::vector<UniformBlockInfo> ubi;
    for (int i = 0; i < count; ++i)
    {
        ubi.push_back(UniformBlockInfo(program, i, flags));
    }
    std::sort(ubi.begin(), ubi.end());
    for (auto it = ubi.cbegin(); it != ubi.cend(); ++it)
    {
        line(ss, "UB", tid) << *it << std::endl;
    }
}

static void dumpUniformInfo(unsigned char tid, GLuint program, int count, std::stringstream &ss)
{
    // Dump uniforms
    if (PRINT_TITLES)
    {
        line(ss, "U", tid) << UniformInfo::title() << std::endl;
    }
    std::vector<UniformInfo> ui;
    for (int i = 0; i < count; ++i)
    {
        ui.push_back(UniformInfo(program, i));
    }
    std::sort(ui.begin(), ui.end());
    for (auto it = ui.cbegin(); it != ui.cend(); ++it)
    {
        line(ss, "U", tid) << *it << std::endl;
    }
}

template<class T>
void memoryElement(std::ostream& o, int realStride, int elementSize, int index, char* cPtrBegin, char* cPtrEnd = 0)
{
    char* cPtrPosition = cPtrBegin + index * realStride;

    if (cPtrPosition >= cPtrEnd)
    {
        o << "OOB";
        return;
    }

    T* p = reinterpret_cast<T*>(cPtrPosition);
    o << Vec<T>(p, elementSize);
}

template<class T>
void memory(std::ostream& o, int realStride, int elementSize, const IndexList_t& indexArray, void* ptr, unsigned int bufOffset, unsigned int size = 0)
{
    char* cBegin = static_cast<char*>(ptr) + bufOffset;
    char* cEnd = static_cast<char*>(ptr) + size;

    for (size_t i = 0; i < indexArray.size(); ++i)
    {
        memoryElement<T>(o, realStride, elementSize, indexArray[i], cBegin, cEnd);
        o << " | ";
    }
}

void memory(std::ostream& o, GLenum elementType, int realStride, int elementSize, const IndexList_t& indexArray, void* ptr, unsigned int bufOffset, unsigned int size = 0)
{
    // Not instanced
    switch (elementType)
    {
    case GL_BYTE:
        memory<GLbyte>(o, realStride, elementSize, indexArray, ptr, bufOffset, size);
        break;
    case GL_UNSIGNED_BYTE:
        memory<GLubyte>(o, realStride, elementSize, indexArray, ptr, bufOffset, size);
        break;
    case GL_FLOAT:
        memory<GLfloat>(o, realStride, elementSize, indexArray, ptr, bufOffset, size);
        break;
    }
}


StateLogger::StateLogger()
    : mPerThreadCallCount(16)
    , mLogOpen(false)
    , mLog()
{
}

void StateLogger::open(const std::string& fileName)
{
    mFileName = fileName;
}

void StateLogger::close()
{
    if (!mLogOpen)
    {
        return;
    }
    mLog.close();
    for (auto it = mPerThreadCallCount.begin(); it != mPerThreadCallCount.end(); ++it)
    {
        it->clear();
    }
}

void StateLogger::checkIfOpen()
{
    if (!mLogOpen) {
        DBG_LOG("State logging started. The state log file name is : %s\n", mFileName.c_str());
        mLog.open(mFileName.c_str(), std::ofstream::out | std::ofstream::trunc);
        mLogOpen = true;
    }
}

void StateLogger::logFunction(unsigned char tid, const std::string& functionName, unsigned callNo, unsigned drawNo)
{
    if (!call_in_range())
    {
        return; // not the call we want
    }

    FunctionCountMap_t& funcMap = mPerThreadCallCount[tid];
    FunctionCountMap_t::iterator it = funcMap.find(functionName);

    // If function not found
    if (it == funcMap.end())
    {
        funcMap[functionName] = 0;
    }

    int& callCount = funcMap[functionName];
    int frameNo = frameNumber;

    if (!PRINT_CALLNO) // creates noise for retracer <-> tracer state comparisons
    {
        callNo = 0;
        drawNo = 0;
        frameNo = 0;
    }

    checkIfOpen();
    mLog << "@F: [" << static_cast<int>(tid) << "] " << functionName << " " << callCount++ << " call=" << callNo << " draw=" << drawNo << " frame=" << frameNo << std::endl;
}

void StateLogger::_logState(std::stringstream& ss, unsigned char tid, GLsizei instancecount, const IndexList_t& indices, uint64_t flags)
{
    GLint program = getCurrentProgram();
    if (!program)
    {
        //DBG_LOG("WARNING: No program bound\n");
        return;
    }

    ProgramInfo programInfo(program);
    VertexArray_t vertexArray(getMaxVertexAttribs());

    // Record active attributes
    for (int i = 0; i < programInfo.activeAttributes; ++i)
    {
        GLchar name[128];
        GLint size = 0;
        GLenum type = 0;
        _glGetActiveAttrib(program, i, 128, 0, &size, &type, name);

        GLint location = _glGetAttribLocation(program, name);

        if (location >= 0 && location < getMaxVertexAttribs())
        {
            vertexArray[location].size = size;
            vertexArray[location].type = type;
            vertexArray[location].name = name;
            vertexArray[location].active = true;
            vertexArray[location].location = location;
        }
        else
        {
            // Add built-in attributes like "gl_VertexId",
            // that normally appears on location >= GL_MAX_VERTEX_ATTRIBS
            VertexArrayInfo vai;
            vai.size = size;
            vai.type = type;
            vai.name = name;
            vai.active = true;
            vai.location = location;
            vertexArray.push_back(vai);
        }
    }
    std::sort(vertexArray.begin(), vertexArray.end()); // sort alphabetically

    // Dump out draw command targets for draw command
    GLint framebuffer;
    GLint elementarray;
    GLint readFramebuffer;
    GLint renderbuffer;
    _glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer);
    _glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &elementarray);
    _glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFramebuffer);
    _glGetIntegerv(GL_RENDERBUFFER_BINDING, &renderbuffer);
    if (PRINT_TITLES)
    {
        line(ss, "T", tid) << "Framebuffer ElementArray ReadFramebuffer Rendbuffer" << std::endl;
    }
    line(ss, "T", tid) << framebuffer << ' ' << elementarray << ' ' << readFramebuffer << ' ' << renderbuffer << std::endl;
    if (renderbuffer != 0)
    {
        GLenum params[] = { GL_RENDERBUFFER_WIDTH, GL_RENDERBUFFER_HEIGHT, GL_RENDERBUFFER_INTERNAL_FORMAT,
                            GL_RENDERBUFFER_RED_SIZE, GL_RENDERBUFFER_GREEN_SIZE, GL_RENDERBUFFER_BLUE_SIZE,
                            GL_RENDERBUFFER_ALPHA_SIZE, GL_RENDERBUFFER_DEPTH_SIZE, GL_RENDERBUFFER_STENCIL_SIZE,
                            GL_RENDERBUFFER_SAMPLES };
        if (PRINT_TITLES)
        {
            line(ss, "RB", tid) << "Width Height InternalFormat Red Green Blue Alpha Depth StencilSize Samples" << std::endl;
        }
        std::stringstream tmpss;
        GLint value;
        for (unsigned i = 0; i < sizeof(params) / sizeof(*params); i++)
        {
            _glGetRenderbufferParameteriv(GL_RENDERBUFFER, params[i], &value);
            tmpss << value << ' ';
        }
        line(ss, "RB", tid) << tmpss.str();
    }

    // Dump program info
    if (PRINT_TITLES)
    {
        line(ss, "P", tid) << ProgramInfo::title() << std::endl;
    }
    line(ss, "P", tid) << programInfo << std::endl;

    // Dump transform feedback varyings
    if (programInfo.transformFeedbackVaryings && PRINT_TITLES)
    {
        line(ss, "TFV", tid) << TransformFeedbackInfo::title() << std::endl;
    }
    std::vector<TransformFeedbackInfo> tfbi;
    for (int i = 0; i < programInfo.transformFeedbackVaryings; ++i)
    {
        tfbi.push_back(TransformFeedbackInfo(program, i));
    }
    std::sort(tfbi.begin(), tfbi.end());
    for (auto it = tfbi.cbegin(); it != tfbi.cend(); ++it)
    {
        line(ss, "TFV", tid) << *it << std::endl;
    }

    dumpProgramOutputs(tid, program, programInfo.activeProgramOutputs, ss);
    dumpGenericBufferInfo(tid, GL_SHADER_STORAGE_BLOCK, program, programInfo.activeShaderStorageBlocks, ss, "SSB");
    dumpGenericBufferInfo(tid, GL_ATOMIC_COUNTER_BUFFER, program, programInfo.activeAtomicCounterBuffers, ss, "AT");
    dumpUniformBlockInfo(tid, program, programInfo.activeUniformBlocks, ss, flags);
    dumpUniformInfo(tid, program, programInfo.activeUniforms, ss);

    // Dump vertex arrays
    if (PRINT_TITLES)
    {
        line(ss, "V", tid) << VertexArrayInfo::title() << std::endl;
    }

    GLint originalBoundBuffer = 0;
    _glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &originalBoundBuffer);

    for (VertexArray_t::iterator it = vertexArray.begin(); it != vertexArray.end(); ++it)
    {
        if (it->location < 0)
        {
            continue;
        }
        else if (it->location >= getMaxVertexAttribs())
        {
            line(ss, "V", tid) << (*it) << std::endl;
            continue;
        }
        GLuint i = it->location;

        it->updateEnabled();
        _glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &it->binding);
        if (!it->enabled && it->binding == 0 && !it->active)
        {
            continue; // just skip it
        }
        _glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_SIZE, &it->elemSz);
        _glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &it->stride);
        _glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_TYPE, &it->typeV);
        _glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &it->normalized);
        _glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_INTEGER, &it->integer);
        _glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_DIVISOR, &it->divisor);

        _glGetVertexAttribPointerv(i, GL_VERTEX_ATTRIB_ARRAY_POINTER, &it->bufOffsetPtr);

        // Get buffer data
        if (it->binding)
        {
            _glBindBuffer(GL_ARRAY_BUFFER, it->binding);
            _glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_ACCESS_FLAGS, &it->accessFlags);
            _glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_MAPPED, &it->isMapped);
            _glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &it->bufferSize);
            _glGetBufferParameteri64v(GL_ARRAY_BUFFER, GL_BUFFER_MAP_LENGTH, &it->mapLength);
            _glGetBufferParameteri64v(GL_ARRAY_BUFFER, GL_BUFFER_MAP_OFFSET, &it->mapOffset);

            // Casting void* to int. Since ptr is just an offset, it will not overlflow the int.
            it->bufOffset = reinterpret_cast<intptr_t>(it->bufOffsetPtr);

            _glBindBuffer(GL_ARRAY_BUFFER, originalBoundBuffer);
        }

        if (it->binding && ((flags & CFG_HASH_VBO) || (flags & CFG_TRACE_DATA_VBO)) && it->active && it->bufferSize)
        {
            // Read VBO
            if (!_glIsBuffer(it->binding))
            {
                STATE_LOG("Buffer %d does not exist!\n", it->binding);
                continue;
            }

            _glBindBuffer(GL_ARRAY_BUFFER, it->binding);

            int isMapped = 0;
            _glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_MAPPED, &isMapped);
            if (isMapped)
            {
                STATE_LOG("TODO - Handle already mapped buffers!\n");
            }

            // Map buffer
            void* mapPtr = _glMapBufferRange(GL_ARRAY_BUFFER, 0, it->bufferSize, GL_MAP_READ_BIT);

            if (mapPtr)
            {
                if (flags & CFG_HASH_VBO)
                {
                    std::stringstream md5;
                    md5 << common::MD5Digest(mapPtr, it->bufferSize);
                    it->bufferMd5 = md5.str();
                }
                else
                {
                    it->bufferMd5 = "-";
                }

                if (flags & CFG_TRACE_DATA_VBO)
                {
                    int realStride = it->elemSz * _gl_type_size(it->typeV);
                    if (it->stride)
                    {
                        realStride = it->stride;
                    }

                    line(ss, "SSD", tid);

                    if (!it->divisor)
                    {
                        // Not instanced
                        memory(ss, it->typeV, realStride, it->elemSz, indices, mapPtr, it->bufOffset, it->bufferSize);
                    }
                    else
                    {
                        // Instanced
                        int numElementsInArray = ((instancecount - 1) / it->divisor) + 1;
                        IndexList_t fakeIndices;
                        for (int i = 0; i < numElementsInArray; ++i)
                        {
                            fakeIndices.push_back(i);
                        }

                        memory(ss, it->typeV, realStride, it->elemSz, fakeIndices, mapPtr, it->bufOffset, it->bufferSize);
                    }
                    ss << std::endl;
                }
            }
            else
            {
                STATE_LOG("Pointer should not be 0!\n");
            }

            _glUnmapBuffer(GL_ARRAY_BUFFER);
            _glBindBuffer(GL_ARRAY_BUFFER, originalBoundBuffer);
        }
        else if (!it->binding && it->enabled && ((flags & CFG_HASH_VBO) || (flags & CFG_TRACE_DATA_VBO)) && !indices.empty())
        {
            // Read client buffer
            size_t elementSizeInBytes = it->elemSz * _gl_type_size(it->typeV);

            if (!it->bufOffsetPtr)
            {
                STATE_LOG("Pointer should not be 0!\n");
            }

            int numElementsInArray = 0;
            if (!it->divisor)
            {
                // Not instanced
                it->bufferSize = _glArrayPointer_size(it->elemSz, it->typeV, it->stride, indices.back());
                numElementsInArray = indices.back();
            }
            else
            {
                // Instanced
                it->bufferSize = _instanced_index_array_size(it->elemSz, it->typeV, it->stride, instancecount, it->divisor);
                numElementsInArray = ((instancecount - 1) / it->divisor) + 1;
            }

            if ((flags & CFG_TRACE_CS) && it->active)
            {
                // The number of elements (floats, uchars, etc) a stride contains.
                int realStride = it->elemSz * _gl_type_size(it->typeV);
                if (it->stride)
                {
                    realStride = it->stride;
                }

                line(ss, "CSD", tid);

                memory(ss, it->typeV, realStride, it->elemSz, indices, it->bufOffsetPtr, 0, it->bufferSize);

                ss << std::endl;
            }

            if (flags & CFG_HASH_CS)
            {
                std::stringstream md5;
                md5 << common::MD5Digest(it->bufOffsetPtr, it->stride, elementSizeInBytes, numElementsInArray);
                it->bufferMd5 = md5.str();
            }
        }

        line(ss, "V", tid) << (*it) << std::endl;
    }

    // Print attached shaders
    for (size_t i = 0; i < programInfo.shaderNames.size(); i++)
    {
        std::stringstream lss;
        GLint id = programInfo.shaderNames[i];
        GLint stype = 0, sdeleteStatus = 0, scompileStatus = 0, sinfoLogLength = 0, ssourceLength = 0;
        _glGetShaderiv(id, GL_SHADER_TYPE, &stype);
        _glGetShaderiv(id, GL_DELETE_STATUS, &sdeleteStatus);
        _glGetShaderiv(id, GL_COMPILE_STATUS, &scompileStatus);
        _glGetShaderiv(id, GL_INFO_LOG_LENGTH, &sinfoLogLength);
        if (!CFG_TRACE_PLATFORM && sinfoLogLength == 1) // hack for spurious log length on nvidia platforms
        {
            sinfoLogLength = 0;
        }
        _glGetShaderiv(id, GL_SHADER_SOURCE_LENGTH, &ssourceLength);
        lss << std::hex;
        lss << stype << " " << sdeleteStatus << " " << scompileStatus << " " << sinfoLogLength << " " << ssourceLength;
        line(ss, "S", tid) << lss.str() << std::endl;
    }
#if 0
    // FIXME: We will never get here for the separate shader program case, since we check at the top of the function for bound legacy program.
    // FIXME: Instead, we will need to rewrite this entire function to support the new separate shader program workflow. Commented out for now.
    // Print currently bound separate shader pipeline (if any)
    GLint ppipe = getInteger(GL_PROGRAM_PIPELINE_BINDING);
    if (ppipe != 0)
    {
        std::stringstream lss;
        GLint activeProgram = 0, vertexShader = 0, fragmentShader = 0, computeShader = 0, infoLogLength = 0, validateStatus = 0;
        _glGetProgramPipelineiv(ppipe, GL_ACTIVE_PROGRAM, &activeProgram);
        _glGetProgramPipelineiv(ppipe, GL_VERTEX_SHADER, &vertexShader);
        _glGetProgramPipelineiv(ppipe, GL_FRAGMENT_SHADER, &fragmentShader);
        _glGetProgramPipelineiv(ppipe, GL_COMPUTE_SHADER, &computeShader);
        _glGetProgramPipelineiv(ppipe, GL_INFO_LOG_LENGTH, &infoLogLength);
        _glGetProgramPipelineiv(ppipe, GL_VALIDATE_STATUS, &validateStatus);
        lss << activeProgram << " " << vertexShader << " " << fragmentShader << " " << computeShader << " " << infoLogLength << " " << validateStatus;
        line(ss, "|", tid) << lss.str() << std::endl;
    }
#endif
    // Write to log file
    checkIfOpen();
    mLog << ss.str() << std::flush;
}

void StateLogger::logState(unsigned char tid, GLint first, GLsizei count, GLsizei instancecount)
{
    if (!call_in_range())
    {
        return; // not the call we want
    }

    uint64_t flags = CONFIG_FLAGS;
    // Generate the index vector
    IndexList_t indexArray;

    GLsizei last = first + count - 1;

    if ((flags & CFG_TRACE_CS_FL) && count)
    {
        indexArray.push_back(first);
        indexArray.push_back(last);
    }
    else
    {
        for (int i = first; i <= last; ++i)
        {
            indexArray.push_back(i);
        }
    }

    std::stringstream ss;
    _logState(ss, tid, 0, indexArray, flags);
}

void StateLogger::logState(unsigned char tid)
{
    if (!call_in_range())
    {
        return; // not the call we want
    }

    std::stringstream ss;
    IndexList_t indexArray;
    _logState(ss, tid, 0, indexArray, CONFIG_FLAGS);
}

void StateLogger::logDrawElementsIndirect(unsigned char tid, GLenum type, const void *indirect, int count)
{
    for (int i = 0; i < count; i++)
    {
        IndirectDrawElements params;
        if (getIndirectBuffer(&params, sizeof(params), (char*)indirect + sizeof(params) * i))
        {
            logState(tid, params.count, type, (const GLvoid *)(params.firstIndex * _gl_type_size(type)), params.instanceCount);
        }
    }
}

void StateLogger::logDrawArraysIndirect(unsigned char tid, const void *indirect, int count)
{
    for (int i = 0; i < count; i++)
    {
        IndirectDrawArrays params;
        if (getIndirectBuffer(&params, sizeof(params), (char*)indirect + sizeof(params) * i))
        {
             logState(tid, params.first, params.count, params.primCount);
        }
    }
}

void StateLogger::logComputeIndirect(unsigned char tid, GLintptr offset)
{
    if (!call_in_range())
    {
        return; // not the call we want
    }

    IndirectCompute params;
    GLint bufSize = 0;
    GLint bufferId = 0;
    _glGetIntegerv(GL_DISPATCH_INDIRECT_BUFFER_BINDING, &bufferId);
    if (bufferId == 0)
    {
        STATE_LOG("No dispatch buffer bound for indirect compute call!\n");
        return;
    }
    _glGetBufferParameteriv(GL_DISPATCH_INDIRECT_BUFFER, GL_BUFFER_SIZE, &bufSize);
    if ((size_t)bufSize < sizeof(params) + offset)
    {
        STATE_LOG("Buffer overrun reading indirect compute parameters!\n");
        return;
    }
    void* mapPtr = _glMapBufferRange(GL_DISPATCH_INDIRECT_BUFFER, offset, sizeof(params), GL_MAP_READ_BIT);
    if (mapPtr)
    {
        memcpy(&params, mapPtr, sizeof(params));
       _glUnmapBuffer(GL_DISPATCH_INDIRECT_BUFFER);
    }
    else
    {
        STATE_LOG("Reading out indirect compute parameters!\n");
        return;
    }
    logCompute(tid, params.x, params.y, params.z);
}

void StateLogger::logCompute(unsigned char tid, GLuint x, GLuint y, GLuint z)
{
    if (!call_in_range())
    {
        return; // not the call we want
    }

    GLuint program = getCurrentProgram();
    GLint cwsize, ssb_active, ssb_max, uniform_active, ub_active, ub_max, input_active, output_active, acb_active, acb_max, var_active;
    _glGetProgramiv(program, GL_COMPUTE_WORK_GROUP_SIZE, &cwsize);
    _glGetProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &ssb_active);
    _glGetProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK, GL_MAX_NUM_ACTIVE_VARIABLES, &ssb_max);
    _glGetProgramInterfaceiv(program, GL_UNIFORM, GL_ACTIVE_RESOURCES, &uniform_active);
    _glGetProgramInterfaceiv(program, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &ub_active);
    _glGetProgramInterfaceiv(program, GL_UNIFORM_BLOCK, GL_MAX_NUM_ACTIVE_VARIABLES, &ub_max);
    _glGetProgramInterfaceiv(program, GL_ATOMIC_COUNTER_BUFFER, GL_ACTIVE_RESOURCES, &acb_active);
    _glGetProgramInterfaceiv(program, GL_ATOMIC_COUNTER_BUFFER, GL_MAX_NUM_ACTIVE_VARIABLES, &acb_max);
    _glGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &input_active);
    _glGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &output_active);
    _glGetProgramInterfaceiv(program, GL_BUFFER_VARIABLE, GL_ACTIVE_RESOURCES, &var_active);
    std::stringstream ss;
    line(ss, "COMP", tid) << "x=" << x << " y=" << y << " z=" << z << " cws=" << cwsize << " ssb=" << ssb_active << "," << ssb_max
        << " u=" << uniform_active << " ub=" << ub_active << "," << ub_max << " input=" << input_active << " output=" << output_active
        << " var=" << var_active << " acb=" << acb_active << "," << acb_max << std::endl;

    dumpProgramOutputs(tid, program, output_active, ss);
    dumpUniformBlockInfo(tid, program, ub_active, ss, CONFIG_FLAGS);
    dumpUniformInfo(tid, program, uniform_active, ss);
    dumpGenericBufferInfo(tid, GL_SHADER_STORAGE_BLOCK, program, ssb_active, ss, "SSB");
    dumpGenericBufferInfo(tid, GL_ATOMIC_COUNTER_BUFFER, program, acb_active, ss, "AT");

    // Write to log file
    checkIfOpen();
    mLog << ss.str() << std::flush;
}

void StateLogger::logState(unsigned char tid, GLsizei count, GLenum indexType, const GLvoid* indices, GLsizei instancecount)
{
    if (!call_in_range())
    {
        return; // not the call we want
    }

    std::stringstream ss;

    if (PRINT_TITLES)
    {
        line(ss, "I", tid) << BufferInfo<GLuint>::title() << std::endl;
    }

    uintptr_t bufferOffset = reinterpret_cast<uintptr_t>(indices);

    // Convert to an int vector
    IndexList_t indexArray;
    switch (indexType)
    {
        case GL_UNSIGNED_BYTE:
        {
            BufferInfo<GLubyte> elementArray(GL_ELEMENT_ARRAY_BUFFER, bufferOffset, indexType, count);
            elementArray.toStr(ss, tid, CONFIG_FLAGS);
            elementArray.convertData(indexArray);
            break;
        }
        case GL_UNSIGNED_SHORT:
        {
            BufferInfo<GLushort> elementArray(GL_ELEMENT_ARRAY_BUFFER, bufferOffset, indexType, count);
            elementArray.toStr(ss, tid, CONFIG_FLAGS);
            elementArray.convertData(indexArray);
            break;
        }
        case GL_UNSIGNED_INT:
        {
            BufferInfo<GLuint> elementArray(GL_ELEMENT_ARRAY_BUFFER, bufferOffset, indexType, count);
            elementArray.toStr(ss, tid, CONFIG_FLAGS);
            elementArray.convertData(indexArray);
            break;
        }
        default:
        {
            STATE_LOG("Wrong type in drawcall: %x", indexType);
            break;
        }
    }

    _logState(ss, tid, instancecount, indexArray, CONFIG_FLAGS);
}

State::State(const std::vector<GLenum>& boolsToSave,
             const std::vector<GLenum>& intsToSave,
             const std::vector<GLenum>& floatsToSave,
             bool saveVertexArrays)
 : mProgramInfo(getCurrentProgram(), true)
 , mVertexArrays(getMaxVertexAttribs())
 , mBoolMap()
 , mIntMap()
 , mFloatMap()
 , mSaveVertexArrays(saveVertexArrays)
{
    std::vector<GLenum>::const_iterator it = find(intsToSave.begin(), intsToSave.end(), GL_TEXTURE_BINDING_2D);
    if (it != intsToSave.end())
    {
        // We need to disable preserving of texture binding for now. It would require
        // that we store its corresponding texture unit.
        DBG_LOG("Warning: Restoring GL_TEXTURE_BINDING_2D is currently unsupported\n");
    }

    for (std::vector<GLenum>::const_iterator it = boolsToSave.begin(); it != boolsToSave.end(); ++it)
    {
        mBoolMap[*it] = getBooleanv(*it);
    }

    for (std::vector<GLenum>::const_iterator it = intsToSave.begin(); it != intsToSave.end(); ++it)
    {
        mIntMap[*it] = getIntegerv(*it);
    }

    for (std::vector<GLenum>::const_iterator it = floatsToSave.begin(); it != floatsToSave.end(); ++it)
    {
        mFloatMap[*it] = getFloatv(*it);
    }

    if (mSaveVertexArrays)
    {
        storeVertexArrays();
    }
}

State::~State()
{
    if (mSaveVertexArrays)
    {
        restoreVertexArrays();
    }

    for (std::map<GLenum, std::vector<GLfloat> >::iterator it = mFloatMap.begin(); it != mFloatMap.end(); ++it)
    {
        setFloatv(it->first, it->second);
    }

    for (std::map<GLenum, std::vector<GLint> >::iterator it = mIntMap.begin(); it != mIntMap.end(); ++it)
    {
        setIntegerv(it->first, it->second);
    }

    for (std::map<GLenum, std::vector<GLboolean> >::iterator it = mBoolMap.begin(); it != mBoolMap.end(); ++it)
    {
        setBooleanv(it->first, it->second);
    }
}

void State::storeVertexArrays()
{
    if (mProgramInfo.id == 0)
    {
        // The code below currently works by querying active
        // attributes from the program. If there is no current program,
        // there is nothing to do -- so return.
        // This also stops any GL-errors from being raised.
        return;
    }

    mProgramInfo.update();

    // Record active attributes
    for (int i = 0; i < mProgramInfo.activeAttributes; ++i)
    {
        GLchar name[128];
        GLint size = 0;
        GLenum type = 0;
        _glGetActiveAttrib(mProgramInfo.id, i, 128, 0, &size, &type, name);

        GLint location = _glGetAttribLocation(mProgramInfo.id, name);

        if (location >= 0 && location < getMaxVertexAttribs())
        {
            mVertexArrays[location].size = size;
            mVertexArrays[location].type = type;
            mVertexArrays[location].name = name;
            mVertexArrays[location].active = true;
            mVertexArrays[location].location = location;
        }
        else
        {
            // Add built-in attributes like "gl_VertexId",
            // that normally appears on location >= GL_MAX_VERTEX_ATTRIBS
            VertexArrayInfo vai;
            vai.size = size;
            vai.type = type;
            vai.name = name;
            vai.active = true;
            vai.location = location;
            mVertexArrays.push_back(vai);
        }
    }

    for (auto it = mVertexArrays.begin(); it != mVertexArrays.end(); ++it)
    {
        GLint location = it->location;
        if (location >= getMaxVertexAttribs() || location < 0)
        {
            continue;
        }

        it->updateEnabled();
        _glGetVertexAttribiv(location, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &it->binding);
        _glGetVertexAttribiv(location, GL_VERTEX_ATTRIB_BINDING, &it->bindingindex);
        _glGetVertexAttribiv(location, GL_VERTEX_ATTRIB_RELATIVE_OFFSET, &it->relativeOffset);
        _glGetIntegeri_v(GL_VERTEX_BINDING_STRIDE, it->bindingindex, &it->vbstride);
        _glGetIntegeri_v(GL_VERTEX_BINDING_OFFSET, it->bindingindex, &it->vboffset);

        mArraysMap[it->bindingindex].push_back(*it);

        _glGetVertexAttribiv(location, GL_VERTEX_ATTRIB_ARRAY_SIZE, &it->elemSz);
        _glGetVertexAttribiv(location, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &it->stride);
        _glGetVertexAttribiv(location, GL_VERTEX_ATTRIB_ARRAY_TYPE, &it->typeV);
        _glGetVertexAttribiv(location, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &it->normalized);
        _glGetVertexAttribiv(location, GL_VERTEX_ATTRIB_ARRAY_INTEGER, &it->integer);
        _glGetVertexAttribiv(location, GL_VERTEX_ATTRIB_ARRAY_DIVISOR, &it->divisor);

        _glGetVertexAttribPointerv(location, GL_VERTEX_ATTRIB_ARRAY_POINTER, &it->bufOffsetPtr);
        it->bufOffset = reinterpret_cast<intptr_t>(it->bufOffsetPtr);
    }
}

void State::restoreVertexArrays()
{
    if (mProgramInfo.id == 0)
    {
        // See comment in storeVertexArrays().
        return;
    }

    mProgramInfo.restore();
    GLint arrayBufferId;
    _glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBufferId);

    for (auto it =  mArraysMap.cbegin(); it != mArraysMap.cend(); it++)
    {
        GLint bindingindex = it->first;
        VertexArray_t vais = it->second;
        _glBindVertexBuffer(bindingindex, vais[0].binding, vais[0].vboffset, vais[0].vbstride );
        for(auto vai = vais.cbegin(); vai!=vais.cend(); vai++){
            _glVertexAttribFormat(vai->location, vai->elemSz, vai->typeV, vai->normalized, vai->relativeOffset);
            _glVertexAttribBinding(vai->location, vai->bindingindex);
            if (vai->enabled)
            {
                _glEnableVertexAttribArray(vai->location);
            }
            else
            {
                _glDisableVertexAttribArray(vai->location);
            }
        }
    }

    _glBindBuffer(GL_ARRAY_BUFFER, arrayBufferId);
}

void State::setIntegerv(GLenum pname, const std::vector<GLint>& value)
{
    switch (pname)
    {
    case GL_ACTIVE_TEXTURE:
        _glActiveTexture(value[0]);
        break;
    case GL_CURRENT_PROGRAM:
        _glUseProgram(value[0]);
        break;
    case GL_TEXTURE_BINDING_2D:
        _glBindTexture(GL_TEXTURE_2D, value[0]);
        break;
    case GL_ARRAY_BUFFER_BINDING:
        _glBindBuffer(GL_ARRAY_BUFFER, value[0]);
        break;
    case GL_ELEMENT_ARRAY_BUFFER_BINDING:
        _glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, value[0]);
        break;
    case GL_FRONT_FACE:
        _glFrontFace(value[0]);
        break;
    case GL_DRAW_FRAMEBUFFER_BINDING:
        _glBindFramebuffer(GL_DRAW_FRAMEBUFFER, value[0]);
        break;
    case GL_READ_FRAMEBUFFER_BINDING:
        _glBindFramebuffer(GL_READ_FRAMEBUFFER, value[0]);
        break;
    case GL_UNIFORM_BUFFER_BINDING:
        _glBindBuffer(GL_UNIFORM_BUFFER, value[0]);
        break;
    case GL_VERTEX_ARRAY_BINDING:
        _glBindVertexArray(value[0]);
        break;
    case GL_VIEWPORT:
        _glViewport(value[0], value[1], value[2], value[3]);
        break;
    default:
        DBG_LOG("Unhandled setIntegerv binding for 0x%x -> %d\n", (unsigned)pname, value[0]);
        break;
    }
}

void State::setFloatv(GLenum pname, const std::vector<GLfloat>& value)
{
    switch (pname)
    {
    case GL_COLOR_CLEAR_VALUE:
        _glClearColor(value[0], value[1], value[2], value[3]);
        break;
    default:
        DBG_LOG("Unhandled setFloatv binding for 0x%x -> %f\n", (unsigned)pname, value[0]);
        break;
    }
}
