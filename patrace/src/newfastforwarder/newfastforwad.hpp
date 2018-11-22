#ifndef _FASTFORWARD_HPP_
#define _FASTFORWARD_HPP_
#include <cstdlib>
#include <stdio.h>
#include <map>
#include <vector>
#include <list>
#include <string>
#include <set>
#include <unordered_set>

namespace newfastforwad {

class shaderContext;
class programContext;
class textureContext;
class bufferContext;
class frameBufferContext;
class renderBufferContext;
class renderBufferContext;
class samplerContext;
class vertexArrayContext;
class shaderState
{
public:
    shaderState();
    //----------
    //external
    void ff_glCreatShader(unsigned int index, unsigned int type, unsigned int callNo);
    void ff_glShaderSource(unsigned int index, unsigned int callNo);
    void ff_glCompileShader(unsigned int index, unsigned int callNo);
    //----------
    //internal
    unsigned int getShaderType(unsigned int index);
    void readCurShaderState(unsigned int index, std::unordered_set<unsigned int> &callNoList);
    void clear();
    //  std::map<unsigned int,shaderContext> allShaderList;
    //private:
    unsigned int curShaderIdx;
    std::map<unsigned int, shaderContext> allShaderList;
};

class shaderContext
{
public:
    shaderContext();
    //external record
    unsigned int glCreateShaderNo;
    unsigned int glShaderSourceNo;
    unsigned int glCompileShaderNo;
    //internal using variable
    //GL_VERTEX_SHADER : 35633, GL_FRAGMENT_SHADER : 35632
    unsigned int shaderType;
};

class programState
{
public:
    programState();
    //external....
    void ff_glCreateProgram(unsigned int index, unsigned int callNo);
    void ff_glAttachShader(unsigned int index, unsigned int shaderIndex, unsigned int callNo);
    void ff_glLinkProgram(unsigned int index, unsigned int callNo);
    void ff_glBindAttribLocation(unsigned int index, char* name, unsigned int callNo);
    void ff_glValidateProgram(unsigned int index, unsigned int callNo);
    void ff_glGetAttribLocation(unsigned int index, char* name, unsigned int callNo);
    void ff_glGetUniformLocation(unsigned int index, char* name, unsigned int location, unsigned int callNo);
    void ff_glUseProgram(unsigned int index, unsigned int callNo);
    void ff_glUniform_Types(unsigned int LocationIndex, unsigned int callNo);
    void ff_glProgramBinary(unsigned int index, unsigned int callNo);
    void ff_glUniformBlockBinding(unsigned int program, unsigned int callNo);
    void debug();
    //internal...
    void clear();
    shaderState curShaderState;
    void readCurProgramState(std::unordered_set<unsigned int> &callNoList);///*std::vector<unsigned int> &clientSideBufferList,std::vector<unsigned int> &bufferList,*/
private:
    unsigned int curProgramIdx;
    unsigned int curProgramNo;
    std::map<unsigned int, programContext> allProgramList;
};

class programContext
{
public:
    programContext();
    unsigned int glCreateProgramNo;
    struct glAttachShaderState
    {
        unsigned int gl_vertex_shaderNo;
        unsigned int gl_fragment_shaderNo;
        unsigned int gl_compute_shaderNo;
        unsigned int gl_tess_control_shaderNo;
        unsigned int gl_tess_evaluation_shaderNo;
        unsigned int gl_geometry_shaderNo;
        unsigned int gl_vertex_shaderIdx;
        unsigned int gl_fragment_shaderIdx;
        unsigned int gl_compute_shaderIdx;
        unsigned int gl_tess_control_shaderIdx;
        unsigned int gl_tess_evaluation_shaderIdx;
        unsigned int gl_geometry_shaderIdx;
    }glAttachShaderNo;
    unsigned int glLinkProgramNo;
    std::map<std::string, unsigned int> glBindAttribLocationNoList;
    unsigned int glValidateProgramNo;
    struct withOneBinder
    {
        unsigned int thisCallNo;
        unsigned int binderCallNo;
    };

    std::map<std::string, withOneBinder> glGetAttribLocationNoList;
    std::map<std::string, unsigned int> glGetUniformLocationNoList;
    std::map<unsigned int, unsigned int> glGetUniformLocationNoIndexList;
    struct GL_VERTEX_SHADERstate
    {
        unsigned int glCreateShaderNo;
        unsigned int glShaderSourceNo;
        unsigned int glCompileShaderNo;
    }GL_VERTEX_SHADERstateNo;
    struct GL_FRAGMENT_SHADERstate
    {
        unsigned int glCreateShaderNo;
        unsigned int glShaderSourceNo;
        unsigned int glCompileShaderNo;
    }GL_FRAGMENT_SHADERstateNo;
    struct GL_COMPUTE_SHADERstate
    {
        unsigned int glCreateShaderNo;
        unsigned int glShaderSourceNo;
        unsigned int glCompileShaderNo;
    }GL_COMPUTE_SHADERstateNo;
    struct GL_TESS_CONTROL_SHADERstate
    {
        unsigned int glCreateShaderNo;
        unsigned int glShaderSourceNo;
        unsigned int glCompileShaderNo;
    }GL_TESS_CONTROL_SHADERstateNo;
    struct GL_TESS_EVALUATION_SHADERstate
    {
        unsigned int glCreateShaderNo;
        unsigned int glShaderSourceNo;
        unsigned int glCompileShaderNo;
    }GL_TESS_EVALUATION_SHADERstateNo;
    struct GL_GEOMETRY_SHADERstate
    {
        unsigned int glCreateShaderNo;
        unsigned int glShaderSourceNo;
        unsigned int glCompileShaderNo;
    }GL_GEOMETRY_SHADERstateNo;
    struct withBinder
    {
        unsigned int thisCallNo;
        unsigned int binderCallNo;
        unsigned int glGetUniformLocationNo;
    };
    std::map<unsigned int, withBinder> glUniform_NoList;
    unsigned int glProgramBinaryNo;
    std::vector<unsigned int> glUniformBlockBindingList;
};

class textureState
{
public:
    textureState();
    void ff_glGenTextures(unsigned int size, unsigned int index[], unsigned int callNo);
    void ff_glBindTexture(unsigned int index, unsigned int type, unsigned int callNo);
    void ff_glTexParameter_Types(unsigned int Typeindex, unsigned int type, unsigned int callNo);
    void ff_glTypes_TexImage2D(unsigned int Typeindex, unsigned int GL_PIXEL_UNPACK_BUFFERNo, unsigned int callNo, std::unordered_set<unsigned int> & callNoList);
    void ff_glPixelStorei(unsigned int Typeindex, unsigned int param, unsigned int callNo);
    void ff_glActiveTexture(unsigned int TypeIndex, unsigned int callNo);
    void ff_glTypes_TexSubImage2D(unsigned int Typeindex, unsigned int GL_PIXEL_UNPACK_BUFFERNo, unsigned int callNo, std::unordered_set<unsigned int> & callNoList);
    void ff_glGenerateMipmap(unsigned int callNo);
    void ff_glBindImageTexture(unsigned int unit, unsigned int index, unsigned int access, unsigned int callNo);
    void ff_glBindSampler(unsigned int unit, unsigned int index, unsigned int callNo);
    void ff_glGenSamplers(unsigned int size, unsigned int index[], unsigned int callNo);
    void ff_glSamplerParameter_Types(unsigned int index, unsigned int type, unsigned int callNo);
    void ff_glCopyImageSubData(unsigned int srcName, unsigned int destName, unsigned int callNo);
    //internal...
    void readCurTextureState(std::unordered_set<unsigned int> &callNoList, bool * callNoListJudge);
    void findTextureContext(std::vector<unsigned int> &textureList, std::unordered_set<unsigned int> & callNoList);
    void readCurTextureIdx(std::vector<unsigned int> &textureList);
    unsigned int readCurTextureNIdx(unsigned int n);
    void clear();
    void fordebug();
    void saveContext();
    void readContext(int ctx);
    int share_ctx;
private:
    unsigned int curActiveTextureIdx;
    unsigned int curActiveTextureNo;
    unsigned int share_context_curActiveTextureIdx;
    unsigned int share_context_curActiveTextureNo;
    unsigned int lastActiveTextureIdx;
    unsigned int lastActiveTextureNo;
    struct activeTextureState
    {
        unsigned int curTextureIdx;
        unsigned int curTextureNo;
        unsigned int curActiveTextureIdx;
        unsigned int curActiveTextureNo;
    };
    //the least is 64, the nvidia is 192
    //64-128 is cube map
    activeTextureState activeTextureNo[192];
    activeTextureState share_context_activeTextureNo[192];
    std::map<unsigned int, textureContext> allTextureList;
    struct globalGlPixelStoreiState
    {
        unsigned int gl_pack_row_lengthNo;
        unsigned int gl_pack_skip_pixelsNo;
        unsigned int gl_pack_skip_rowsNo;
        unsigned int gl_pack_alignmentNo;
        unsigned int gl_unpack_row_lengthNo;
        unsigned int gl_unpack_image_heightNo;
        unsigned int gl_unpack_skip_pixelsNo;
        unsigned int gl_unpack_skip_rowsNo;
        unsigned int gl_unpack_skip_imagesNo;
        unsigned int gl_unpack_alignmentNo;
        unsigned int gl_unpack_alignmentBinderNo;
        unsigned int gl_unpack_alignmentActiveNo;
    }globalGlPixelStoreiNo;
    std::map<unsigned int, samplerContext> allSamplerList;
    std::map<unsigned int, unsigned int> allSamplerBindingList;
};

class textureContext
{
public:
    textureContext();
    unsigned int glGenTexturesNo;
    struct withBinder
    {
        unsigned int thisCallNo;
        unsigned int binderCallNo;
        unsigned int activeCallNo;
    };
    struct withBinderForGenerate
    {
        unsigned int thisCallNo;
        unsigned int binderCallNo;
        unsigned int activeCallNo;
        unsigned int nowActiveCallNo;
    };
    struct withBinderForParameterState
    {
        unsigned int thisCallNo;
        unsigned int binderCallNo;
        unsigned int activeCallNo;
        unsigned int activeCallNo2;
    };
    //need to complete all parameters
    struct glTexParameterState
    {
        //total 17
        withBinderForParameterState gl_texture_wrap_sNo;
        withBinderForParameterState gl_texture_wrap_rNo;
        withBinderForParameterState gl_texture_wrap_tNo;
        withBinderForParameterState gl_texture_mag_filterNo;
        withBinderForParameterState gl_texture_min_filterNo;
        withBinderForParameterState gl_texture_max_levelNo;
        withBinderForParameterState gl_texture_swizzle_aNo;
        withBinderForParameterState gl_texture_swizzle_rNo;
        withBinderForParameterState gl_texture_swizzle_gNo;
        withBinderForParameterState gl_texture_swizzle_bNo;
        withBinderForParameterState gl_texture_base_levelNo;
        withBinderForParameterState gl_texture_compare_modeNo;
        withBinderForParameterState gl_texture_compare_funcNo;
        withBinderForParameterState gl_texture_srgs_decode_extNo;
        withBinderForParameterState gl_texture_min_lodNo;
        withBinderForParameterState gl_texture_max_lodNo;
    }glTexParameterNo;
    struct imageWithBinder
    {
        unsigned int thisCallNo;
        unsigned int binderCallNo;
        unsigned int activeCallNo;
        unsigned int activeCallNo2;
        unsigned int gl_pack_row_lengthNo;
        unsigned int gl_pack_skip_pixelsNo;
        unsigned int gl_pack_skip_rowsNo;
        unsigned int gl_pack_alignmentNo;
        unsigned int gl_unpack_row_lengthNo;
        unsigned int gl_unpack_image_heightNo;
        unsigned int gl_unpack_skip_pixelsNo;
        unsigned int gl_unpack_skip_rowsNo;
        unsigned int gl_unpack_skip_imagesNo;
        unsigned int gl_unpack_alignmentNo;
        unsigned int gl_unpack_alignmentBinderNo;
        unsigned int gl_unpack_alignmentActiveNo;
        unsigned int GL_PIXEL_UNPACK_BUFFERNo;

        withBinderForParameterState gl_texture_min_filterNo;
    };
    std::map<unsigned int, imageWithBinder> glTexImage2DNoList;
    unsigned int glTexImage2DIdx;
    std::map<unsigned int, std::unordered_set<unsigned int>> glTexImage2DGL_PIXEL_UNPACK_BUFFERList;
    std::map<unsigned int, imageWithBinder> glTexSubImage2DNoList;
    unsigned int glTexSubImage2DIdx;
    std::map<unsigned int, std::unordered_set<unsigned int>> glTexSubImage2DGL_PIXEL_UNPACK_BUFFERList;
    withBinderForGenerate glGenerateMipmapNo;
    struct withBinderSampler
    {
        unsigned int thisCallNo;
        unsigned int binderCallNo;
        unsigned int activeCallNo;
        unsigned int bindSamplerIdx;
    };
    withBinderSampler glBindSamplerNo[32];//max sampler num is 32

    std::list<unsigned int> glCopyImageSubDataNoList;
    std::unordered_set<unsigned int> glCopyImageSubData_textureContextNoList;

};

class samplerContext
{
public:
    samplerContext();
    unsigned int glGenSamplersNo;
    struct glSamplerParameterState
    {
        unsigned int gl_texture_wrap_sNo;
        unsigned int gl_texture_wrap_rNo;
        unsigned int gl_texture_wrap_tNo;
        unsigned int gl_texture_mag_filterNo;
        unsigned int gl_texture_min_filterNo;
        unsigned int gl_texture_min_lodNo;
        unsigned int gl_texture_max_lodNo;
        unsigned int gl_texture_compare_modeNo;
        unsigned int gl_texture_compare_funcNo;
    }glSamplerParameterNo;
private:
};

class bufferState
{
public:
    bufferState();
    void ff_glGenBuffers(unsigned int size, unsigned int index[], unsigned int callNo);
    void ff_glBindBuffer(unsigned int type, unsigned int index, unsigned int callNo);
    void ff_glBufferData(unsigned int type, unsigned int callNo);
    void ff_glBufferSubData(unsigned int type, unsigned int offset, unsigned int size, unsigned int callNo);
    void ff_glDeleteBuffers(unsigned int size, unsigned int index[], unsigned int callNo);
    void ff_glMapBufferRange(unsigned int target, unsigned int offset, unsigned int size, unsigned int callNo);
    void ff_glCopyClientSideBuffer(unsigned int target, unsigned long long nameNo, unsigned int callNo);
    void ff_glFlushMappedBufferRange(unsigned int target, unsigned int callNo);
    void ff_glUnmapBuffer(unsigned int target, unsigned int callNo);
    void ff_glGenVertexArrays(unsigned int size, unsigned int index[], unsigned int callNo);
    void ff_glBindVertexArray(unsigned int index, unsigned int callNo);
    void ff_glVertexAttribPointerForVao(unsigned int LocationIndex, unsigned int callNo, unsigned int clientSideBufferIndex, unsigned int bufferIndex, unsigned int bufferNo);
    void ff_glEnableVertexAttribArrayForVao(unsigned int LocationIndex, unsigned int callNo);
    void ff_glBindBufferForVao(unsigned int type, unsigned int index, unsigned int callNo);
    //internal...
    unsigned int getCurGL_ARRAY_BUFFERIdx();
    unsigned int getCurGL_ARRAY_BUFFERNo();
    unsigned int getCurGL_ELEMENT_ARRAY_BUFFERIdx();
    unsigned int getCurGL_ELEMENT_ARRAY_BUFFERNo();
    unsigned int getCurGL_PIXEL_UNPACK_BUFFERIdx();
    unsigned int getCurGL_PIXEL_UNPACK_BUFFERNo();
    unsigned int getVAOindex();
    void readCurBufferState(unsigned int type, std::vector<unsigned int> &bufferList, std::unordered_set<unsigned int> &callNoList);
    void findABufferState(unsigned int index, std::unordered_set<unsigned int> &callNoList);
    void readCurDeleteBuffers(std::unordered_set<unsigned int> &callNoList);
    void clear();
    void debug(unsigned int callNo);
    struct mapBufferWithBinder
    {
        unsigned int thisCallNo;
        unsigned int binderCallNo;
        unsigned int binderBufferIdx;
    };
    unsigned int curGL_SHADER_STORAGE_BUFFER0Idx[32];//max GL_SHADER_STORAGE_BUFFER num is 32
    unsigned int curGL_SHADER_STORAGE_BUFFER0No[32];
    unsigned int curGL_SHADER_STORAGE_BUFFER1Idx;
    unsigned int curGL_SHADER_STORAGE_BUFFER1No;
    unsigned int curGL_UNIFORM_BUFFER0Idx[32];//max GL_UNIFORM_BUFFER num is 32
    unsigned int curGL_UNIFORM_BUFFER0No[32];
    //for mapbuffer
    mapBufferWithBinder glMapBufferRange_GL_ARRAY_BUFFERNo;
    mapBufferWithBinder glMapBufferRange_GL_ELEMENT_ARRAY_BUFFERNo;
    mapBufferWithBinder glMapBufferRange_GL_PIXEL_UNPACK_BUFFERNo;
    unsigned int glCopyClientSideBuffer_GL_ARRAY_BUFFERIdx;
    unsigned int glCopyClientSideBuffer_GL_ELEMENT_ARRAY_BUFFERIdx;
    unsigned int glCopyClientSideBuffer_GL_PIXEL_UNPACK_BUFFERIdx;
    bool glMapBufferRange_GL_ARRAY_BUFFEREnd;
    bool glMapBufferRange_GL_ELEMENT_ARRAY_BUFFEREnd;
    bool glMapBufferRange_GL_PIXEL_UNPACK_BUFFEREnd;
    //for mapbuffer
private:
    unsigned int curGL_ARRAY_BUFFERIdx;
    unsigned int curGL_ARRAY_BUFFERNo;
    unsigned int curGL_ELEMENT_ARRAY_BUFFERIdx;
    unsigned int curGL_ELEMENT_ARRAY_BUFFERNo;
    unsigned int curGL_PIXEL_UNPACK_BUFFERIdx;
    unsigned int curGL_PIXEL_UNPACK_BUFFERNo;
    unsigned int curGL_COPY_WRITE_BUFFERIdx;
    unsigned int curGL_COPY_WRITE_BUFFERNo;
    unsigned int curGL_DRAW_INDIRECT_BUFFERIdx;
    unsigned int curGL_DRAW_INDIRECT_BUFFERNo;
    unsigned int curGL_ATOMIC_COUNTER_BUFFERIdx;
    unsigned int curGL_ATOMIC_COUNTER_BUFFERNo;
    unsigned int curVertexArrayObjectNo;
    unsigned int curVertexArrayObjectIdx;
    std::map<unsigned int, bufferContext> allBufferList;
    std::map<unsigned int, std::vector<unsigned int>> GlobalGenIndexList;
    std::map<unsigned int, std::vector<unsigned int>> GlobalDeleteIndexList;
    std::map<unsigned int, vertexArrayContext> allVaoList;
};

class bufferContext
{
public:
    bufferContext();
    unsigned int glGenBuffersNo;
    struct withBinder
    {
        unsigned int thisCallNo;
        unsigned int binderCallNo;
    };
    std::list<withBinder> glBufferDataNoList;
    unsigned int glBufferDataIdx;
    struct withBinderSubData
    {
        unsigned int thisCallNo;
        unsigned int binderCallNo;
        unsigned int offSet;
        unsigned int size;
    };
    std::list<withBinderSubData> glBufferSubDataNoList;
    unsigned int glBufferSubDataIdx;
    //for mapbuffer
    unsigned int glCopyClientSideBuffer_GL_ARRAY_BUFFERIdx;
    unsigned int glCopyClientSideBuffer_GL_ELEMENT_ARRAY_BUFFERIdx;
    unsigned int glCopyClientSideBuffer_GL_PIXEL_UNPACK_BUFFERIdx;
    unsigned int glCopyClientSideBuffer_GL_DRAW_INDIRECT_BUFFERIdx;
    unsigned int glCopyClientSideBuffer_GL_COPY_WRITE_BUFFERIdx;
    unsigned int glCopyClientSideBuffer_GL_UNIFORM_BUFFERIdx;
    bool glMapBufferRange_GL_ARRAY_BUFFEREnd;
    bool glMapBufferRange_GL_ELEMENT_ARRAY_BUFFEREnd;
    bool glMapBufferRange_GL_PIXEL_UNPACK_BUFFEREnd;
    bool glMapBufferRange_GL_DRAW_INDIRECT_BUFFEREnd;
    bool glMapBufferRange_GL_COPY_WRITE_BUFFEREnd;
    bool glMapBufferRange_GL_UNIFORM_BUFFEREnd;
    //for mapbuffer
    struct mapBuffer
    {
        unsigned int glMapBufferRangeNo;
        unsigned int glMapBufferRangeBinderNo;
        unsigned int glCopyClientSideBufferNo;
        unsigned int glCopyClientSideBufferBinderNo;
        unsigned long long glCopyClientSideBufferIdxNo;
        unsigned int glFlushMappedBufferRangeNo;
        unsigned int glFlushMappedBufferRangeBinderNo;
        unsigned int glUnmapBufferNo;
    };
    std::map<unsigned int, mapBuffer> glMapBufferRangeNoList;
};

class vertexArrayContext
{
public:
    vertexArrayContext();
    unsigned int glGenVertexArraysNo;
    struct withPointerBuffer
    {
        unsigned int binderNo;
        unsigned int thisCallNo;
        unsigned int clientSideBufferIdx;
        unsigned int bufferIdx;
        unsigned int bufferNo;
    };
    std::map<unsigned int, withPointerBuffer> glVertexAttribPointerNoList;
    struct withBinder
    {
        unsigned int binderCallNo;
        unsigned int thisCallNo;
        bool isUsing;
        unsigned int lastBinderCallNo;
    };
    std::map<unsigned int, withBinder> glEnableVertexAttribArrayNoList;
    struct elementArrayState
    {
        unsigned int binderCallNo;
        unsigned int thisCallNo;
        unsigned int elementArrayBufferIdx;
    }gl_element_array_bufferNo;
    unsigned int patchNum;
};

class clientSideBufferState
{
public:
    clientSideBufferState();
    void ff_glClientSideBufferData(unsigned int index, unsigned int callNo);
    void readClientSideBufferState(std::vector<unsigned int> &clientSideBufferList, std::unordered_set<unsigned int> &callNoList);
    void clear();
    unsigned long long findAClientSideBufferState(unsigned int name);
    std::map<unsigned int, unsigned long long>* glClientSideBuffDataNoList;
private:
};

class globalState
{
public:
    globalState();
    void ff_glFrontFace(unsigned int type, unsigned int callNo);
    void ff_glEnableDisable(unsigned int type, unsigned int callNo);
    void ff_glCullFace(unsigned int type, unsigned int callNo);
    void ff_glDepthMask(unsigned int type, unsigned int callNo);
    void ff_glDepthFunc(unsigned int type, unsigned int callNo);
    void ff_glBlendFunc(unsigned int type, unsigned int callNo);
    void ff_glBlendEquation(unsigned int type, unsigned int callNo);
    void ff_glViewport(unsigned int callNo);
    void ff_glFinish(unsigned int callNo);
    void ff_glFlush(unsigned int callNo);
    void ff_glScissor(unsigned int callNo);
    void ff_glClearColor(unsigned int callNo);
    void ff_glClear(unsigned int mask, unsigned int callNo, bool frameBuffer, unsigned int drawframebuffer);
    void ff_glEnableVertexAttribArray(unsigned int LocationIndex, unsigned int callNo);
    void ff_glDisableVertexAttribArray(unsigned int LocationIndex, unsigned int callNo);
    void ff_glColorMask(unsigned int callNo);
    void ff_glBlendColor(unsigned int callNo);
    void ff_glClearDepthf(unsigned int callNo);
    void ff_glBlendFuncSeparate(unsigned int callNo);
    void ff_glStencilOpSeparate(unsigned int type, unsigned int callNo);
    void ff_glStencilMask(unsigned int callNo);
    void ff_glStencilFuncSeparate(unsigned int type, unsigned int callNo);
    void ff_glPolygonOffset(unsigned int callNo);
    void ff_glClearStencil(unsigned int callNo);
    void ff_glBlendEquationSeparate(unsigned int callNo);
    void ff_glHint(unsigned int target, unsigned int callNo);
    void ff_glLineWidth(unsigned int callNo);
    void ff_glVertexAttribPointer(unsigned int LocationIndex, unsigned int callNo, unsigned int clientSideBufferIndex, unsigned int bufferIndex, unsigned int bufferNo);
    void ff_glVertexAttrib_Types(unsigned int LocationIndex, unsigned int callNo);
    void ff_glStencilMaskSeparate(unsigned int face, unsigned int callNo);
    void ff_glDepthRangef(unsigned int callNo);
    void ff_glInvalidateFramebuffer(unsigned int target, unsigned int num, unsigned int attachment[], unsigned int callNo);
    void ff_glSampleCoverage(unsigned int callNo);
    void ff_glDrawBuffers(unsigned int callNo);
    void ff_glVertexAttribDivisor(unsigned int LocationIndex, unsigned int callNo);
    void ff_glPatchParameteri(unsigned int callNo);
    //internal...
    void readCurGlobalState(std::vector<unsigned int> &clientSideBufferList, std::vector<unsigned int> &bufferList, std::unordered_set<unsigned int> &callNoList, bool framebuffer);
    void readCurGlobalStateForFrameBufferClear(std::unordered_set<unsigned int> &callNoList);
    void clear();
    void debug(unsigned int callNo);
    bool TransformFeedbackSwitch;
    bool computeMemoryBarrier;
private:
    unsigned int glFrontFaceNo;
    struct glEnableState
    {
        //14
        unsigned int gl_blendNo;
        unsigned int gl_cull_faceNo;
        unsigned int gl_debug_outputNo;
        unsigned int gl_debug_output_synchronousNo;
        unsigned int gl_depth_testNo;
        unsigned int gl_ditherNo;
        unsigned int gl_polygon_offset_fillNo;
        unsigned int gl_primitive_restart_fixed_indexNo;
        unsigned int gl_rasterizer_discardNo;
        unsigned int gl_sample_alpha_to_coverageNo;
        unsigned int gl_sample_coverageNo;
        unsigned int gl_sample_maskNo;
        unsigned int gl_scissor_testNo;
        unsigned int gl_stencil_testNo;
    }glEnableNo;
    unsigned int glCullFaceNo;
    unsigned int glDepthMaskNo;
    unsigned int glDepthFuncNo;
    unsigned int glBlendFuncNo;
    unsigned int glBlendEquationNo;
    unsigned int glViewportNo;
    unsigned int glFinishNo;
    unsigned int glFlushNo;
    unsigned int glScissorNo;
    unsigned int glClearColorNo;
    struct glClearState
    {
        unsigned int gl_color_buffer_bitNo;
        unsigned int gl_color_buffer_bitBinderNo;
        unsigned int gl_depth_buffer_bitNo;
        unsigned int gl_depth_buffer_bitBinderNo;
        unsigned int gl_stencil_buffer_bitNo;
        unsigned int gl_stencil_buffer_bitBinderNo;
        unsigned int glColorMaskBinderNo;
        unsigned int glClearColorNo;
        unsigned int glDepthMaskBinderNo;
        unsigned int glStencilMaskBinderNo;
        bool frameBuffer;
        unsigned int glScissorBinderNo;
    }glClearNo;
    unsigned int glClearDepthfNo;
    unsigned int glColorMaskNo;
    unsigned int glBlendColorNo;
    unsigned int glBlendFuncSeparateNo;
    struct glStencilOpSeparateState
    {
        unsigned int gl_frontNo;
        unsigned int gl_backNo;
    }glStencilOpSeparateNo;
    unsigned int glStencilMaskNo;
    struct glStencilFuncSeparateState
    {
        unsigned int gl_frontNo;
        unsigned int gl_backNo;
    }glStencilFuncSeparateNo;
    unsigned int glPolygonOffsetNo;
    unsigned int glClearStencilNo;
    unsigned int glBlendEquationSeparateNo;
    struct withBinder
    {
        unsigned int thisCallNo;
        unsigned int binderCallNo;
    };
    std::map<unsigned int, unsigned int> glEnableVertexAttribArrayNoList;
    std::map<unsigned int, unsigned int> glDisableVertexAttribArrayNoList;
    struct withPointerBuffer
    {
        unsigned int thisCallNo;
        unsigned int clientSideBufferIdx;
        unsigned int bufferIdx;
        unsigned int bufferNo;
    };
    std::map<unsigned int, withPointerBuffer> glVertexAttribPointerNoList;
    std::map<unsigned int, unsigned int> glVertexAttribNoList;
    struct glHintState
    {
        unsigned int gl_fragment_shader_derivative_hintNo;
        unsigned int gl_generate_mipmap_hintNo;
    }glHintNo;
    unsigned int glLineWidthNo;
    struct glStencilMaskSeparateState
    {
        unsigned int gl_frontNo;
        unsigned int gl_backNo;
    }glStencilMaskSeparateNo;
    unsigned int glDepthRangefNo;
    struct glInvalidateFramebufferState
    {
        unsigned int GL_READ_FRAMEBFUFFER_GL_COLOR_ATTACHMENT0;
        unsigned int GL_READ_FRAMEBFUFFER_GL_DEPTH_ATTACHMENT;
        unsigned int GL_READ_FRAMEBFUFFER_GL_STENCIL_ATTACHMENT;
        unsigned int GL_READ_FRAMEBFUFFER_GL_COLOR;
        unsigned int GL_READ_FRAMEBFUFFER_GL_DEPTH;
        unsigned int GL_READ_FRAMEBFUFFER_GL_STENCIL;
        unsigned int GL_FRAMEBFUFFER_GL_COLOR_ATTACHMENT0;
        unsigned int GL_FRAMEBFUFFER_GL_DEPTH_ATTACHMENT;
        unsigned int GL_FRAMEBFUFFER_GL_STENCIL_ATTACHMENT;
        unsigned int GL_FRAMEBFUFFER_GL_COLOR;
        unsigned int GL_FRAMEBFUFFER_GL_DEPTH;
        unsigned int GL_FRAMEBFUFFER_GL_STENCIL;
    }glInvalidateFramebufferNo;
    std::set<unsigned int> glEnableVertexAttribArrayList;//for save enable num indexv
    unsigned int glSampleCoverageNo;
    unsigned int glDrawBuffersNo;
    std::map<unsigned int, unsigned int> glVertexAttribDivisorList;
    unsigned int glPatchParameteriNo;
};

class eglState
{
public:
    eglState();
    void ff_eglCreateContext(unsigned int callNo);
    void ff_eglCreateWindowSurface(unsigned int callNo);
    void ff_eglMakeCurrent(unsigned int callNo);
    void ff_eglDestroySurface(unsigned int callNo);
    //internal...
    void readCurEglState(std::unordered_set<unsigned int> &callNoList);
    void clear();
private:
    std::vector<unsigned int> eglCreateContextNoList;
    std::vector<unsigned int> eglCreateWindowSurfaceNoList;
    std::vector<unsigned int> eglMakeCurrentNoList;
    std::vector<unsigned int> eglDestroySurfaceNoList;
};

class renderBufferState
{
public:
    renderBufferState();
    void ff_glGenRenderbuffers(unsigned int index, unsigned int callNo);
    void ff_glBindRenderbuffer(unsigned int index, unsigned int callNo);
    void ff_glRenderbufferStorage(unsigned int callNo);
    //internal...
    void readCurRenderBufferState(unsigned int index, std::unordered_set<unsigned int> &callNoList);
    void clear();
private:
    unsigned int curRenderBufferIdx;
    unsigned int curRenderBufferNo;
    std::map<unsigned int, renderBufferContext> allRenderBufferList;
};

class renderBufferContext
{
public:
    renderBufferContext();
    unsigned int glGenRenderbuffersNo;
    struct withBinder
    {
        unsigned int thisCallNo;
        unsigned int binderCallNo;
    };
    withBinder glRenderbufferStorageNo;
};

class frameBufferState
{
public:
    frameBufferState();
    void ff_glGenFramebuffers(unsigned int n, unsigned int index[], unsigned int callNo);
    void ff_glBindFramebuffer(unsigned int index, unsigned int type, unsigned int callNo);
    void ff_glFramebufferTexture2D(unsigned int type, unsigned int attachment, unsigned int textarget, unsigned int index, unsigned int callNo);
    void ff_glFramebufferRenderbuffer(unsigned int type, unsigned int attachment, unsigned int index, unsigned int callNo);
    void ff_glDeleteFramebuffers(unsigned int n, unsigned int index[], unsigned int callNo, std::unordered_set<unsigned int> &callNoList);
    void ff_glBlitFramebuffer(unsigned int callNo);
    void ff_glDrawBuffers(unsigned int callNo);
    void readCurFrameBufferState(std::vector<unsigned int> &textureList, std::unordered_set<unsigned int> &callNoList);
    bool isBindFrameBuffer();
    bool isBindReadFrameBuffer();
    unsigned int getDrawTextureIdx();
    unsigned int getReadTextureIdx();
    unsigned int getDrawTextureNo();
    unsigned int getReadTextureNo();
    void debug();
    void clear();
    void clearBlitFramebuffer();
    renderBufferState curRenderBufferState;
private:
    unsigned int curReadFrameBufferIdx;
    unsigned int curReadFrameBufferNo;
    unsigned int curDrawFrameBufferIdx;
    unsigned int curDrawFrameBufferNo;
    std::map<unsigned int, frameBufferContext> allFrameBufferList;
    std::unordered_set<unsigned int> curFrameBufferIdx;
};

class frameBufferContext
{
public:
    frameBufferContext();
    unsigned int glGenFramebuffersNo;
    struct withBinder
    {
        unsigned int thisCallNo;
        unsigned int binderCallNo;
    };
    struct glFramebufferTexture2D_RenderbufferState
    {
        unsigned int GL_COLOR_ATTACHMENT0Idx;
        unsigned int GL_COLOR_ATTACHMENT0Type;
        withBinder GL_COLOR_ATTACHMENT0No;
        //only GL_COLOR_ATTACHMENT0 for now
        //GL_COLOR_ATTACHMENT0 texture doesn't find its generate
        unsigned int GL_COLOR_ATTACHMENT1Idx;
        unsigned int GL_COLOR_ATTACHMENT1Type;
        withBinder GL_COLOR_ATTACHMENT1No;
        unsigned int GL_COLOR_ATTACHMENT2Idx;
        unsigned int GL_COLOR_ATTACHMENT2Type;
        withBinder GL_COLOR_ATTACHMENT2No;
        unsigned int GL_COLOR_ATTACHMENT3Idx;
        unsigned int GL_COLOR_ATTACHMENT3Type;
        withBinder GL_COLOR_ATTACHMENT3No;
        unsigned int GL_DEPTH_ATTACHMENTIdx;
        unsigned int GL_DEPTH_ATTACHMENTType;
        withBinder GL_DEPTH_ATTACHMENTNo;
        unsigned int GL_STENCIL_ATTACHMENTIdx;
        unsigned int GL_STENCIL_ATTACHMENTType;
        withBinder GL_STENCIL_ATTACHMENTNo;
    }glFramebufferTexture2D_RenderbufferNo;
    struct glBlitFramebufferState
    {
        unsigned int readFrameBufferIdx;
        unsigned int readFrameBufferNo;
        unsigned int drawFrameBufferIdx;
        unsigned int drawFrameBufferNo;
        unsigned int thisCallNo;
    };
    std::list<glBlitFramebufferState> glBlitFramebufferNoList;
    unsigned int glDrawBuffersNo;
private:
};


class contextState
{
public:
    contextState(std::map<unsigned int, unsigned long long> &glClientSideBuffDataNoListForShare);
    programState               curProgramState;
    textureState               curTextureState;
    globalState                curGlobalState;
    eglState                   curEglState;
    bufferState                curBufferState;
    frameBufferState           curFrameBufferState;
    clientSideBufferState      curClientSideBufferState;
    void readAllState(unsigned int frameNo, unsigned int drawType, unsigned int indices, std::unordered_set<unsigned int> &callNoList, bool * callNoListJudge);
    void readStateForDispatchCompute(std::unordered_set<unsigned int> &callNoList, bool * callNoListJudge);
    void clear();
    int share_context;
};

}//namespace newfastforwad

#endif// BEGIN
