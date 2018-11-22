#include <cassert>
#include <numeric>
#include <unordered_set>
#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <utility>
#include <algorithm>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#include <limits.h>
#include <iomanip>
#include <sstream>

#include "tool/parse_interface_retracing.hpp"

#include "jsoncpp/include/json/writer.h"
#include "common/in_file.hpp"
#include "common/file_format.hpp"
#include "common/out_file.hpp"
#include "common/api_info.hpp"
#include "common/parse_api.hpp"
#include "common/trace_model.hpp"
#include "common/gl_utility.hpp"
#include "common/os.hpp"
#include "eglstate/context.hpp"
#include "tool/config.hpp"
#include "base/base.hpp"

static std::string compiler; // path to malisc
static int startframe = 0;
static int lastframe = INT_MAX;
static bool debug = false;
static bool dump_to_text = false;
static std::string dump_csv_filename;
static int frames = 0;
static std::unordered_set<int> renderpassframes;
static bool complexity_only_mode = false;
static bool dumpCallTime = false;
static bool bareLog = false;
static bool report_unused_shaders = false;
static bool write_used_shaders = false;
static std::unordered_map<int, double> heavinesses;

struct ShaderState
{
    int arithmetic = 0;
    int loadstore = 0;
    int texturing = 0;
    bool spilling = false;
    int uniform_registers = 0;
};

struct ProgramState
{
    GLint linkStatus = 0;
    GLint activeAttributes = 0;
    GLint activeUniforms = 0;
    GLint activeUniformBlocks = 0;
    GLint activeAtomicCounterBuffers = 0;
    GLint activeInputs = 0;
    GLint activeOutputs = 0;
    GLint activeTransformFeedbackVaryings = 0;
    GLint activeBufferVariables = 0;
    GLint activeSSBOs = 0;
    GLint activeTransformFeedbackBuffers = 0;
    struct ProgramSampler
    {
        std::string name;
        GLenum type;
        GLint value;
    };
    std::vector<ProgramSampler> texture_bindings;
};

static inline bool relevant(int frame)
{
    return (frame >= startframe && frame <= lastframe);
}
#define debugstream if (!debug) {} else std::cerr
#define dumpstream if (!(relevant(frames) && dump_to_text)) {} else std::cout
#define DEBUG_LOG(...) if (debug) DBG_LOG(__VA_ARGS__)

// For legacy texture functions, merge input format and type into sized internal format
static GLenum merge_texture_format(GLenum internalformat, GLenum type)
{
    GLenum newformat = internalformat;
    switch (internalformat)
    {
    case GL_RGB: // unsized internal formats
        switch (type)
        {
        case GL_UNSIGNED_BYTE:
            newformat = GL_RGBA8;
            break;
        case GL_UNSIGNED_SHORT_5_6_5:
            newformat = GL_RGB565;
            break;
        }
        break;
    case GL_RGBA:
        switch (type)
        {
        case GL_UNSIGNED_BYTE:
            newformat = GL_RGBA8;
            break;
        case GL_UNSIGNED_SHORT_4_4_4_4:
            newformat = GL_RGBA4;
            break;
        case GL_UNSIGNED_SHORT_5_5_5_1:
            newformat = GL_RGB5_A1;
            break;
        }
        break;
    case GL_LUMINANCE_ALPHA: // only one possible size formats
    case GL_LUMINANCE:
    case GL_ALPHA:
    default: // sized internal formats
        break;
    }
    return newformat;
}

static int count_uniform_values(common::CallTM* call)
{
    int count = 0;
    bool matrix_handled = false;

    // Find base multiplier
    if (call->mCallName.find("2x3") != std::string::npos || call->mCallName.find("3x2") != std::string::npos)
    {
        count = 6;
        matrix_handled = true;
    }
    else if (call->mCallName.find("2x4") != std::string::npos || call->mCallName.find("4x2") != std::string::npos)
    {
        count = 8;
        matrix_handled = true;
    }
    else if (call->mCallName.find("3x4") != std::string::npos || call->mCallName.find("4x3") != std::string::npos)
    {
        count = 12;
        matrix_handled = true;
    }
    else if (call->mCallName.find("1") != std::string::npos)
    {
        count = 1;
    }
    else if (call->mCallName.find("2") != std::string::npos)
    {
        count = 2;
    }
    else if (call->mCallName.find("3") != std::string::npos)
    {
        count = 3;
    }
    else if (call->mCallName.find("4") != std::string::npos)
    {
        count = 4;
    }

    if (!matrix_handled && call->mCallName.find("Matrix") != std::string::npos) // eg glUniformMatrix4fv - an array of 4x4 matrices
    {
        count *= count;
    }

    // Multiply base multiplier with array length for pointer to array variants
    if (call->mCallName.find("fv") != std::string::npos || call->mCallName.find("iv") != std::string::npos)
    {
        if (call->mCallName.find("glUniform") != std::string::npos)
        {
            count *= call->mArgs[1]->GetAsInt();
        }
        else // glProgramUniform*()
        {
            count *= call->mArgs[2]->GetAsInt();
        }
    }

    return count;
}

static ProgramState getProgramInfo(GLuint id)
{
    ProgramState p;

    _glGetProgramiv(id, GL_LINK_STATUS, &p.linkStatus);
    _glGetProgramiv(id, GL_ACTIVE_ATTRIBUTES, &p.activeAttributes);
    _glGetProgramInterfaceiv(id, GL_UNIFORM, GL_ACTIVE_RESOURCES, &p.activeUniforms);
    _glGetProgramInterfaceiv(id, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &p.activeUniformBlocks);
    _glGetProgramInterfaceiv(id, GL_ATOMIC_COUNTER_BUFFER, GL_ACTIVE_RESOURCES, &p.activeAtomicCounterBuffers);
    _glGetProgramInterfaceiv(id, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &p.activeInputs);
    _glGetProgramInterfaceiv(id, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &p.activeOutputs);
    _glGetProgramInterfaceiv(id, GL_TRANSFORM_FEEDBACK_BUFFER, GL_ACTIVE_RESOURCES, &p.activeTransformFeedbackBuffers);
    _glGetProgramInterfaceiv(id, GL_TRANSFORM_FEEDBACK_VARYING, GL_ACTIVE_RESOURCES, &p.activeTransformFeedbackVaryings);
    _glGetProgramInterfaceiv(id, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &p.activeSSBOs);
    for (int i = 0; i < p.activeUniforms; ++i)
    {
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = GL_NONE;
        GLchar tmp[128];
        memset(tmp, 0, sizeof(tmp));
        _glGetActiveUniform(id, i, sizeof(tmp) - 1, &length, &size, &type, tmp);
        GLint location = _glGetUniformLocation(id, tmp);
        for (int j = 0; location >= 0 && j < size; ++j)
        {
            if (isUniformSamplerType(type))
            {
                GLint param = 0;
                _glGetUniformiv(id, location + j, &param); // arrays are guaranteed to be in sequential location
                p.texture_bindings.push_back({ tmp, type, param });
            }
        }
    }
    return p;
}

static int sum_map(std::map<int, int> &map)
{
    return std::accumulate(map.cbegin(), map.cend(), 0, [](const int lhs, const std::pair<int ,int> &rhs) { return lhs + rhs.second; });
}

static std::string shader_filename(const StateTracker::Shader &shader, int context_index, int program_index)
{
    return "shader_" + std::to_string(shader.id) + "_p" + std::to_string(program_index) + "_c" + std::to_string(context_index) + shader_extension(shader.shader_type);
}

static void printHelp()
{
    std::cout <<
        "Usage : analyze_trace [OPTIONS] trace_file\n"
        "Options:\n"
        "  -c <path>     Mali shader compiler path\n"
        "  -f <f> <l>    Define frame interval, inclusive\n"
        "  -h            Print help\n"
        "  -v            Print version\n"
        "  -D            Add debug information to stderr\n"
        "  -d            Dump trace log to stdout (similar to trace_to_txt)\n"
        "  -o <basename> Dump metrics in CSV and JSON format to given base filename\n"
        "  -r <frames>   Dump renderpass metrics for given comma-separated list of frames (no spaces in the list!)\n"
        "  -C            Only report complexity metric on stdout\n"
        "  -S            Show visual output\n"
        "  -s            Report also on shaders that are not in use\n"
        "  -n            Do not save screenshots\n"
        "Options for per frame output:\n"
        "  -Z            Write out used shaders to disk\n"
        "Options for trace log:\n"
        "  -z            Show CPU cycles for each call\n"
        "  -b            Bare log - useful for making diffs\n"
        ;
}

static void printVersion()
{
    std::cout << PATRACE_VERSION << std::endl;
}

struct PerUnit
{
    std::string csv_description;
    std::vector<long> values;
};

long sum_PerUnit_values(const PerUnit& values)
{
    long sum = 0;
    for (const auto& v : values.values)
    {
        sum += v;
    }
    return sum;
}

static std::map<std::string, PerUnit> createPerDraw()
{
    std::map<std::string, PerUnit> v;
    v["primitives"].csv_description = "Primitives";
    v["vertices"].csv_description = "Vertices";
    v["vertices.unique"].csv_description = "Unique Vertices";
    v["max_sparseness"].csv_description = "Maximum Indexing Sparseness";
    v["avg_sparseness"].csv_description = "Average Indexing Sparseness"; // TBD, should be double
    v["spatial_locality"].csv_description = "Spatial Locality %";
    v["temporal_locality"].csv_description = "Temporal Locality %";
    v["vec4_locality"].csv_description = "Block of 4 Locality %";
    v["instancing"].csv_description = "Instancing";
    v["uniforms"].csv_description = "Uniform calls";
    v["uniform_values"].csv_description = "Uniform values";
    v["texture_binding"].csv_description = "Texture binding calls";
    v["buffer_binding"].csv_description = "Buffer binding calls";
    v["vertex_buffer_binding"].csv_description = "Vertex buffer binding calls";
    v["index_buffer_binding"].csv_description = "Index buffer binding calls";
    v["uniform_buffer_binding"].csv_description = "Uniform buffer binding calls";
    v["vertex_buffer_changes"].csv_description = "Vertex buffer changes";
    v["uniform_buffer_changes"].csv_description = "Uniform buffer changes";
    v["index_buffer_changes"].csv_description = "Index buffer changes";
    v["program_binding"].csv_description = "Program binding calls";
    v["textures"].csv_description = "Texture sources";
    v["renderpass"].csv_description = "Renderpass index";
    return v;
}

static std::map<std::string, PerUnit> createPerFrame()
{
    std::map<std::string, PerUnit> v;
    v["draws.total"].csv_description = "Draw:Total";
    v["draws.blended"].csv_description = "Draw:Blended";
    v["draws.depthtested"].csv_description = "Draw:Depthtested";
    v["draws.depthwriting"].csv_description = "Draw:Depthwriting";
    v["draws.stenciled"].csv_description = "Draw:Stenciled";
    v["draws.faceculled"].csv_description = "Draw:Faceculled";
    v["draws.indexed"].csv_description = "Draw:Indexed";
    v["draws.indirect"].csv_description = "Draw:Indirect";
    v["draws.instanced"].csv_description = "Draw:Instanced";
    v["draws.polygonoffset"].csv_description = "Draw:PolygonOffset";;
    v["draws.primitive_restart"].csv_description = "Draw:Primitive restart";
    v["draws.dither"].csv_description = "Draw:Dithering";
    v["compressed_textures"].csv_description = "Compressed Textures";
    v["uncompressed_textures"].csv_description = "Uncompressed Textures";
    v["vertices"].csv_description = "Vertices";
    v["vertices.indexed"].csv_description = "Indexed Vertices";
    v["vertices.unique"].csv_description = "Unique Indexed Vertices";
    v["primitives"].csv_description = "Primitives";
    v["instancing"].csv_description = "Instancing";
    v["clears"].csv_description = "Clears";
    v["calls"].csv_description = "Calls";
    v["compute"].csv_description = "Compute";
    v["framebuffers"].csv_description = "Framebuffers";
    v["flushes"].csv_description = "Explicit flushes";
    v["uniforms"].csv_description = "Uniform calls";
    v["uniform_values"].csv_description = "Uniform values";
    v["programs_linked"].csv_description = "Programs linked";
    v["texture_binding"].csv_description = "Texture binding calls";
    v["buffer_binding"].csv_description = "Buffer binding calls";
    v["vertex_buffer_changes"].csv_description = "Vertex buffer changes";
    v["uniform_buffer_changes"].csv_description = "Uniform buffer changes";
    v["index_buffer_changes"].csv_description = "Index buffer changes";
    v["program_binding"].csv_description = "Program binding calls";
    v["textures"].csv_description = "Texture sources";
    v["vao_binding"].csv_description = "Vertex array object binding calls";
    v["fbo_attachments"].csv_description = "Framebuffer attachment changes"; // compare vs 'framebuffers'
    return v;
}

struct BlendMode {
    GLenum modeRGB, modeAlpha, srcRGB, dstRGB, srcAlpha, dstAlpha;

    BlendMode() : modeRGB(GL_FUNC_ADD), modeAlpha(GL_FUNC_ADD), srcRGB(GL_ONE), dstRGB(GL_ZERO), srcAlpha(GL_ONE), dstAlpha(GL_ZERO) {} // GL defaults

    std::string to_str() const
    {
        return "Equation { RGB=" + blendEnum(modeRGB) + ", Alpha=" + blendEnum(modeAlpha) + " }, Function { RGB=(" + blendEnum(srcRGB)
               + ", " + blendEnum(dstRGB) + "), Alpha=(" + blendEnum(srcAlpha) + ", " + blendEnum(dstAlpha) + ") }";
    }

    void setEquation(GLenum _modeRGB, GLenum _modeAlpha)
    {
        modeRGB = _modeRGB;
        modeAlpha = _modeAlpha;
    }

    void setFunction(GLenum _srcRGB, GLenum _dstRGB, GLenum _srcAlpha, GLenum _dstAlpha)
    {
        srcRGB = _srcRGB;
        dstRGB = _dstRGB;
        srcAlpha = _srcAlpha;
        dstAlpha = _dstAlpha;
    }

    bool operator<(const BlendMode& rhs) const
    {
        std::vector<GLenum> lhsVec = { modeRGB, modeAlpha, srcRGB, dstRGB, srcAlpha, dstAlpha };
        std::vector<GLenum> rhsVec = { rhs.modeRGB, rhs.modeAlpha, rhs.srcRGB, rhs.dstRGB, rhs.srcAlpha, rhs.dstAlpha };
        return lhsVec < rhsVec;
    }
};

static void write_CSV(const std::string& csv_filename, std::map<std::string, PerUnit> &map, bool omit_last)
{
    // Write legacy style
    std::fstream fs;
    fs.open(csv_filename + ".csv", std::fstream::out |  std::fstream::trunc);
    for (auto& v : map)
    {
        if (omit_last)
        {
            v.second.values.pop_back(); // remove last column as we only want actual draws
        }
        fs << v.second.csv_description;
        for (auto& f : v.second.values)
        {
            fs << "," << f;
        }
        fs << std::endl;
    }
    fs.close();
    // Write normal style (the way everyone else does CSV data)
    // PS We did the omit_last step above - don't repeat it!
    fs.open(csv_filename + ".std.csv", std::fstream::out |  std::fstream::trunc);
    fs << "Index";
    int count = 0;
    for (auto& v : map) // write out column headers
    {
        fs << "," << v.second.csv_description;
        count = v.second.values.size();
    }
    fs << std::endl;
    for (int item = 0; item < count; item++)
    {
        fs << item;
        for (auto& v : map)
        {
            fs << "," << v.second.values.at(item);
        }
        fs << std::endl;
    }
    fs.close();
}

static void startNewRows(std::map<std::string, PerUnit> &map)
{
    for (auto& v : map)
    {
        v.second.values.push_back(0); // initialize
    }
}

static void addMapToJson(Json::Value &result, const std::string& key, std::map<std::string, Json::Value::Int64>& map)
{
    result[key] = Json::arrayValue;
    for (auto& s : map)
    {
        Json::Value v;
        v["name"] = s.first;
        v["count"] = s.second;
        result[key].append(v);
    }
}

enum { FEATURE_VA, FEATURE_VBO, FEATURE_INDIRECT, FEATURE_SSBO, FEATURE_UBO, FEATURE_TF, FEATURE_PRIMRESTART,
       FEATURE_INSTANCING, FEATURE_EGL_IMAGE, FEATURE_SEPARATE_SHADER_OBJECTS, FEATURE_VERTEX_ATTR_BINDING,
       FEATURE_VERTEX_BUFFER_BINDING, FEATURE_DISCARD_FBO, FEATURE_INVALIDATE_FBO, FEATURE_BINARY_SHADERS, FEATURE_TESSELLATION,
       FEATURE_GEOMETRY_SHADER, FEATURE_COMPUTE_SHADER, FEATURE_EGLCREATEIMAGE, FEATURE_POLYGON_OFFSET_FILL, FEATURE_VAO,
       FEATURE_FBO_REUSE, FEATURE_INDIRECT_COMPUTE, FEATURE_MAX };
static std::vector<const char*> featurenames = { "VA", "VBO", "DrawIndirect", "SSBO", "UBO", "TransformFeedback", "PrimitiveRestart",
                                   "Instancing", "EGLImage", "SeparateShaderObjects", "VertexAttribBinding", "VertexBufferBinding",
                                   "DiscardFBO", "InvalidateFBO", "BinaryShaders", "Tessellation", "GeometryShader", "ComputeShader",
                                   "EglCreateImage", "PolygonOffsetFill", "VAO", "FramebufferReuse", "IndirectCompute" };
static std::vector<double> complexity_feature_value = { 0.02, 0.04, 0.2, 0.07, 0.06, 0.1, 0.2, 0.08, 0.08, 0.5, 0.15, 0.15, 0.05, 0.05, 0.3,
                                                        0.4, 0.3, 0.2, 0.2, 0.2, 0.15, 0.5, 0.6 };

struct CallData
{
    long cycles;
    long count;
};

class AnalyzeTrace
{
private:
    std::map<std::string, PerUnit> perframe = createPerFrame();
    std::map<std::string, PerUnit> perdraw = createPerDraw();
    std::map<int, std::unordered_set<int>> textures_by_renderpass;
    std::vector<int> features;
    int clears = 0;
    int compute = 0;
    int flushes = 0;
    int blended = 0;
    int dither = 0;
    int depthtested = 0;
    int depthwriting = 0;
    int stenciled = 0;
    int faceculled = 0;
    int indexed = 0;
    int calls = 0; // actually "executed" calls
    int relative_frames = 0;
    long drawcalls = 0;
    std::vector<int> calls_per_frame;
    GLuint numColorAttachments = 0;
    GLuint highestTextureUnitUsed = 0;
    std::map<GLenum, bool> depthfuncs;
    std::map<BlendMode, int> blendModes; // number of times a particular blend mode is used in a draw call
    std::map<GLenum, Json::Value::Int64> drawtypes; // triangles, triangle strips, etc
    std::map<GLenum, Json::Value::Int64> drawtypescount; // as above, but sum of vertices drawn with each
    std::map<GLenum, Json::Value::Int64> drawtypesprimitives; // also count primitives
    std::map<GLenum, Json::Value::Int64> drawtypesindirect; // did draw type utilize indirect drawing?
    std::map<GLenum, Json::Value::Int64> shaders; // type and count
    std::map<std::string, Json::Value::Int64> texturetypes; // texture target as type
    std::map<std::string, std::map<std::string, Json::Value::Int64>> texturetypefilters; // for each texture target type, how many of each type of filtering is used
    std::map<std::string, std::map<std::string, Json::Value::Int64>> texturetypesizes; // for each texture target type, how many of each type of filtering is used
    struct ALT { int a; int l; int t; }; // arithemtic, load-store, texture pipe cycles
    std::map<GLenum, ALT> shadercycles; // A,L,T cycles for each shader type
    int clientsidebuffers = 0; // total number
    int64_t clientsidebuffersize = 0; // total size of
    int last_changed_vertex_buffer = -1;
    struct Context {
        std::map<unsigned, ShaderState> shaders;
        std::map<unsigned, ProgramState> programs;
        int id;
        int index;
        int display;
        int share_context;
        int textures = 0;
        int compressed_textures = 0;
        int curbuffer = 0; // currently bound array buffer
        int drawframebuffer = 0; // currently bound draw framebuffer
        int framebuffers = 0;
        BlendMode blendMode;
        std::map<GLuint, GLenum> shadertypes;
        std::map<GLuint, bool> textureIdCompressed;
        std::unordered_set<GLuint> textureIdUsed; // has been in use this frame
        std::map<GLenum, std::map<GLuint, GLuint>> boundTextureIds; // target : texture unit : texture id
        GLuint activeTextureUnit = 0;
        Context(int _id, int _index, int _display, int _share) : id(_id), index(_index), display(_display), share_context(_share) {}
    };
    std::vector<Context> contexts;
    struct Surface {
        GLuint id;
        int index;
        int display;
        unsigned call_created;
        unsigned call_destroyed;
        unsigned frame_created;
        unsigned frame_destroyed;
        int glcount;
        int eglcount;
        int activations;
        int swaps;
        Surface(int _id, int _index, int _display, unsigned _call_created, unsigned _frame_created)
                : id(_id), index(_index), display(_display), call_created(_call_created), call_destroyed(0),
                  frame_created(_frame_created), frame_destroyed(0), glcount(0), eglcount(0), activations(0), swaps(0) {}
        std::map<int, bool> threads_used;
        std::map<int, bool> contexts_used; // by idx
    };
    std::vector<Surface> surfaces;
    std::map<std::string, Json::Value::Int64> tex_formats;
    std::map<std::string, Json::Value::Int64> tex_sizes;
    std::map<std::string, Json::Value::Int64> scissor_sizes;

public:
    AnalyzeTrace() : features(FEATURE_MAX) {}

    void analyze(ParseInterfaceRetracing &input);

private:
    void buffer_changed(GLenum target);
    void buffer_bound(GLenum target);
    Json::Value trace_json(ParseInterfaceRetracing& input);
    Json::Value frame_json(ParseInterfaceRetracing& input, int frame);
    Json::Value json_program(const StateTracker::Context& context, const StateTracker::Program& program, int frame);
    double calculate_heaviness(const ParseInterfaceRetracing& input, int frame);
    double calculate_dump_heaviness(const ParseInterfaceRetracing& input, int frame);
    double calculate_complexity(const ParseInterfaceRetracing& input);
    std::map<std::string, CallData> mCallData;
};

static void write_callstats(const std::map<std::string, CallData>& calldata, const std::string& basename)
{
    std::string filename = basename + "_callstats.csv";
    FILE *fp = fopen(filename.c_str(), "w");
    if (!fp)
    {
        DBG_LOG("Could not open %s for writing: %s\n", filename.c_str(), strerror(errno));
        return;
    }
    fprintf(fp, "Function, Cycles, Count, Average\n");
    for (auto& pair : calldata)
    {
        fprintf(fp, "%s, %ld, %ld, %ld\n", pair.first.c_str(), pair.second.cycles, pair.second.count, pair.second.cycles / pair.second.count);
    }
    fclose(fp);
}

// this is not 100% reliable - as the user could bind to another point to change it first
void AnalyzeTrace::buffer_changed(GLenum target)
{
    if (target == GL_ARRAY_BUFFER)
    {
        perdraw["vertex_buffer_changes"].values.back()++;
        perframe["vertex_buffer_changes"].values.back()++;
    }
    else if (target == GL_ELEMENT_ARRAY_BUFFER)
    {
        perdraw["index_buffer_changes"].values.back()++;
        perframe["index_buffer_changes"].values.back()++;
    }
    else if (target == GL_SHADER_STORAGE_BUFFER || target == GL_UNIFORM_BUFFER)
    {
        perdraw["uniform_buffer_changes"].values.back()++;
        perframe["uniform_buffer_changes"].values.back()++;
    }
}
void AnalyzeTrace::buffer_bound(GLenum target)
{
    perdraw["buffer_binding"].values.back()++;
    perframe["buffer_binding"].values.back()++;
    if (target == GL_ARRAY_BUFFER)
    {
        perdraw["vertex_buffer_binding"].values.back()++;
    }
    else if (target == GL_ELEMENT_ARRAY_BUFFER)
    {
        perdraw["index_buffer_binding"].values.back()++;
    }
    else if (target == GL_SHADER_STORAGE_BUFFER || target == GL_UNIFORM_BUFFER)
    {
        perdraw["uniform_buffer_binding"].values.back()++;
    }
}

void AnalyzeTrace::analyze(ParseInterfaceRetracing& input)
{
    common::CallTM *call = NULL;
    bool in_renderpass_frame = false;

    if (startframe == 0)
    {
        relative_frames++;
        startNewRows(perframe);
    }
    startNewRows(perdraw);
    calls_per_frame.push_back(0);
    if (renderpassframes.count(0))
    {
        input.dumpFrameBuffers(true); // start dumping FBOs for this frame
        input.setQuickMode(false); // start analysing index buffers
        in_renderpass_frame = true;
    }
    while ((call = input.next_call()))
    {
        const int context_index = input.context_index;

        dumpstream << "[t" << call->mTid << ":c" << ((context_index != UNBOUND) ? std::to_string(context_index) : std::string("-"))
                   << ":s" << ((input.surface_index != UNBOUND) ? std::to_string(input.surface_index) : std::string("-"));
        if (dumpCallTime)
        {
            dumpstream << ":" << std::setfill('0') << std::setw(10) << input.getCpuCycles();
        }
        dumpstream << "] ";
        if (!bareLog)
        {
            dumpstream << call->mCallNo << ": ";
        }
        dumpstream << call->ToStr(false) << std::endl;

        if (call->mCallName == "eglCreateWindowSurface" || call->mCallName == "eglCreateWindowSurface2"
            || call->mCallName == "eglCreatePbufferSurface" || call->mCallName == "eglCreatePixmapSurface")
        {
            int mret = call->mRet.GetAsInt();
            int display = call->mArgs[0]->GetAsInt();
            dumpstream << "    idx=" << surfaces.size() << std::endl;
            surfaces.push_back(Surface(mret, surfaces.size(), display, call->mCallNo, frames));
        }
        else if (call->mCallName == "eglCreateImage" || call->mCallName == "eglCreateImageKHR")
        {
            features[FEATURE_EGLCREATEIMAGE]++;
        }
        else if (call->mCallName == "glEGLImageTargetTexture2DOES") // size can be inferred, original format lost
        {
            const GLenum target = interpret_texture_target(call->mArgs[0]->GetAsUInt());
            const GLuint unit = contexts[context_index].activeTextureUnit;
            const GLuint tex_id = contexts[context_index].boundTextureIds[target][unit];
            contexts[context_index].textureIdCompressed[tex_id] = false;
            contexts[context_index].textures++;
            texturetypes[texEnum(target)]++;
            features[FEATURE_EGL_IMAGE]++;
            dumpstream << "    bound=" << tex_id << " active=" << unit << std::endl;
            DEBUG_LOG("Created EGL image texture %u on context %d with target 0x%04x\n", tex_id, context_index, target);
        }
        else if (call->mCallName == "eglCreateContext")
        {
            int mret = call->mRet.GetAsInt();
            int display = call->mArgs[0]->GetAsInt();
            int share = call->mArgs[2]->GetAsInt();
            dumpstream << "    idx=" << contexts.size() << std::endl;
            contexts.push_back(Context(mret, contexts.size(), display, share));
        }
        else if (call->mCallName == "eglDestroySurface")
        {
            int surface = call->mArgs[1]->GetAsInt();
            int target_surface_index = input.surface_remapping[surface];
            surfaces[target_surface_index].call_destroyed = call->mCallNo;
            surfaces[target_surface_index].frame_destroyed = frames;
            dumpstream << "    call created=" << surfaces[target_surface_index].call_created << " frames=" << frames - surfaces[target_surface_index].frame_created << std::endl;
        }
        else if (call->mCallName == "eglDestroyContext")
        {
            int context = call->mArgs[1]->GetAsInt();
            int target_context_index = input.context_remapping[context];
            dumpstream << "    call created=" << input.contexts[target_context_index].call_created << " frames=" << frames - input.contexts[target_context_index].frame_created << std::endl;
        }
        else if (call->mCallName == "eglMakeCurrent")
        {
            int surface = call->mArgs[1]->GetAsInt();
            int context = call->mArgs[3]->GetAsInt();
            int new_context_index = input.context_remapping[context];
            if (surface != (int64_t)EGL_NO_SURFACE)
            {
                int new_surface_index = input.surface_remapping[surface];
                surfaces[new_surface_index].activations++;
                surfaces[new_surface_index].eglcount++;
                surfaces[new_surface_index].threads_used[call->mTid] = true;
                surfaces[new_surface_index].contexts_used[new_context_index] = true;
                if (surfaces[new_surface_index].call_destroyed != 0)
                {
                    DBG_LOG("Warning: eglMakeCurrent on destroyed thread idx %d, id %u at call %u!\n", new_surface_index, surfaces[new_surface_index].id, call->mCallNo);
                }
            }
        }
        else if (call->mCallName.compare(0, 14, "eglSwapBuffers") == 0)
        {
            int surface = call->mArgs[1]->GetAsInt();
            int target_surface_index = input.surface_remapping[surface];
            surfaces[target_surface_index].swaps++;
            frames++;
            for (auto& c : contexts) // handle texture counting
            {
                // patrace defines 'frame' as any eglSwapBuffers on any context, so count all contexts
                // TODO, handle shared contexts. For now, they will be counted as uncompressed.
                for (auto& t : c.textureIdUsed)
                {
                    if (c.textureIdCompressed.count(t) == 0)
                    {
                        DBG_LOG("Texture ID %d on context ID=%d has been used, but never registered (call=%d)!\n", t, c.id, (int)call->mCallNo);
                    }
                    bool compressed = c.textureIdCompressed[t];
                    if (relevant(frames))
                    {
                        perframe["compressed_textures"].values.back() += compressed ? 1 : 0;
                        perframe["uncompressed_textures"].values.back() += compressed ? 0 : 1;
                    }
                }
                c.textureIdUsed.clear();
            }
            if (relevant(frames))
            {
                relative_frames++;
                startNewRows(perframe);
            }
            calls_per_frame.push_back(0);

            if (in_renderpass_frame) // previous frame was in our renderpass list
            {
                write_CSV((dump_csv_filename.empty() ? "frame" : dump_csv_filename) + "_draws_f" + std::to_string(frames - 1), perdraw, true);
                input.setQuickMode(true);
                heavinesses[frames - 1] = calculate_dump_heaviness(input, frames - 1);
            }

            if (renderpassframes.count(frames))
            {
                input.dumpFrameBuffers(true); // start dumping FBOs for this frame
                input.setQuickMode(false); // start analysing index buffers
                in_renderpass_frame = true;
            }
            else if (in_renderpass_frame)
            {
                input.dumpFrameBuffers(false);
                in_renderpass_frame = false;
            }

            if (frames > lastframe)
            {
                break; // stop parsing file here
            }

            perdraw = createPerDraw();
            startNewRows(perdraw);
        }
        /// --- end EGL ---
        else if (context_index == UNBOUND)
        {
            // nothing, just prevent the GLES calls below from being processed without a GLES context
        }
        /// --- start GLES ---
        else if (call->mCallName == "glDiscardFramebufferEXT")
        {
            features[FEATURE_DISCARD_FBO]++;
        }
        else if (call->mCallName == "glInvalidateFramebuffer")
        {
            features[FEATURE_INVALIDATE_FBO]++;
        }
        else if (call->mCallName == "glBlendEquation")
        {
            int mode = call->mArgs[0]->GetAsInt();
            contexts[context_index].blendMode.setEquation(mode, mode);
        }
        else if (call->mCallName == "glBlendEquationSeparate")
        {
            int modeRGB = call->mArgs[0]->GetAsInt();
            int modeAlpha = call->mArgs[1]->GetAsInt();
            contexts[context_index].blendMode.setEquation(modeRGB, modeAlpha);
        }
        else if (call->mCallName == "glBlendFunc")
        {
            int sfactor = call->mArgs[0]->GetAsInt();
            int dfactor = call->mArgs[1]->GetAsInt();
            contexts[context_index].blendMode.setFunction(sfactor, dfactor, sfactor, dfactor);
        }
        else if (call->mCallName == "glBlendFuncSeparate")
        {
            int sfactorRGB = call->mArgs[0]->GetAsInt();
            int dfactorRGB = call->mArgs[1]->GetAsInt();
            int sfactorAlpha = call->mArgs[2]->GetAsInt();
            int dfactorAlpha = call->mArgs[3]->GetAsInt();
            contexts[context_index].blendMode.setFunction(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
        }
        else if (call->mCallName == "glDepthFunc")
        {
            GLenum func = call->mArgs[0]->GetAsUInt();
            depthfuncs[func] = true;
        }
        else if (call->mCallName == "glGenProgramPipelines")
        {
            features[FEATURE_SEPARATE_SHADER_OBJECTS] += call->mArgs[0]->GetAsInt();
        }
        else if (call->mCallName == "glLinkProgram" && relevant(frames))
        {
            perframe["programs_linked"].values.back()++;
        }
        else if (call->mCallName.find("glTexImage") != std::string::npos
                 || call->mCallName.find("glTexStorage") != std::string::npos
                 || call->mCallName.find("glCompressedTexImage") != std::string::npos)
        {
            const GLenum target = interpret_texture_target(call->mArgs[0]->GetAsUInt());
            int level = 0;
            const GLuint unit = contexts[context_index].activeTextureUnit;
            const GLuint tex_id = contexts[context_index].boundTextureIds[target][unit];
            dumpstream << "    bound=" << tex_id << " active=" << unit << std::endl;
            DEBUG_LOG("Created texture %u on context %d with target 0x%04x\n", tex_id, context_index, target);
            const int x = call->mArgs[3]->GetAsInt();
            std::string size;
            std::string format = SafeEnumString(call->mArgs[2]->GetAsUInt(), call->mCallName.c_str());
            if (format == "Unknown")
            {
                DBG_LOG("Could not decipher texture format 0x%04x\n", (unsigned)call->mArgs[2]->GetAsUInt());
            }
            int type_param = -1;

            if (call->mCallName.find("1D") != std::string::npos) // 1D
            {
                size = std::to_string(x);
                type_param = 6;
            }
            else if (call->mCallName.find("2D") != std::string::npos) // 2D
            {
                const int y = call->mArgs[4]->GetAsInt();
                size = std::to_string(x) + "x" + std::to_string(y);
                type_param = 7;
            }
            else if (call->mCallName.find("3D") != std::string::npos) // is 3D
            {
                const int y = call->mArgs[4]->GetAsInt();
                const int z = call->mArgs[5]->GetAsInt();
                size = std::to_string(x) + "x" + std::to_string(y) + "x" + std::to_string(z);
                type_param = 8;
            }
            else
            {
                DBG_LOG("%s texture function not handled!\n", call->mCallName.c_str());
            }

            if (call->mCallName.find("glTexImage") != std::string::npos && type_param != -1)
            {
                level = call->mArgs[1]->GetAsInt();
                // internal format may have to be derived from input format
                format = SafeEnumString(merge_texture_format(call->mArgs[2]->GetAsUInt(), call->mArgs[type_param]->GetAsUInt()), call->mCallName.c_str());
                if (format == "Unknown")
                {
                    DBG_LOG("Could not decipher texture format 0x%04x merged with type 0x%04x (type@%d)\n", (unsigned)call->mArgs[2]->GetAsUInt(), (unsigned)call->mArgs[type_param]->GetAsUInt(), type_param);
                }
                contexts[context_index].textureIdCompressed[tex_id] = false;
            }
            else if (call->mCallName.find("glTexStorage") != std::string::npos)
            {
                const bool compressed = isCompressedFormat(call->mArgs[2]->GetAsUInt());
                contexts[context_index].textureIdCompressed[tex_id] = compressed;
                contexts[context_index].compressed_textures += compressed ? 1 : 0;
            }
            else // compressed variant
            {
                contexts[context_index].compressed_textures++;
                contexts[context_index].textureIdCompressed[tex_id] = true;
            }

            if (level == 0) // do not count uploading individual mipmaps as separate textures
            {
                contexts[context_index].textures++;
                tex_formats[format]++;
                tex_sizes[size]++;
                texturetypes[texEnum(target)]++;
            }
        }
        else if (call->mCallName == "glVertexAttribBinding")
        {
            features[FEATURE_VERTEX_ATTR_BINDING] = true;
        }
        else if (call->mCallName == "glBindVertexBuffer" && relevant(frames))
        {
            features[FEATURE_VERTEX_BUFFER_BINDING] = true;
            if (relevant(frames))
            {
                perdraw["buffer_binding"].values.back()++;
                perdraw["vertex_buffer_binding"].values.back()++;
                perframe["buffer_binding"].values.back()++;
            }
        }
        else if (call->mCallName == "glCreateShader" || call->mCallName == "glCreateShaderProgramv")
        {
            const int mret = call->mRet.GetAsInt();
            const int type = call->mArgs[0]->GetAsInt();
            contexts[context_index].shadertypes[mret] = type;
            shaders[type]++;
        }
        else if (call->mCallName == "glShaderSource")
        {
            const GLuint shader = call->mArgs[0]->GetAsUInt();
            const GLenum type = contexts[context_index].shadertypes[shader];
            if (!compiler.empty())
            {
                std::string shtype;
                switch (type)
                {
                case GL_VERTEX_SHADER: shtype = "--vert"; break;
                case GL_FRAGMENT_SHADER: shtype = "--frag"; break;
                case GL_COMPUTE_SHADER: shtype = "--comp"; break;
                case GL_GEOMETRY_SHADER: shtype = "--geom"; break;
                case GL_TESS_CONTROL_SHADER: shtype = "--tesc"; break;
                case GL_TESS_EVALUATION_SHADER: shtype = "--tese"; break;
                default:
                    std::cerr << "Unknown shader type: " << type << std::endl;
                    exit(1);
                    break;
                }
                std::string tmpname = tmpnam(nullptr); // make sure we can run multiple analyze_trace in parallel
                std::string tmpout = tmpnam(nullptr); // yes, yes... I know "tmpnam is dangerous"...
                FILE *fp = fopen(tmpname.c_str(), "w");
                if (!fp)
                {
                    fprintf(stderr, "Failed to open temporary output file %s: %s\n", tmpname.c_str(), strerror(errno));
                    exit(1);
                }
                for (unsigned i = 0; i < call->mArgs[2]->mArrayLen; i++)
                {
                    std::string source = call->mArgs[2]->mArray[i].GetAsString();
                    fwrite(source.c_str(), source.size(), 1, fp);
                }
                fclose(fp);
                std::string command = compiler + " -V --core=Mali-G71 --revision=r0p0 " + shtype + + " -o " + tmpout + " " + tmpname;
                unlink(tmpout.c_str());
                char line[PATH_MAX];
                FILE *result = popen(command.c_str(), "r");
                if (!result)
                {
                    std::cerr << "Failed to open result pipe: " << strerror(errno) << std::endl;
                    exit(1);
                }
                ShaderState s;
                while (fgets(line, PATH_MAX, result) != NULL)
                {
                    int v;
                    char tmp[10];

                    if (sscanf(line, "arithmetic_cycles = %d", &v) == 1)
                    {
                        shadercycles[type].a += v;
                        s.arithmetic = v;
                    }
                    else if (sscanf(line, "load_store_cycles = %d", &v) == 1)
                    {
                        shadercycles[type].l += v;
                        s.loadstore = v;
                    }
                    else if (sscanf(line, "texture_cycles = %d", &v) == 1)
                    {
                        shadercycles[type].t += v;
                        s.texturing = v;
                    }
                    else if (sscanf(line, "spilling_used = %s", tmp) == 1)
                    {
                        s.spilling = (strcmp(tmp, "true") == 0);
                    }
                    else if (sscanf(line, "uniform_registers_used = %d", &v) == 1)
                    {
                        s.uniform_registers = v;
                    }
                }
                const int shaderidx = input.contexts[context_index].shader_remapping[shader];
                contexts[context_index].shaders[shaderidx] = s;
                pclose(result);
            }
        }
        else if (call->mCallName == "glBindFramebuffer")
        {
            GLenum target = call->mArgs[0]->GetAsUInt();
            GLuint fb = call->mArgs[1]->GetAsUInt();
            bool valid;
            if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER)
            {
                contexts[context_index].drawframebuffer = fb;
                // Setting it to the same value as the old value does not count
                valid = (input.contexts[context_index].drawframebuffer != input.contexts[context_index].prev_drawframebuffer);
            }
            else
            {
                valid = (input.contexts[context_index].readframebuffer != input.contexts[context_index].prev_readframebuffer);
            }
            if (fb != 0 && relevant(frames) && valid)
            {
                perframe["framebuffers"].values.back()++;
            }
        }
        else if (call->mCallName == "glFramebufferTexture2D" || call->mCallName == "glFramebufferTextureLayer"
                 || call->mCallName == "glFramebufferRenderbuffer" || call->mCallName == "glFramebufferTexture2DOES"
                 || call->mCallName == "glFramebufferTextureLayerOES" || call->mCallName == "glFramebufferRenderbufferOES")
        {
            GLenum target = call->mArgs[0]->GetAsUInt();
            GLenum attachment = call->mArgs[1]->GetAsUInt();
            GLuint fb = 0;
            GLuint id = 0;

            if (call->mCallName == "glFramebufferTextureLayer")
            {
                id = call->mArgs[2]->GetAsUInt();
            }
            else
            {
                id = call->mArgs[3]->GetAsUInt();
            }

            if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER)
            {
                fb = input.contexts[context_index].drawframebuffer;
            }
            else
            {
                fb = input.contexts[context_index].readframebuffer;
            }

            if (fb != 0 && relevant(frames) && id != 0)
            {
                const int fbo_idx = input.contexts[context_index].framebuffer_remapping[input.contexts[context_index].drawframebuffer];
                const StateTracker::Framebuffer& framebuffer = input.contexts[context_index].framebuffers[fbo_idx];
                // If the framebuffer attachments are modified in the same frame as they have been drawn to, then we consider it 'reused'.
                if (framebuffer.used && framebuffer.last_used == frames)
                {
                    features[FEATURE_FBO_REUSE]++;
                }
                perframe["fbo_attachments"].values.back()++;
            }
            (void)attachment; // silence compiler
        }
        else if ((call->mCallName == "glFinish" || call->mCallName == "glFlush") && relevant(frames))
        {
           perframe["flushes"].values.back()++;
           flushes++;
        }
        else if (call->mCallName == "glGenFramebuffers")
        {
            contexts[context_index].framebuffers += call->mArgs[0]->GetAsUInt();
        }
        else if (relevant(frames) && (call->mCallName == "glFramebufferRenderbuffer" || call->mCallName == "glFramebufferTexture2D"))
        {
            const unsigned attachment = call->mArgs[1]->GetAsUInt();
            if (attachment >= GL_COLOR_ATTACHMENT0 && attachment <= GL_COLOR_ATTACHMENT31)
            {
                numColorAttachments = std::max<unsigned>(numColorAttachments, attachment - GL_COLOR_ATTACHMENT0 + 1);
            }
        }
        else if (call->mCallName == "glActiveTexture")
        {
            GLuint unit = call->mArgs[0]->GetAsUInt() - GL_TEXTURE0;
            contexts[context_index].activeTextureUnit = unit;
            if (relevant(frames))
            {
                highestTextureUnitUsed = std::max<GLuint>(highestTextureUnitUsed, unit);
            }
        }
        else if (call->mCallName == "glUseProgram")
        {
            const GLuint id = call->mArgs[0]->GetAsUInt();
            if (id != 0 && relevant(frames))
            {
                perdraw["program_binding"].values.back()++;
                perframe["program_binding"].values.back()++;
            }
        }
        else if (call->mCallName == "glBindTexture")
        {
            const unsigned unit = contexts[context_index].activeTextureUnit;
            const unsigned target = call->mArgs[0]->GetAsUInt();
            const unsigned tex_id = call->mArgs[1]->GetAsUInt();
            contexts[context_index].boundTextureIds[target][unit] = tex_id;
            dumpstream << "    active=" << unit << std::endl;
            if (tex_id != 0 && relevant(frames))
            {
                perdraw["texture_binding"].values.back()++;
                perframe["texture_binding"].values.back()++;
            }
        }
        else if (call->mCallName == "glMapBuffer" || call->mCallName == "glMapBufferRange")
        {
            const GLenum target = call->mArgs[0]->GetAsUInt();
            unsigned access = 0;

            if (call->mCallName == "glMapBuffer")
            {
                access = call->mArgs[1]->GetAsUInt();
            }
            else
            {
                access = call->mArgs[3]->GetAsUInt();
            }

            if ((access | GL_MAP_WRITE_BIT) && relevant(frames))
            {
                buffer_changed(target);
            }
        }
        else if (relevant(frames) && (call->mCallName == "glBufferData" || call->mCallName == "glBufferSubData"))
        {
            const GLenum target = call->mArgs[0]->GetAsUInt();
            buffer_changed(target);
        }
        else if (relevant(frames) && call->mCallName == "glCopyBufferSubData")
        {
            // This is likely to use GL_COPY_WRITE_BUFFER for target, which will not tell us anything useful,
            // but we try looking anyway.
            const GLenum writetarget = call->mArgs[1]->GetAsUInt();
            buffer_changed(writetarget);
        }
        else if (call->mCallName == "glVertexAttribIPointer" || call->mCallName == "glVertexAttribPointer")
        {
            if (last_changed_vertex_buffer == -1)
            {
                if (relevant(frames))
                {
                    buffer_changed(GL_ARRAY_BUFFER);
                }
                last_changed_vertex_buffer = input.contexts[context_index].boundBufferIds[GL_ARRAY_BUFFER][0].buffer;
            }
        }
        else if (call->mCallName == "glBindBuffer" || call->mCallName == "glBindBufferBase" || call->mCallName == "glBindBufferRange")
        {
            GLenum target = call->mArgs[0]->GetAsInt();
            GLenum bufferid;
            if (call->mCallName == "glBindBuffer")
            {
                bufferid = call->mArgs[1]->GetAsInt();
            }
            else
            {
                bufferid = call->mArgs[2]->GetAsInt();
            }
            switch (target)
            {
            case GL_SHADER_STORAGE_BUFFER: if (relevant(frames)) features[FEATURE_SSBO] = true; break;
            case GL_UNIFORM_BUFFER: if (relevant(frames)) features[FEATURE_UBO] = true; break;
            case GL_TRANSFORM_FEEDBACK_BUFFER: if (relevant(frames)) features[FEATURE_TF] = true; break;
            case GL_ARRAY_BUFFER: // never used with glBindBuffer{Base|Range}
                contexts[context_index].curbuffer = call->mArgs[1]->GetAsUInt();
                last_changed_vertex_buffer = -1;
                break;
            default: break;
            }
            if (bufferid != 0 && relevant(frames))
            {
                buffer_bound(target);
            }
        }
        else if (call->mCallName.find("glUniform") != std::string::npos || call->mCallName.find("glProgramUniform") != std::string::npos)
        {
            GLint location = 0;
            GLuint program_id = 0;
            int program_index = UNBOUND;

            if (call->mCallName.find("glUniform") != std::string::npos)
            {
                program_index = input.contexts[context_index].program_index;
                if (program_index != UNBOUND)
                {
                    program_id = input.contexts[context_index].programs[program_index].id;
                    location = call->mArgs[0]->GetAsInt();
                }
            }
            else // glProgramUniform*()
            {
                program_id = call->mArgs[0]->GetAsUInt();
                if (input.contexts[context_index].program_remapping.count(program_id) > 0)
                {
                    program_index = input.contexts[context_index].program_remapping[program_id];
                    location = call->mArgs[1]->GetAsInt();
                }
            }
            if (program_index != UNBOUND)
            {
                StateTracker::Program &p = input.contexts[context_index].programs[program_index];
                dumpstream << "    program=" << program_id << " name=" << p.uniformNames[location] << std::endl;
            }
            else
            {
                dumpstream << "    program=ERROR name=ERROR" << std::endl;
            }
            if (relevant(frames))
            {
                const int count = count_uniform_values(call);
                perdraw["uniforms"].values.back()++;
                perdraw["uniform_values"].values.back() += count;
                perframe["uniforms"].values.back()++;
                perframe["uniform_values"].values.back() += count;
            }
        }
        else if (call->mCallName == "glEnable")
        {
            GLenum target = call->mArgs[0]->GetAsInt();
            if (target == GL_PRIMITIVE_RESTART_FIXED_INDEX && relevant(frames))
            {
                features[FEATURE_PRIMRESTART] = true;
            }
        }
        else if (call->mCallName == "glVertexAttribPointer" && relevant(frames))
        {
            if (contexts[context_index].curbuffer != 0)
            {
                features[FEATURE_VBO]++;
            }
            else
            {
                features[FEATURE_VA]++;
            }
        }
        else if (relevant(frames) && (call->mCallName == "glGenVertexArrays" || call->mCallName == "glGenVertexArraysOES"))
        {
            features[FEATURE_VAO]++;
        }
        else if (relevant(frames) && (call->mCallName == "glBindVertexArray" || call->mCallName == "glBindVertexArrayOES"))
        {
            perframe["vao_binding"].values.back()++;
        }
        else if (call->mCallName == "glClientSideBufferData" && relevant(frames))
        {
            clientsidebuffersize += call->mArgs[1]->GetAsUInt();
            clientsidebuffers = std::max<int>(clientsidebuffers, call->mArgs[0]->GetAsInt() + 1);
        }
        else if (call->mCallName == "glProgramBinary" || call->mCallName == "glProgramBinaryOES")
        {
            // binaryFormat=0xDEADDEAD means we're messing with the driver to make this feature fail
            GLenum binaryFormat = call->mArgs[1]->GetAsUInt();
            if (binaryFormat != 0xdeaddead)
            {
                features[FEATURE_BINARY_SHADERS]++;
            }
        }
        else if (call->mCallName.compare(0, 10, "glDispatch") == 0 && relevant(frames))
        {
            compute++;
            perframe["compute"].values.back()++;
            if (call->mCallName == "glDispatchComputeIndirect")
            {
                features[FEATURE_INDIRECT_COMPUTE]++;
            }
        }
        else if (call->mCallName == "glDrawTexiOES")
        {
            // GLES1 texture to screen blitting extension
        }
        else if (call->mCallName.compare(0, 6, "glDraw") == 0 && call->mCallName != "glDrawBuffers" && relevant(frames))
        {
            StateTracker::Framebuffer* writeframebuffer = nullptr;

            if (input.contexts[context_index].drawframebuffer != 0)
            {
                int fbo_idx = input.contexts[context_index].framebuffer_remapping[input.contexts[context_index].drawframebuffer];
                writeframebuffer = &input.contexts[context_index].framebuffers[fbo_idx];
            }

            dumpstream << "    ";
            GLenum mode = call->mArgs[0]->GetAsUInt();
            perframe["draws.total"].values.back()++;
            perdraw["renderpass"].values.back() = input.contexts[context_index].render_passes.size();
            if (input.contexts[context_index].enabled[GL_PRIMITIVE_RESTART_FIXED_INDEX])
            {
                perframe["draws.primitive_restart"].values.back()++;
                dumpstream << "primitive_restart ";
            }
            if (input.contexts[context_index].enabled[GL_DITHER])
            {
                dither++;
                perframe["draws.dither"].values.back()++;
                dumpstream << "dither ";
            }
            if (input.contexts[context_index].enabled[GL_BLEND])
            {
                blended++;
                perframe["draws.blended"].values.back()++;
                blendModes[contexts[context_index].blendMode]++;
                dumpstream << "blended ";
            }
            if (input.contexts[context_index].enabled[GL_SCISSOR_TEST]
                && input.contexts[context_index].fillstate.scissor.width > 0
                && input.contexts[context_index].fillstate.scissor.height > 0)
            {
                dumpstream << "scissored(" << input.contexts[context_index].fillstate.scissor.width << "x" << input.contexts[context_index].fillstate.scissor.height << ") ";
                std::string size = std::to_string(input.contexts[context_index].fillstate.scissor.width) + "x" + std::to_string(input.contexts[context_index].fillstate.scissor.height);
                scissor_sizes[size]++;
            }
            if (input.contexts[context_index].enabled[GL_DEPTH_TEST])
            {
                depthtested++;
                perframe["draws.depthtested"].values.back()++;
                dumpstream << "depthtested ";
                if (input.contexts[context_index].fillstate.depthmask
                    && (input.contexts[context_index].drawframebuffer == 0
                        || writeframebuffer->attachments.count(GL_DEPTH_ATTACHMENT) > 0))
                {
                    depthwriting++;
                    perframe["draws.depthwriting"].values.back()++;
                    dumpstream << "depthwriting ";
                }
            }
            if (input.contexts[context_index].enabled[GL_STENCIL_TEST])
            {
                stenciled++;
                perframe["draws.stenciled"].values.back()++;
                dumpstream << "stenciled ";
            }
            if (input.contexts[context_index].enabled[GL_CULL_FACE])
            {
                faceculled++;
                perframe["draws.faceculled"].values.back()++;
                dumpstream << "facefulled ";
            }
            if (call->mCallName.find("Indirect") != std::string::npos)
            {
                features[FEATURE_INDIRECT]++;
                drawtypesindirect[mode]++;
                perframe["draws.indirect"].values.back()++;
            }

            const DrawParams params = input.getDrawCallCount(call);
            drawtypescount[mode] += params.vertices;
            drawtypes[mode]++;
            if (renderpassframes.count(frames))
            {
                perdraw["primitives"].values.back() = params.primitives;
                perdraw["vertices"].values.back() = params.vertices;
                perdraw["vertices.unique"].values.back() = params.unique_vertices;
                perdraw["max_sparseness"].values.back() = params.max_sparseness;
                perdraw["avg_sparseness"].values.back() = params.avg_sparseness;
                perdraw["spatial_locality"].values.back() = params.spatial_locality * 100.0;
                perdraw["temporal_locality"].values.back() = params.temporal_locality * 100.0;
                perdraw["vec4_locality"].values.back() = params.vec4_locality * 100.0;
                perdraw["instancing"].values.back() = params.instances;
                startNewRows(perdraw);
            }
            drawtypesprimitives[mode] += params.primitives;
            perframe["vertices"].values.back() += params.vertices;
            perframe["instancing"].values.back() += params.instances;
            perframe["primitives"].values.back() += params.primitives;

            if (call->mCallName.find("Elements") != std::string::npos)
            {
                indexed++;
                perframe["draws.indexed"].values.back()++;
                perframe["vertices.indexed"].values.back() += params.vertices;
                perframe["vertices.unique"].values.back() += params.unique_vertices;
            }

            if (params.instances > 1)
            {
                features[FEATURE_INSTANCING]++;
                perframe["draws.instanced"].values.back()++;
            }

            if (input.contexts[context_index].enabled[GL_POLYGON_OFFSET_FILL])
            {
                features[FEATURE_POLYGON_OFFSET_FILL]++;
                perframe["draws.polygonoffset"].values.back()++;
            }

            // figure out used texture types
            const int program_index = input.contexts[context_index].program_index;
            if (program_index >= 0) // will be zero for GLES1
            {
                dumpstream << std::endl << "    ";
                ProgramState p = getProgramInfo(input.contexts[context_index].programs[program_index].id);
                if (contexts[context_index].programs.count(program_index) == 0)
                {
                    contexts[context_index].programs[program_index] = p;
                }
                for (const ProgramState::ProgramSampler& s : p.texture_bindings)
                {
                    GLenum binding = samplerTypeToBindingType(s.type);
                    if (binding == GL_NONE)
                    {
                        DBG_LOG("Could not map sampler %s of type %04x at call %d\n", s.name.c_str(), s.type, (int)call->mCallNo);
                        continue;
                    }
                    else if (input.contexts[context_index].textureUnits.count(s.value) == 0
                        || input.contexts[context_index].textureUnits[s.value].count(binding) == 0) // sanity check
                    {
                        DBG_LOG("Very strange texture binding for sampler %s at call %d\n", s.name.c_str(), (int)call->mCallNo);
                        continue;
                    }
                    const GLuint texture_id = input.contexts[context_index].textureUnits[s.value][binding];
                    contexts[context_index].textureIdUsed.insert(texture_id);
                    dumpstream << "TextureUnit:" << s.value << "(" + texEnum(binding) + ") = TexureId:" << texture_id << ",SamplerId:"
                               << input.contexts[context_index].sampler_binding[s.value] << std::endl << "    ";
                    StateTracker::SamplerState sampler;
                    const GLuint samplerObject = input.contexts[context_index].sampler_binding[s.value];
                    if (samplerObject) // do we have a sampler object bound?
                    {
                        const int idx = input.contexts[context_index].sampler_remapping[samplerObject];
                        sampler = input.contexts[context_index].samplers[idx].state;
                    }
                    else if (texture_id != 0) // if not, use texture's sampler state
                    {
                        const int idx = input.contexts[context_index].texture_remapping[texture_id];
                        sampler = input.contexts[context_index].textures[idx].state;
                    }
                    if (texture_id != 0)
                    {
                        texturetypefilters[texEnum(binding)][texEnum(sampler.min_filter)]++;
                        const int idx = input.contexts[context_index].texture_remapping[texture_id];
                        StateTracker::Texture& texture = input.contexts[context_index].textures[idx];
                        texturetypesizes[texEnum(binding)][std::to_string(texture.width) + "x" + std::to_string(texture.height) + "x" + std::to_string(texture.depth)]++;
                        perframe["textures"].values.back()++;
                        perdraw["textures"].values.back()++;
                        if (input.contexts[context_index].render_passes.size() > 0 && input.contexts[context_index].render_passes.back().active
                            && renderpassframes.count(frames))
                        {
                            textures_by_renderpass[input.contexts[context_index].render_passes.back().index].insert(texture_id);
                        }
                        texture.used_min_filters.insert(sampler.min_filter);
                        texture.used_mag_filters.insert(sampler.mag_filter);
                    }
                }
            }
            if (input.contexts[context_index].render_passes.size() > 0 && input.contexts[context_index].render_passes.back().active)
            {
                dumpstream << "renderpass=" << input.contexts[context_index].render_passes.back().index << " ";
            }
            dumpstream << "drawfb=" << input.contexts[context_index].drawframebuffer << " ";
            dumpstream << "readfb=" << input.contexts[context_index].readframebuffer << " ";
            dumpstream << "program=" << (program_index >= 0 ? input.contexts[context_index].programs[program_index].id : 0) << " ";
            dumpstream << "drawcall=" << drawcalls << " ";
            dumpstream << "frame=" << frames << " ";
            dumpstream << std::endl;
            drawcalls++;
        }
        else if ((call->mCallName == "glClear" || call->mCallName.compare(0, 13, "glClearBuffer") == 0) && relevant(frames))
        {
            clears++;
            perframe["clears"].values.back()++;
        }

        if (surfaces.size() > static_cast<unsigned>(input.surface_index) && input.surface_index != UNBOUND)
        {
            surfaces[input.surface_index].threads_used[call->mTid] = true;
            if (call->mCallName.compare(0, 2, "gl") == 0)
            {
                surfaces[input.surface_index].glcount++;
            }
            else if (call->mCallName.compare(0, 3, "egl") == 0)
            {
                surfaces[input.surface_index].eglcount++;
            }
        }

        calls_per_frame.back()++;
        if (relevant(frames))
        {
            calls++;
            CallData& callData = mCallData[call->mCallName];
            callData.cycles += input.getCpuCycles();
            callData.count++;
            perframe["calls"].values.back()++;
        }
    }

    for (auto& c : input.contexts)
    {
        for (auto& p : c.programs)
        {
            if (!p.stats.drawcalls && !p.stats.dispatches)
            {
                continue; // skip unused programs
            }
            features[FEATURE_GEOMETRY_SHADER] += p.shaders.count(GL_GEOMETRY_SHADER);
            features[FEATURE_TESSELLATION] += p.shaders.count(GL_TESS_CONTROL_SHADER);
            features[FEATURE_COMPUTE_SHADER] += p.shaders.count(GL_COMPUTE_SHADER);
        }
    }

    // Now generate reports
    if (complexity_only_mode)
    {
        printf("%.04f\n", calculate_complexity(input));
        return;
    }

    for (int frame : renderpassframes)
    {
        Json::Value result = frame_json(input, frame);
        std::string filename = dump_csv_filename.empty() ? "frame_info" : dump_csv_filename;
        filename += "_f" + std::to_string(frame) + ".json";
        std::fstream fs;
        fs.open(filename, std::fstream::out |  std::fstream::trunc);
        fs << result.toStyledString();
        fs.close();
    }
    // JSON
    Json::Value result = trace_json(input);
    std::fstream fs;
    std::string filename = dump_csv_filename.empty() ? "trace" : dump_csv_filename;
    fs.open(filename + ".json", std::fstream::out |  std::fstream::trunc);
    fs << result.toStyledString();
    fs.close();
    // CSV
    write_CSV(dump_csv_filename.empty() ? "trace" : dump_csv_filename, perframe, true);
    // Dump out callstats
    write_callstats(mCallData, dump_csv_filename.empty() ? "trace" : dump_csv_filename);
}

static double ratio_with_cap(long limit, long value)
{
    return std::min<double>(value, limit) / static_cast<double>(limit);
}

// Calculates how heavy it is to run up to and including this frame
double AnalyzeTrace::calculate_dump_heaviness(const ParseInterfaceRetracing& input, int frame)
{
    const double w_ca = ratio_with_cap(7500000, calls);
    long _pr = 0;
    long _dr = 0;
    for (const auto& c : input.contexts)
    {
        for (const auto& p : c.programs)
        {
            _pr += p.stats.primitives;
            _dr += p.stats.drawcalls;
        }
    }
    const double w_dr = ratio_with_cap(500000, _dr);
    const double w_pr = ratio_with_cap(150000000, _pr);
    long _px = 0;
    for (const auto& thread : input.header["threads"])
    {
        if (thread["id"].asUInt() == input.defaultTid)
        {
            _px = thread["winH"].asInt() * thread["winW"].asInt();
        }
    }
    const double w_px = ratio_with_cap(3840 * 2160, (_px / 200) * frame); // top at 4k resolution at 200 frames
    //printf("%d :dump: calls=%f(%ld) draws=%f(%ld=%ld) prims=%f(%ld) pixels=%f(%ld) sum=%f\n", frame, w_ca, (long)calls, w_dr, drawcalls, _dr, w_pr, _pr, w_px, _px, (w_ca + w_dr + w_pr + w_px) / 4.0);
    return (w_ca + w_dr + w_pr + w_px) / 4.0;
}

// Calculates how heavy it is to run this frame alone
double AnalyzeTrace::calculate_heaviness(const ParseInterfaceRetracing& input, int frame)
{
    const double w_ca = ratio_with_cap(10000, calls_per_frame.at(frame));
    long _pr = 0;
    long _dr = 0;
    for (const auto& c : input.contexts)
    {
        for (const auto& p : c.programs)
        {
            if (p.stats_per_frame.count(frame) > 0)
            {
                _pr += p.stats_per_frame.at(frame).primitives;
                _dr += p.stats_per_frame.at(frame).drawcalls;
            }
        }
    }
    const double w_dr = ratio_with_cap(1000, _dr);
    const double w_pr = ratio_with_cap(1000000, _pr);
    long _px = 0;
    for (const auto& thread : input.header["threads"])
    {
        if (thread["id"].asUInt() == input.defaultTid)
        {
            _px = thread["winH"].asInt() * thread["winW"].asInt();
        }
    }
    const double w_px = ratio_with_cap(3840 * 2160, _px); // top at 4k resolution
    //printf("%d :sep: calls=%f(%ld) draws=%f(%ld) prims=%f(%ld) pixels=%f(%ld) sum=%f\n", frame, w_ca, (long)calls_per_frame.at(frame), w_dr, _dr, w_pr, _pr, w_px, _px, (w_ca + w_dr + w_pr + w_px) / 4.0);
    return (w_ca + w_dr + w_pr + w_px) / 4.0;
}

double AnalyzeTrace::calculate_complexity(const ParseInterfaceRetracing& input)
{
    double feature_complexity = 0.0;
    for (unsigned i = 0; i < features.size(); i++)
    {
        if (features[i])
        {
            feature_complexity += complexity_feature_value.at(i);
        }
    }
    std::vector<double> weights;
    weights.push_back(ratio_with_cap(1000000, calls));
    int programs = 0;
    for (const auto& c : input.contexts)
    {
        programs += c.programs.size();
    }
    weights.push_back(ratio_with_cap(100, programs));
    weights.push_back(ratio_with_cap(10, texturetypes.size()));
    weights.push_back(ratio_with_cap(10, tex_formats.size()));
    unsigned so_far = weights.size();
    for (unsigned i = 0; i < so_far; i++)
    {
        weights.push_back(std::min(1.0, feature_complexity)); // so should be 50% of total weight
    }
    return std::accumulate(weights.begin(), weights.end(), 0.0) / static_cast<double>(weights.size());
}

static Json::Value json_base(const StateTracker::Resource& base)
{
    Json::Value json;
    json["name"] = base.id;
    json["index"] = base.index;
    json["frame_created"] = base.frame_created;
    json["call_created"] = base.call_created;
    if (base.frame_destroyed != -1)
    {
        json["frame_destroyed"] = base.frame_destroyed;
    }
    if (base.call_destroyed != -1)
    {
        json["call_destroyed"] = base.call_destroyed;
    }
    if (!base.label.empty())
    {
        json["label"] = base.label;
    }
    return json;
}

static Json::Value json_fb(const StateTracker::Context& c, int idx, int frame)
{
    const StateTracker::Framebuffer& f = c.framebuffers.at(idx);
    Json::Value fb = json_base(f);
    fb["attachment_calls"] = f.attachment_calls;
    fb["uses"] = f.used;
    if (frame != -1)
    {
        fb["duplicate_clears"] = f.duplicate_clears.at(frame);
        fb["total_clears"] = f.total_clears.at(frame);
    }
    fb["attachments"] = Json::arrayValue;
    for (const auto& pair : f.attachments)
    {
        Json::Value attachment;
        attachment["id"] = pair.second.id;
        attachment["index"] = pair.second.index;
        attachment["binding"] = texEnum(pair.first);
        attachment["type"] = texEnum(pair.second.type);
        if (pair.second.type == GL_RENDERBUFFER && pair.second.id > 0)
        {
            const StateTracker::Renderbuffer& rb = c.renderbuffers.at(pair.second.index);
            attachment["samples"] = (int)rb.samples;
            attachment["width"] = (int)rb.width;
            attachment["height"] = (int)rb.height;
            attachment["internal_format"] = texEnum(rb.internalformat);
        }
        else if (pair.second.id > 0)
        {
            const StateTracker::Texture& tex = c.textures.at(pair.second.index);
            attachment["levels"] = tex.levels;
            attachment["width"] = tex.width;
            attachment["height"] = tex.height;
            attachment["depth"] = tex.depth;
            attachment["internal_format"] = texEnum(tex.internal_format);
            {
                fb["attachments"].append(attachment);
            }
        }
    }
    return fb;
}

Json::Value AnalyzeTrace::json_program(const StateTracker::Context& context, const StateTracker::Program& program, int frame)
{
    const ProgramState& ps = contexts[context.index].programs[program.index];
    Json::Value json = json_base(program);
    if (ps.activeAtomicCounterBuffers > 0)
    {
        json["atomics"] = ps.activeAtomicCounterBuffers;
    }
    if (ps.activeTransformFeedbackVaryings > 0)
    {
        json["transformfeedbackvaryings"] = ps.activeTransformFeedbackVaryings;
    }
    if (ps.activeTransformFeedbackBuffers > 0)
    {
        json["transformfeedbackbuffers"] = ps.activeTransformFeedbackBuffers;
    }
    json["link_status"] = program.link_status;
    json["attributes"] = ps.activeAttributes;
    json["md5sum"] = program.md5sum;
    if (frame < 0)
    {
        json["vertices"] = (Json::Value::Int64)program.stats.vertices;
        json["primitives"] = (Json::Value::Int64)program.stats.primitives;
        json["compute_dispatches"] = program.stats.dispatches;
        json["draw_calls"] = (Json::Value::Int64)program.stats.drawcalls;
    }
    else
    {
        json["vertices"] = (Json::Value::Int64)program.stats_per_frame.at(frame).vertices;
        json["primitives"] = (Json::Value::Int64)program.stats_per_frame.at(frame).primitives;
        json["compute_dispatches"] = program.stats_per_frame.at(frame).dispatches;
        json["draw_calls"] = (Json::Value::Int64)program.stats_per_frame.at(frame).drawcalls;
    }
    json["uniforms"] = (int)program.uniformNames.size();
    if (ps.activeUniformBlocks > 0)
    {
        json["uniform_blocks"] = ps.activeUniformBlocks;
    }
    json["shaders"] = Json::arrayValue;
    json["shader_ids"] = Json::arrayValue;
    if (!compiler.empty())
    {
        json["arithmetic"] = Json::arrayValue;
        json["loadstore"] = Json::arrayValue;
        json["texturing"] = Json::arrayValue;
        json["spilling"] = Json::arrayValue;
        json["uniform_registers"] = Json::arrayValue;
    }
    json["samplers"] = Json::arrayValue;
    json["compile_status"] = Json::arrayValue;
    std::unordered_set<std::string> samplers;
    int lines = 0;
    int bytes = 0;
    for (const auto& pair : program.shaders)
    {
        const StateTracker::Shader& shader = context.shaders[pair.second];
        const ShaderState& s = contexts[context.index].shaders[shader.index];
        lines += std::count(shader.source_code.cbegin(), shader.source_code.cend(), '\n');
        bytes += shader.source_compressed.size();
        switch (pair.first)
        {
        case GL_FRAGMENT_SHADER:
            json["shaders"].append("fragment");
            json["varying_locations"] = shader.varying_locations_used;
            json["lowp_varyings"] = shader.lowp_varyings;
            json["mediump_varyings"] = shader.mediump_varyings;
            json["highp_varyings"] = shader.highp_varyings;
            break;
        case GL_VERTEX_SHADER: json["shaders"].append("vertex"); break;
        case GL_GEOMETRY_SHADER: json["shaders"].append("geometry"); break;
        case GL_TESS_EVALUATION_SHADER: json["shaders"].append("tess eval"); break;
        case GL_TESS_CONTROL_SHADER: json["shaders"].append("tess control"); break;
        case GL_COMPUTE_SHADER: json["shaders"].append("compute"); break;
        default: json["shaders"].append("unknown"); break;
        }
        if (!compiler.empty())
        {
            json["arithmetic"].append(s.arithmetic);
            json["loadstore"].append(s.loadstore);
            json["texturing"].append(s.texturing);
            json["spilling"].append(s.spilling);
            json["uniform_registers"].append(s.uniform_registers);
        }
        if (shader.contains_invariants) json["contains_invariants"] = true;
        if (shader.contains_optimize_off_pragma) json["contains_optimize_off_pragma"] = true;
        if (shader.contains_debug_on_pragma) json["contains_debug_on_pragma"] = true;
        if (shader.contains_invariant_all_pragma) json["contains_invariant_all_pragma"] = true;
        for (const auto& sampler : shader.samplers)
        {
            std::string precision = "highp ";
            if (sampler.second.precision == StateTracker::MEDIUMP) precision = "mediump ";
            else if (sampler.second.precision == StateTracker::LOWP) precision = "lowp ";
            std::string entry =  precision + EnumString(sampler.second.type, "glShaderSource");
            samplers.insert(entry);
        }
        json["shader_ids"].append(shader.id);
        json["compile_status"].append(shader.compile_status);
    }
    for (const std::string& s : samplers)
    {
        json["samplers"].append(s);
    }
    json["original_source_lines"] = lines;
    json["processed_source_bytes"] = bytes;
    return json;
}

Json::Value AnalyzeTrace::trace_json(ParseInterfaceRetracing& input)
{
    Json::Value result;
    result["source"] = input.filename;
    result["contexts"] = Json::arrayValue;
    result["complexity"] = calculate_complexity(input);
    result["extensions"] = Json::arrayValue;
    for (const std::string& str : input.used_extensions)
    {
        result["extensions"].append(str);
    }
    result["depthfuncs"] = Json::arrayValue;
    for (const auto pair : depthfuncs)
    {
        result["depthfuncs"].append(texEnum(pair.first));
    }
    result["gl_version"] = ((float)input.highest_gles_version) / 10.0;
    for (auto& c : input.contexts)
    {
        Json::Value v = json_base(c);
        std::unordered_map<GLenum, long> used_texture_target_types;
        std::unordered_map<GLenum, long> used_renderbuffer_target_types;
        for (const auto& r : c.render_passes)
        {
            for (const auto& tt : r.used_texture_targets)
            {
                const StateTracker::Texture& tex = c.textures.at(tt);
                used_texture_target_types[tex.internal_format]++;
            }
            for (const auto& tt : r.used_renderbuffers)
            {
                const StateTracker::Renderbuffer& rb = c.renderbuffers.at(tt);
                used_renderbuffer_target_types[rb.internalformat]++;
            }
        }
        Json::Value used_texture_target_typesv;
        for (const auto& pair : used_texture_target_types)
        {
            used_texture_target_typesv[texEnum(pair.first)] = (Json::Value::Int64)pair.second;
        }
        v["used_texture_target_types"] = used_texture_target_typesv;
        Json::Value used_renderbuffer_target_typesv;
        for (const auto& pair : used_renderbuffer_target_types)
        {
            used_renderbuffer_target_typesv[texEnum(pair.first)] = (Json::Value::Int64)pair.second;
        }
        v["used_renderbuffer_target_types"] = used_renderbuffer_target_typesv;
        v["display"] = c.display;
        v["share"] = c.share_context;
        v["shaders"] = (int)c.shaders.size();
        v["textures"] = (int)c.textures.size();
        v["compressed_textures"] = (int)contexts[c.index].compressed_textures;
        v["framebuffers"] = Json::arrayValue;
        for (const auto& fb : c.framebuffers)
        {
            v["framebuffers"].append(json_fb(c, fb.index, -1));
        }
        v["renderbuffers"] = (int)c.renderbuffers.size();
        v["samplers"] = (int)c.samplers.size();
        v["buffers"] = (int)c.buffers.size();
        v["queries"] = (int)c.queries.size();
        v["draw_calls"] = sum_map(c.draw_calls_per_frame);
        v["flush_calls"] = sum_map(c.flush_calls_per_frame);
        v["finish_calls"] = sum_map(c.finish_calls_per_frame);
        v["transform_feedback_objects"] = (int)c.transform_feedbacks.size();
        v["attribute_buffers"] = Json::arrayValue;
        std::map<std::tuple<GLenum, GLint, GLsizei>, int> buffer_combos;
        for (const auto& buffer : c.buffers)
        {
            for (const auto& tuple : buffer.types)
            {
                buffer_combos[tuple]++;
            }
        }
        for (const auto& pair : buffer_combos)
        {
            Json::Value tv;
            std::string type = texEnum(std::get<0>(pair.first));
            if (type == "Unknown")
            {
                DBG_LOG("Attribute buffer of unknown type %x detected!\n", std::get<0>(pair.first));
                std::stringstream ss;
                ss << "0x" << std::hex << std::get<0>(pair.first);
                type = ss.str();
            }
            tv["type"] = type;
            tv["components"] = (int)std::get<1>(pair.first);
            tv["stride"] = (int)std::get<2>(pair.first);
            tv["count"] = pair.second;
            v["attribute_buffers"].append(tv);
        }
        v["programs"] = Json::arrayValue;
        for (auto& p : c.programs)
        {
            if (p.stats.dispatches > 0 || p.stats.drawcalls > 0 || report_unused_shaders)
            {
                Json::Value pv = json_program(c, p, -1);
                v["programs"].append(pv);
            }
        }
        result["contexts"].append(v);
    }
    result["surfaces"] = Json::arrayValue;
    for (auto& s : surfaces)
    {
        Json::Value v = json_base(input.surfaces[s.index]);
        v["display"] = s.display;
        v["activations"] = s.activations;
        v["gl_calls"] = s.glcount;
        v["egl_calls"] = s.eglcount;
        v["swapsbuffers"] = s.swaps;
        v["threads"] = Json::arrayValue;
        v["contexts"] = Json::arrayValue;
        switch (input.surfaces[s.index].type)
        {
        case SURFACE_NATIVE: v["type"] = "Native"; break;
        case SURFACE_PBUFFER: v["type"] = "Pbuffer"; break;
        case SURFACE_PIXMAP: v["type"] = "Pixmap"; break;
        case SURFACE_COUNT: assert(false); break;
        }
        for (auto& c : s.threads_used)
        {
            v["threads"].append(c.first);
        }
        for (auto& c : s.contexts_used)
        {
            v["contexts"].append(c.first);
        }
        result["surfaces"].append(v);
    }
    result["threads"] = input.numThreads;
    result["frames"] = frames;
    result["framerange"] = Json::Value();
    result["framerange"]["client_side_buffers"] = Json::Value();
    result["framerange"]["client_side_buffers"]["count"] = clientsidebuffers;
    result["framerange"]["client_side_buffers"]["total_size"] = (Json::Value::Int64)clientsidebuffersize;
    result["framerange"]["absolute"] = Json::Value();
    result["framerange"]["start"] = startframe;
    result["framerange"]["end"] = lastframe;
    result["framerange"]["texture_units"] = highestTextureUnitUsed + 1; // simplification, sparse usage not terribly interesting
    result["framerange"]["color_attachments"] = numColorAttachments; // another simplification, as above
    result["framerange"]["absolute"]["clears"] = clears;
    result["framerange"]["absolute"]["flushes"] = flushes;
    result["framerange"]["absolute"]["compute"] = compute;
    result["framerange"]["absolute"]["calls"] = calls;
    result["framerange"]["absolute"]["draws"] = Json::Value();
    result["framerange"]["absolute"]["draws"]["all"] = (Json::Value::Int64)drawcalls;
    result["framerange"]["absolute"]["draws"]["blended"] = blended;
    result["framerange"]["absolute"]["draws"]["dither"] = dither;
    result["framerange"]["absolute"]["draws"]["depthtested"] = depthtested;
    result["framerange"]["absolute"]["draws"]["depthwriting"] = depthwriting;
    result["framerange"]["absolute"]["draws"]["stenciled"] = stenciled;
    result["framerange"]["absolute"]["draws"]["faceculled"] = faceculled;
    result["framerange"]["absolute"]["draws"]["indexed"] = indexed;
    result["framerange"]["absolute"]["draw_modes"] = Json::Value();
    for (auto& d : drawtypes)
    {
       result["framerange"]["absolute"]["draw_modes"][drawEnum(d.first)] = Json::Value();
       result["framerange"]["absolute"]["draw_modes"][drawEnum(d.first)]["calls"] = d.second;
       result["framerange"]["absolute"]["draw_modes"][drawEnum(d.first)]["vertices"] = drawtypescount[d.first];
       result["framerange"]["absolute"]["draw_modes"][drawEnum(d.first)]["primitives"] = drawtypesprimitives[d.first];
       result["framerange"]["absolute"]["draw_modes"][drawEnum(d.first)]["primitives/calls"] = (double)drawtypesprimitives[d.first] / (double)d.second;
       if (drawtypesindirect[d.first])
       {
           result["framerange"]["absolute"]["draw_modes"][drawEnum(d.first)]["indirect_calls"] = drawtypesindirect[d.first];
       }
    }

    result["shaders"] = Json::arrayValue;
    for (auto& s : shaders)
    {
        Json::Value v;
        v["name"] = SafeEnumString(s.first);
        v["count"] = s.second;
        if (!compiler.empty())
        {
            v["average_arithmetic"] = shadercycles[s.first].a / s.second;
            v["average_loadstore"] = shadercycles[s.first].l / s.second;
            v["average_texture"] = shadercycles[s.first].t / s.second;
        }
        result["shaders"].append(v);
    }
    addMapToJson(result, "texture_formats", tex_formats);
    addMapToJson(result, "texture_sizes", tex_sizes);
    addMapToJson(result, "texture_types", texturetypes);
    addMapToJson(result, "scissor_sizes", scissor_sizes);
    result["texture_type_filters"] = Json::Value();
    for (auto& m : texturetypefilters)
    {
        result["texture_type_filters"][m.first] = Json::arrayValue;
        for (auto& n : m.second)
        {
            Json::Value w;
            w["name"] = n.first;
            w["count"] = n.second;
            result["texture_type_filters"][m.first].append(w);
        }
    }
    result["texture_type_sizes"] = Json::Value();
    for (auto& m : texturetypesizes)
    {
        result["texture_type_sizes"][m.first] = Json::arrayValue;
        for (auto& n : m.second)
        {
            Json::Value w;
            w["name"] = n.first;
            w["count"] = n.second;
            result["texture_type_sizes"][m.first].append(w);
        }
    }
    result["blend_modes"] = Json::arrayValue;
    for (auto& b : blendModes)
    {
        Json::Value v;
        v["mode"] = b.first.to_str();
        v["count"] = b.second;
        result["blend_modes"].append(v);
    }
    result["framerange"]["features"] = Json::arrayValue;
    for (unsigned i = 0; i < features.size(); i++)
    {
        if (features[i] > 0)
        {
            Json::Value v;
            v["name"] = featurenames.at(i);
            if (features[i] > 1)
            {
                v["count"] = features[i];
            }
            result["framerange"]["features"].append(v);
        }
    }
    return result;
}

static std::string add_node(const std::string& orig_prefix, int origid, const std::string& target_prefix, int targetid)
{
    return orig_prefix + std::to_string(origid) + " -> " + target_prefix + std::to_string(targetid) + ";\n";
}

static std::string dim2str(int width, int height, int depth) // return nice dimension string
{
    if (height == 1 && depth == 1) return std::to_string(width);
    else if (depth == 1) return std::to_string(width) + "x" + std::to_string(height);
    else return std::to_string(width) + "x" + std::to_string(height) + "x" + std::to_string(depth);
}

Json::Value AnalyzeTrace::frame_json(ParseInterfaceRetracing& input, int frame)
{
    Json::Value result;
    int frame_clears = 0;
    int frame_dupes = 0;
    result["contexts"] = Json::arrayValue;
    result["source"] = input.filename;
    result["frame"] = frame;
    result["dump_heaviness"] = heavinesses.at(frame);
    result["frame_heaviness"] = calculate_heaviness(input, frame);
    result["calls"] = calls_per_frame.at(frame);
    for (auto& c : input.contexts)
    {
        std::string digraph = "digraph frame_f" + std::to_string(frame) + " {\n";
        std::unordered_set<int> used_fbos; // by index
        std::unordered_set<int> used_rbs; // by index
        std::unordered_set<int> used_texs; // by index
        std::unordered_set<int> used_programs; // by index
        Json::Value subresult = json_base(c);
        subresult["renderpasses"] = Json::arrayValue;
        subresult["programs"] = Json::arrayValue;
        subresult["textures"] = Json::arrayValue;
        subresult["framebuffers"] = Json::arrayValue;
        subresult["renderpasses"] = Json::arrayValue;
        std::unordered_set<int> used_textures_by_id;
        std::unordered_set<int> used_texture_targets_by_id;
        for (auto& r : c.render_passes)
        {
            if (r.frame == frame)
            {
                Json::Value v;
                v["index"] = r.index;
                v["first_drawcall"] = r.first_call;
                v["last_drawcall"] = r.last_call;
                v["framebuffer_id"] = r.drawframebuffer;
                const GLuint fb_id = r.drawframebuffer;
                const int fb_index = c.framebuffer_remapping[fb_id];
                StateTracker::Framebuffer& fb = c.framebuffers[fb_index];
                used_fbos.insert(fb_index);
                if (!fb.label.empty())
                {
                    v["label"] = fb.label;
                }
                if (!r.snapshot_filename.empty())
                {
                    v["snapshot"] = r.snapshot_filename;
                }
                Json::Value list = Json::arrayValue;
                if (r.used_renderbuffers.size() > 0)
                {
                    for (auto t : r.used_renderbuffers)
                    {
                        assert(t < (int)c.renderbuffers.size());
                        assert(c.renderbuffers[t].id != 0);
                        list.append(c.renderbuffers[t].id);
                        digraph += add_node("    fbo_", fb_id, "rb_", c.renderbuffers[t].id);
                        used_rbs.insert(t);
                    }
                    v["used_renderbuffer_target_ids"] = list;
                }
                if (r.used_texture_targets.size() > 0)
                {
                    list = Json::arrayValue;
                    for (auto t : r.used_texture_targets)
                    {
                        assert(t < (int)c.textures.size());
                        assert(c.textures[t].id != 0);
                        list.append(c.textures[t].id);
                        digraph += add_node("    fbo_", fb_id, "tex_", c.textures[t].id);
                        used_texs.insert(t);
                        used_textures_by_id.insert(c.textures[t].id);
                        used_texture_targets_by_id.insert(c.textures[t].id);
                    }
                    v["used_texture_target_ids"] = list;
                }
                if (textures_by_renderpass[r.index].size() > 0)
                {
                    list = Json::arrayValue;
                    for (auto& id : textures_by_renderpass[r.index])
                    {
                        assert(id != 0);
                        list.append(id);
                        // If texture has already been written to during this frame, add it to graph.
                        // We ignore 'normal' texture accesses - the graph would be far too crowded otherwise.
                        if (std::find(used_texture_targets_by_id.begin(), used_texture_targets_by_id.end(), id) != used_texture_targets_by_id.end())
                        {
                            digraph += add_node("    tex_", id, "fbo_", fb_id);
                        }
                        used_textures_by_id.insert(id);
                    }
                    v["used_texture_source_ids"] = list;
                }
                if (r.used_programs.size() > 0)
                {
                    list = Json::arrayValue;
                    std::unordered_set<int> vertexset; // by id
                    for (const auto t : r.used_programs)
                    {
                        assert(t < (int)c.programs.size());
                        assert(c.programs[t].id != 0);
                        list.append(c.programs[t].id);
                        for (const auto s : c.programs[t].shaders)
                        {
                            if (s.first == GL_VERTEX_SHADER)
                            {
                                const StateTracker::Shader& shader = c.shaders[s.second];
                                vertexset.insert(shader.id);
                            }
                        }
                        if (c.programs[t].shaders.count(GL_COMPUTE_SHADER) > 0)
                        {
                            digraph += add_node("    fbo_", fb_id, "compute_", c.programs[t].id);
                            digraph += "    compute_" + std::to_string(c.programs[t].id) + " [shape=doubleoctagon];\n";
                        }
                        used_programs.insert(t);
                    }
                    v["used_program_ids"] = list;
                    Json::Value vertexlist = Json::arrayValue;
                    for (const int v : vertexset)
                    {
                        vertexlist.append(v);
                    }
                    v["used_vertex_shader_ids"] = vertexlist;
                }
                v["draw_calls"] = r.draw_calls;
                v["primitives"] = (Json::Value::Int64)r.primitives;
                v["vertices"] = (Json::Value::Int64)r.vertices;
                subresult["renderpasses"].append(v);
            }
        }
        for (auto& r : c.textures)
        {
            if (used_textures_by_id.count(r.index) > 0)
            {
                Json::Value tex = json_base(r);
                tex["format"] = texEnum(r.internal_format);
                tex["type"] = texEnum(r.binding_point);
                tex["levels"] = r.levels;
                tex["width"] = r.width;
                tex["height"] = r.height;
                tex["depth"] = r.depth;
                tex["min_filters"] = Json::arrayValue;
                for (GLenum f : r.used_min_filters)
                {
                    tex["min_filters"].append(texEnum(f));
                }
                tex["mag_filters"] = Json::arrayValue;
                for (GLenum f : r.used_mag_filters)
                {
                    tex["mag_filters"].append(texEnum(f));
                }
                subresult["textures"].append(tex);
            }
        }
        for (auto& r : c.programs)
        {
            if (used_programs.count(r.index) == 0)
            {
                continue;
            }
            Json::Value program = json_program(c, r, frame);
            for (auto& s : r.shaders) // type : index pairs
            {
                const StateTracker::Shader& shader = c.shaders[s.second];
                std::string filename = shader_filename(shader, c.index, r.index);
                if (!write_used_shaders)
                {
                    continue;
                }
                switch (s.first)
                {
                case GL_FRAGMENT_SHADER: program["frag"] = filename; break;
                case GL_VERTEX_SHADER: ; program["vert"] = filename; break;
                case GL_GEOMETRY_SHADER: program["geom"] = filename; break;
                case GL_TESS_EVALUATION_SHADER: program["tess"] = filename; break;
                case GL_TESS_CONTROL_SHADER: program["tscc"] = filename; break;
                case GL_COMPUTE_SHADER: program["comp"] = filename; break;
                default: assert(false); break;
                }
                FILE *fp = fopen(filename.c_str(), "w");
                if (fp)
                {
                    fwrite(shader.source_code.c_str(), shader.source_code.size(), 1, fp);
                    fclose(fp);
                }
                else
                {
                    std::cout << "Failed to create " << filename << std::endl;
                    exit(1);
                }
            }
            subresult["programs"].append(program);
        }
        for (const GLuint idx : used_fbos)
        {
            frame_clears += c.framebuffers[idx].total_clears[frame];
            frame_dupes += c.framebuffers[idx].duplicate_clears[frame];
            GLuint id = c.framebuffers[idx].id;
            digraph += "    fbo_" + std::to_string(id) + " [label=\"FBO " + std::to_string(id) + "\"];\n";
            subresult["framebuffers"].append(json_fb(c, idx, frame));
        }
        for (const GLuint idx : used_rbs)
        {
            Json::Value rb = json_base(c.renderbuffers[idx]);
            GLuint id = c.renderbuffers[idx].id;
            digraph += "    rb_" + std::to_string(id) + " [shape=Mrecord, label=\"{Renderbuffer " + std::to_string(id) + "|";
            digraph += texEnum(c.renderbuffers[idx].internalformat) + "}|{" + std::to_string(c.renderbuffers[idx].samples) + "|";
            digraph += dim2str(c.renderbuffers[idx].width, c.renderbuffers[idx].height, 1) + "}\"];\n";
            subresult["renderbuffers"].append(rb);
        }
        for (const GLuint idx : used_texs)
        {
            GLuint id = c.textures[idx].id;
            digraph += "    tex_" + std::to_string(id) + " [shape=record, label=\"{Texture " + std::to_string(id) + "|";
            digraph += texEnum(c.textures[idx].internal_format) + "}|{" + texEnum(c.textures[idx].binding_point) + "|";
            digraph += dim2str(c.textures[idx].width, c.textures[idx].height, c.textures[idx].depth) + "}\"];\n";
        }
        if (c.render_passes.size() > 0)
        {
            digraph += "}";
            std::string filename = (dump_csv_filename.empty() ? "renderpasses_c" : (dump_csv_filename + "_c")) + std::to_string(c.index) + "_f" + std::to_string(frame) + ".dot";
            FILE *fp = fopen(filename.c_str(), "w");
            fprintf(fp, "%s", digraph.c_str());
            fclose(fp);
        }
        result["total_clears"] = frame_clears;
        result["duplicate_clears"] = frame_dupes;
        result["contexts"].append(subresult);
    }
    return result;
}

int main(int argc, char **argv)
{
    assert(complexity_feature_value.size() == featurenames.size());
    assert(complexity_feature_value.size() == FEATURE_MAX);

    bool display_mode = false;
    bool no_screenshots = false;
    int argIndex = 1;
    for (; argIndex < argc; ++argIndex)
    {
        std::string arg = argv[argIndex];

        if (arg[0] != '-')
        {
            break;
        }
        else if (arg == "-h")
        {
            printHelp();
            return 1;
        }
        else if (arg == "-f" && argIndex + 2 < argc)
        {
            startframe = atoi(argv[argIndex + 1]);
            lastframe = atoi(argv[argIndex + 2]);
            argIndex += 2;
        }
        else if (arg == "-r" && argIndex + 1 < argc)
        {
            std::istringstream mylist(argv[argIndex + 1]);
            std::string s;
            while (std::getline(mylist, s, ','))
            {
                renderpassframes.insert(atoi(s.c_str()));
            }
            argIndex++;
        }
        else if (arg == "-v")
        {
            printVersion();
            return 1;
        }
        else if (arg == "-D")
        {
            debug = true;
        }
        else if (arg == "-C")
        {
            complexity_only_mode = true;
        }
        else if (arg == "-z")
        {
            dumpCallTime = true;
        }
        else if (arg == "-b")
        {
            bareLog = true;
        }
        else if (arg == "-n")
        {
            no_screenshots = true;
        }
        else if (arg == "-S")
        {
            display_mode = true;
        }
        else if (arg == "-s")
        {
            report_unused_shaders = true;
        }
        else if (arg == "-Z")
        {
            write_used_shaders = true;
        }
        else if (arg == "-q")
        {
            // no longer used, used to be quick mode
        }
        else if (arg == "-d")
        {
            dump_to_text = true;
        }
        else if (arg == "-c" && argIndex + 1 < argc)
        {
            compiler = argv[argIndex + 1];
            argIndex++;
        }
        else if (arg == "-o" && argIndex + 1 < argc)
        {
            dump_csv_filename = argv[argIndex + 1];
            argIndex++;
        }
        else
        {
            std::cerr << "Error: Unknown option " << arg << std::endl;
            printHelp();
            return 1;
        }
    }

    if (argIndex + 1 > argc)
    {
        printHelp();
        return 1;
    }
    std::string source_trace_filename = argv[argIndex++];
    ParseInterfaceRetracing inputFile;
    inputFile.setDisplayMode(display_mode);
    inputFile.setQuickMode(true);
    inputFile.setScreenshots(!no_screenshots);
    if (!inputFile.open(source_trace_filename))
    {
        std::cerr << "Failed to open for reading: " << source_trace_filename << std::endl;
        return 1;
    }
    AnalyzeTrace antr;
    antr.analyze(inputFile);
    inputFile.close();
    return 0;
}
