#include <cstdlib>
#include <stdio.h>
#include <map>
#include <vector>
#include <list>
#include <GLES3/gl32.h>
#include <newfastforwarder/parser.hpp>
#include <GLES2/gl2ext.h>

using namespace std;

namespace newfastforwad {

const unsigned int MAX_ACTIVE_TEXTURE_NUM = 192;
const unsigned int MAX_GL_UNIFORM_BUFFER_NUM = 32;
const unsigned int MAX_GL_SHADER_STORAGE_BUFFER_NUM = 32;
const unsigned int MAX_SAMPLER_NUM = 32;
const unsigned int DEFAULT_VAO_INDEX = 65535;

const unsigned int CUBE_MAP_INDEX = 500000000;
const unsigned int FIRST_10_BIT = 1000000000;
const unsigned int NO_IDX = 4294967295;
const unsigned int MAX_INT = 4294967295;
const unsigned int MAP_BUFFER_RANGE = 4000000000u;

shaderContext::shaderContext()
 : glCreateShaderNo(0)
 , glShaderSourceNo(0)
 , glCompileShaderNo(0)
{
}

shaderState::shaderState()
 : curShaderIdx(0)
{
}

programContext::programContext()
 : glCreateProgramNo(0)
 , glAttachShaderNo({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})
 , glLinkProgramNo(0)
 , glValidateProgramNo(0)
 , GL_VERTEX_SHADERstateNo({0, 0, 0})
 , GL_FRAGMENT_SHADERstateNo({0, 0, 0})
 , GL_COMPUTE_SHADERstateNo({0, 0, 0})
 , GL_TESS_CONTROL_SHADERstateNo({0, 0, 0})
 , GL_TESS_EVALUATION_SHADERstateNo({0, 0, 0})
 , GL_GEOMETRY_SHADERstateNo({0, 0, 0})
 , glProgramBinaryNo(0)
{
}

programState::programState()
 : curProgramIdx(0)
 , curProgramNo(0)
{
}

textureContext::textureContext()
 : glGenTexturesNo(0)
 , glTexParameterNo({{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}})
 , glTexImage2DIdx(0)
 , glTexSubImage2DIdx(0)
 , glGenerateMipmapNo({0, 0, 0, 0})
 , glBindSamplerNo({{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}
                  , {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}})
{
}

textureState::textureState()
 : share_ctx(0)
 , curActiveTextureIdx(0)
 , curActiveTextureNo(0)
 , share_context_curActiveTextureIdx(0)
 , share_context_curActiveTextureNo(0)
 , lastActiveTextureIdx(0)
 , lastActiveTextureNo(0)
 , globalGlPixelStoreiNo({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})
{
    for(unsigned int i=0; i<MAX_ACTIVE_TEXTURE_NUM; i++)
    {
        activeTextureNo[i] = {0, 0, 0, 0};
        share_context_activeTextureNo[i] = {0, 0, 0, 0};
    }
}

samplerContext::samplerContext()
 : glGenSamplersNo(0)
 , glSamplerParameterNo({0, 0, 0, 0, 0, 0, 0, 0, 0})
{
}

bufferContext::bufferContext()
 : glGenBuffersNo(0)
 , glBufferDataIdx(0)
 , glBufferSubDataIdx(0)
 , glCopyClientSideBuffer_GL_ARRAY_BUFFERIdx(0)
 , glCopyClientSideBuffer_GL_ELEMENT_ARRAY_BUFFERIdx(0)
 , glCopyClientSideBuffer_GL_PIXEL_UNPACK_BUFFERIdx(0)
 , glCopyClientSideBuffer_GL_DRAW_INDIRECT_BUFFERIdx(0)
 , glCopyClientSideBuffer_GL_COPY_WRITE_BUFFERIdx(0)
 , glCopyClientSideBuffer_GL_UNIFORM_BUFFERIdx(0)
 , glMapBufferRange_GL_ARRAY_BUFFEREnd(0)
 , glMapBufferRange_GL_ELEMENT_ARRAY_BUFFEREnd(0)
 , glMapBufferRange_GL_PIXEL_UNPACK_BUFFEREnd(0)
 , glMapBufferRange_GL_DRAW_INDIRECT_BUFFEREnd(0)
 , glMapBufferRange_GL_COPY_WRITE_BUFFEREnd(0)
 , glMapBufferRange_GL_UNIFORM_BUFFEREnd(0)
{
}

bufferState::bufferState()
 : curGL_SHADER_STORAGE_BUFFER1Idx(0)
 , curGL_SHADER_STORAGE_BUFFER1No(0)
 , glMapBufferRange_GL_ARRAY_BUFFERNo({0, 0, 0})
 , glMapBufferRange_GL_ELEMENT_ARRAY_BUFFERNo({0, 0, 0})
 , glMapBufferRange_GL_PIXEL_UNPACK_BUFFERNo({0, 0, 0})
 , glCopyClientSideBuffer_GL_ARRAY_BUFFERIdx(0)
 , glCopyClientSideBuffer_GL_ELEMENT_ARRAY_BUFFERIdx(0)
 , glCopyClientSideBuffer_GL_PIXEL_UNPACK_BUFFERIdx(0)
 , glMapBufferRange_GL_ARRAY_BUFFEREnd(0)
 , glMapBufferRange_GL_ELEMENT_ARRAY_BUFFEREnd(0)
 , glMapBufferRange_GL_PIXEL_UNPACK_BUFFEREnd(0)
 , curGL_ARRAY_BUFFERIdx(0)
 , curGL_ARRAY_BUFFERNo(0)
 , curGL_ELEMENT_ARRAY_BUFFERIdx(0)
 , curGL_ELEMENT_ARRAY_BUFFERNo(0)
 , curGL_PIXEL_UNPACK_BUFFERIdx(0)
 , curGL_PIXEL_UNPACK_BUFFERNo(0)
 , curGL_COPY_WRITE_BUFFERIdx(0)
 , curGL_COPY_WRITE_BUFFERNo(0)
 , curGL_DRAW_INDIRECT_BUFFERIdx(0)
 , curGL_DRAW_INDIRECT_BUFFERNo(0)
 , curGL_ATOMIC_COUNTER_BUFFERIdx(0)
 , curGL_ATOMIC_COUNTER_BUFFERNo(0)
 , curVertexArrayObjectNo(0)
 , curVertexArrayObjectIdx(DEFAULT_VAO_INDEX)//0)
{
    for(unsigned int i=0; i<MAX_GL_UNIFORM_BUFFER_NUM; i++)
    {
        curGL_UNIFORM_BUFFER0Idx[i] = 0;
        curGL_UNIFORM_BUFFER0No[i] = 0;
    }
    for(unsigned int i=0; i<MAX_GL_SHADER_STORAGE_BUFFER_NUM; i++)
    {
        curGL_SHADER_STORAGE_BUFFER0Idx[i] = 0;
        curGL_SHADER_STORAGE_BUFFER0No[i] = 0;
    }

    vertexArrayContext newVao;
    newVao.glGenVertexArraysNo = 0;
    allVaoList.insert(pair<unsigned int, vertexArrayContext>(DEFAULT_VAO_INDEX, newVao));
}

vertexArrayContext::vertexArrayContext()
 : glGenVertexArraysNo(0)
 , gl_element_array_bufferNo({0, 0, 0})
 , patchNum(199)//magic num for patch situation
{
}

clientSideBufferState::clientSideBufferState()
{
}

globalState::globalState()
 : TransformFeedbackSwitch(0)
 , computeMemoryBarrier(0)
 , glFrontFaceNo(0)
 , glEnableNo({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})
 , glCullFaceNo(0)
 , glDepthMaskNo(0)
 , glDepthFuncNo(0)
 , glBlendFuncNo(0)
 , glBlendEquationNo(0)
 , glViewportNo(0)
 , glFinishNo(0)
 , glFlushNo(0)
 , glScissorNo(0)
 , glClearColorNo(0)
 , glClearNo({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})
 , glClearDepthfNo(0)
 , glColorMaskNo(0)
 , glBlendColorNo(0)
 , glBlendFuncSeparateNo(0)
 , glStencilOpSeparateNo({0, 0})
 , glStencilMaskNo(0)
 , glPolygonOffsetNo(0)
 , glClearStencilNo(0)
 , glBlendEquationSeparateNo(0)
 , glHintNo({0, 0})
 , glLineWidthNo(0)
 , glStencilMaskSeparateNo({0, 0})
 , glDepthRangefNo(0)
 , glInvalidateFramebufferNo({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})
 , glSampleCoverageNo(0)
 , glDrawBuffersNo(0)
 , glPatchParameteriNo(0)
{
}

eglState::eglState()
{
}

frameBufferState::frameBufferState()
 : curReadFrameBufferIdx(0)
 , curReadFrameBufferNo(0)
 , curDrawFrameBufferIdx(0)
 , curDrawFrameBufferNo(0)
{
}

frameBufferContext::frameBufferContext()
 : glGenFramebuffersNo(0)
 , glFramebufferTexture2D_RenderbufferNo({0, 0, {0, 0}, 0, 0, {0, 0}, 0, 0, {0, 0}})
 , glDrawBuffersNo(0)
{
}

renderBufferState::renderBufferState()
 : curRenderBufferIdx(0)
 , curRenderBufferNo(0)
{
}

renderBufferContext::renderBufferContext()
 : glGenRenderbuffersNo(0)
 , glRenderbufferStorageNo({0, 0})
{
}

contextState::contextState(map<unsigned int, unsigned long long> &glClientSideBuffDataNoListForShare)
{
    curClientSideBufferState.glClientSideBuffDataNoList = &glClientSideBuffDataNoListForShare;
    share_context = 0;
}

void shaderState::ff_glCreatShader(unsigned int index, unsigned int type, unsigned int callNo)
{
    shaderContext newShader;
    newShader.glCreateShaderNo = callNo;
    newShader.shaderType = type;
    map<unsigned int, shaderContext>::iterator it = allShaderList.find(index);
    if(it != allShaderList.end())
    {
        allShaderList.erase(it);
    }
    allShaderList.insert(pair<unsigned int, shaderContext>(index, newShader));
    curShaderIdx = index;
}

void shaderState::ff_glShaderSource(unsigned int index, unsigned int callNo)
{
    map<unsigned int, shaderContext>::iterator it = allShaderList.find(index);
    if(it == allShaderList.end())
    {
        //printf("error 1:no %d shader\n", index);
        DBG_LOG("error:no %d shader\n", index);
        exit(1);
    }
    else
    {
        it->second.glShaderSourceNo = callNo;
    }
}

void shaderState::ff_glCompileShader(unsigned int index, unsigned int callNo)
{
    map<unsigned int, shaderContext>::iterator it = allShaderList.find(index);
    if(it == allShaderList.end())
    {
        //printf("error 2:no %d shader\n", index);
        DBG_LOG("error:no %d shader\n", index);
        exit(1);
    }
    else
    {
        it->second.glCompileShaderNo = callNo;
    }
}

unsigned int shaderState::getShaderType(unsigned int index)
{
    map<unsigned int, shaderContext>::iterator it = allShaderList.find(index);
    if(it == allShaderList.end())
    {
        //printf("error 3:no %d shader\n", index);
        DBG_LOG("error:no %d shader\n", index);
        return 0;
    }
    else
    {
        return it->second.shaderType;
    }
}

void shaderState::readCurShaderState(unsigned int index, unordered_set<unsigned int> &callNoList)
{
    map<unsigned int, shaderContext>::iterator it = allShaderList.find(index);
    callNoList.insert(it->second.glCreateShaderNo);
    callNoList.insert(it->second.glShaderSourceNo);
    callNoList.insert(it->second.glCompileShaderNo);
}

void shaderState::clear()
{
    curShaderIdx = 0;
    allShaderList.clear();
}

void programState::ff_glCreateProgram(unsigned int index, unsigned int callNo)
{
    programContext newProgram;
    newProgram.glCreateProgramNo = callNo;
    map<unsigned int, programContext>::iterator it = allProgramList.find(index);
    if(it != allProgramList.end())
    {
        allProgramList.erase(it);
    }
    allProgramList.insert(pair<unsigned int, programContext>(index, newProgram));
    //error create program doesn't change current program index
}

void programState::ff_glAttachShader(unsigned int index, unsigned int shaderIndex, unsigned int callNo)
{
    map<unsigned int, programContext>::iterator it = allProgramList.find(index);
    unsigned int shaderType = curShaderState.getShaderType(shaderIndex);
    if(shaderType == GL_VERTEX_SHADER)//35633)//GL_VERTEX_SHADER
    {
        it->second.glAttachShaderNo.gl_vertex_shaderNo = callNo;
        it->second.glAttachShaderNo.gl_vertex_shaderIdx = shaderIndex;
    }
    else if(shaderType == GL_FRAGMENT_SHADER)//35632)//GL_FRAGMENT_SHADER
    {
        it->second.glAttachShaderNo.gl_fragment_shaderNo = callNo;
        it->second.glAttachShaderNo.gl_fragment_shaderIdx = shaderIndex;
    }
    else if(shaderType == GL_COMPUTE_SHADER)//37305)//GL_COMPUTE_SHADER
    {
        it->second.glAttachShaderNo.gl_compute_shaderNo = callNo;
        it->second.glAttachShaderNo.gl_compute_shaderIdx = shaderIndex;
    }
    else if(shaderType == GL_TESS_CONTROL_SHADER)//36488)//GL_TESS_CONTROL_SHADER
    {
        it->second.glAttachShaderNo.gl_tess_control_shaderNo = callNo;
        it->second.glAttachShaderNo.gl_tess_control_shaderIdx = shaderIndex;
    }
    else if(shaderType == GL_TESS_EVALUATION_SHADER)//36487)//GL_TESS_EVALUATION_SHADER
    {
        it->second.glAttachShaderNo.gl_tess_evaluation_shaderNo = callNo;
        it->second.glAttachShaderNo.gl_tess_evaluation_shaderIdx = shaderIndex;
    }
    else if(shaderType == GL_GEOMETRY_SHADER)//36313)//GL_GEOMETRY_SHADER
    {
        it->second.glAttachShaderNo.gl_geometry_shaderNo = callNo;
        it->second.glAttachShaderNo.gl_geometry_shaderIdx = shaderIndex;
    }
    else
    {
        //printf("error 4:no %d shader in %d\n", index, callNo);
        DBG_LOG("error:no %d shader in %d\n", index, callNo);
    }
}

void programState::ff_glLinkProgram(unsigned int index, unsigned int callNo)
{
    map<unsigned int, programContext>::iterator it = allProgramList.find(index);
    if(it == allProgramList.end())
    {
        //printf("error 5:no %d program\n", index);
        DBG_LOG("error:no %d program\n", index);
    }
    else
    {
        it->second.glLinkProgramNo = callNo;
    }
    if(it != allProgramList.end())
    {
        map<unsigned int, shaderContext>::iterator potr = curShaderState.allShaderList.find(it->second.glAttachShaderNo.gl_vertex_shaderIdx);
        if(potr!=curShaderState.allShaderList.end())
        {
            it->second.GL_VERTEX_SHADERstateNo.glCreateShaderNo = potr->second.glCreateShaderNo;
            it->second.GL_VERTEX_SHADERstateNo.glShaderSourceNo = potr->second.glShaderSourceNo;
            it->second.GL_VERTEX_SHADERstateNo.glCompileShaderNo = potr->second.glCompileShaderNo;
        }
        potr = curShaderState.allShaderList.find(it->second.glAttachShaderNo.gl_fragment_shaderIdx);
        if(potr!=curShaderState.allShaderList.end())
        {
            it->second.GL_FRAGMENT_SHADERstateNo.glCreateShaderNo = potr->second.glCreateShaderNo;
            it->second.GL_FRAGMENT_SHADERstateNo.glShaderSourceNo = potr->second.glShaderSourceNo;
            it->second.GL_FRAGMENT_SHADERstateNo.glCompileShaderNo = potr->second.glCompileShaderNo;
        }
        potr = curShaderState.allShaderList.find(it->second.glAttachShaderNo.gl_compute_shaderIdx);
        if(potr!=curShaderState.allShaderList.end())
        {
            it->second.GL_COMPUTE_SHADERstateNo.glCreateShaderNo = potr->second.glCreateShaderNo;
            it->second.GL_COMPUTE_SHADERstateNo.glShaderSourceNo = potr->second.glShaderSourceNo;
            it->second.GL_COMPUTE_SHADERstateNo.glCompileShaderNo = potr->second.glCompileShaderNo;
        }
        potr = curShaderState.allShaderList.find(it->second.glAttachShaderNo.gl_tess_control_shaderIdx);
        if(potr!=curShaderState.allShaderList.end())
        {
            it->second.GL_TESS_CONTROL_SHADERstateNo.glCreateShaderNo = potr->second.glCreateShaderNo;
            it->second.GL_TESS_CONTROL_SHADERstateNo.glShaderSourceNo = potr->second.glShaderSourceNo;
            it->second.GL_TESS_CONTROL_SHADERstateNo.glCompileShaderNo = potr->second.glCompileShaderNo;
        }
        potr = curShaderState.allShaderList.find(it->second.glAttachShaderNo.gl_tess_evaluation_shaderIdx);
        if(potr!=curShaderState.allShaderList.end())
        {
            it->second.GL_TESS_EVALUATION_SHADERstateNo.glCreateShaderNo = potr->second.glCreateShaderNo;
            it->second.GL_TESS_EVALUATION_SHADERstateNo.glShaderSourceNo = potr->second.glShaderSourceNo;
            it->second.GL_TESS_EVALUATION_SHADERstateNo.glCompileShaderNo = potr->second.glCompileShaderNo;
        }
        potr = curShaderState.allShaderList.find(it->second.glAttachShaderNo.gl_geometry_shaderIdx);
        if(potr!=curShaderState.allShaderList.end())
        {
            it->second.GL_GEOMETRY_SHADERstateNo.glCreateShaderNo = potr->second.glCreateShaderNo;
            it->second.GL_GEOMETRY_SHADERstateNo.glShaderSourceNo = potr->second.glShaderSourceNo;
            it->second.GL_GEOMETRY_SHADERstateNo.glCompileShaderNo = potr->second.glCompileShaderNo;
        }
    }//if
}

void programState::ff_glBindAttribLocation(unsigned int index, char* name, unsigned int callNo)
{
    map<unsigned int, programContext>::iterator it = allProgramList.find(index);
    if(it != allProgramList.end())
    {
        map<string, unsigned int>::iterator attrib = it->second.glBindAttribLocationNoList.find(name);
        if(attrib == it->second.glBindAttribLocationNoList.end())
        {
            it->second.glBindAttribLocationNoList.insert(pair<string, unsigned int>(name, callNo));
        }
        else
        {
            attrib->second = callNo;
        }
    }
    else
    {
        //printf("error 6:no %d program\n", index);
        DBG_LOG("error:no %d program\n", index);
    }
}

void programState::ff_glValidateProgram(unsigned int index, unsigned int callNo)
{
    map<unsigned int, programContext>::iterator it = allProgramList.find(index);
    if(it == allProgramList.end())
    {
        //printf("error 7:no %d program\n", index);
        DBG_LOG("error:no %d program\n", index);
        exit(1);
    }
    else
    {
        it->second.glValidateProgramNo = callNo;
    }
}

void programState::ff_glGetAttribLocation(unsigned int index, char* name, unsigned int callNo)
{
    unsigned int binderCallNo = 0;

    map<unsigned int, programContext>::iterator it = allProgramList.find(index);
    map<string, programContext::withOneBinder>::iterator attrib = it->second.glGetAttribLocationNoList.find(name);

    map<string, unsigned int>::iterator attrib2 = it->second.glBindAttribLocationNoList.find(name);

    if(attrib2 == it->second.glBindAttribLocationNoList.end())
    {
        binderCallNo = 0;
    }
    else
    {
        binderCallNo = attrib2->second;
    }

    programContext::withOneBinder oneBinder = {binderCallNo, callNo};

    if(attrib == it->second.glGetAttribLocationNoList.end())
    {
        it->second.glGetAttribLocationNoList.insert(pair<string, programContext::withOneBinder>(name, oneBinder));
    }
    else
    {
        attrib->second = oneBinder;
    }
}

void programState::ff_glProgramBinary(unsigned int index, unsigned int callNo)
{
    map<unsigned int, programContext>::iterator it = allProgramList.find(index);
    if(it == allProgramList.end())
    {
        //printf("error 8:glprogrambinary :no %d program\n", index);
        DBG_LOG("error:glprogrambinary :no %d program\n", index);
        exit(1);
    }
    else
    {
        it->second.glProgramBinaryNo = callNo;
    }
}

//to fix

void programState::debug()
{
}

void programState::ff_glGetUniformLocation(unsigned int index, char* name, unsigned int location, unsigned int callNo)
{
    map<unsigned int, programContext>::iterator it = allProgramList.find(index);
    if(it!=allProgramList.end())
    {
        map<string, unsigned int>::iterator attrib = it->second.glGetUniformLocationNoList.find(name);
        if(attrib == it->second.glGetUniformLocationNoList.end())
        {
            it->second.glGetUniformLocationNoList.insert(pair<string, unsigned int>(name, callNo));
        }
        else
        {
            attrib->second = callNo;
        }
        map<unsigned int, unsigned int>::iterator attrib2 = it->second.glGetUniformLocationNoIndexList.find(location);
        if(attrib2 == it->second.glGetUniformLocationNoIndexList.end())
        {
            it->second.glGetUniformLocationNoIndexList.insert(pair<unsigned int, unsigned int>(location, callNo));
        }
        else
        {
            attrib2->second = callNo;
        }
    }//if
    else
    {
        //printf("error 9: no program before glGetUniformLocation\n");
        DBG_LOG("error: no program before glGetUniformLocation\n");
    }
}

void programState::ff_glUniformBlockBinding(unsigned int program, unsigned int callNo)
{
    map<unsigned int, programContext>::iterator it = allProgramList.find(program);
    if(it!=allProgramList.end())
    {
        it->second.glUniformBlockBindingList.push_back(callNo);
    }
    else
    {
        //printf("error 10: no program before glUniformBlockBinding\n");
        DBG_LOG("error: no program before glUniformBlockBinding\n");
    }
}

void programState::ff_glUseProgram(unsigned int index, unsigned int callNo)
{
    curProgramIdx = index;
    curProgramNo = callNo;
}
//all glUniform functions
void programState::ff_glUniform_Types(unsigned int LocationIndex, unsigned int callNo)
{
    map<unsigned int, programContext>::iterator it = allProgramList.find(curProgramIdx);
    unsigned int getUniformNo = 0;
    if(it != allProgramList.end())
    {
        map<unsigned int, programContext::withBinder>::iterator attrib = it->second.glUniform_NoList.find(LocationIndex);
        map<unsigned int, unsigned int>::iterator attrib2 = it->second.glGetUniformLocationNoIndexList.find(LocationIndex);
        if(attrib2 != it->second.glGetUniformLocationNoIndexList.end())
        {
            getUniformNo = attrib2->second;
        }
        programContext::withBinder curBinder = {callNo, curProgramNo, getUniformNo};
        if(attrib == it->second.glUniform_NoList.end())
        {
            it->second.glUniform_NoList.insert(pair<unsigned int, programContext::withBinder>(LocationIndex, curBinder));
        }
        else
        {
            attrib->second = curBinder;
        }
    }
    else
    {
        //printf("error 11: no useProgram!! before %d\n", callNo);
        DBG_LOG("error: no useProgram!! before %d\n", callNo);
    }
}

void programState::readCurProgramState(unordered_set<unsigned int> &callNoList)///*vector<unsigned int> &clientSideBufferList, vector<unsigned int> &bufferList, */
{
    if(curProgramNo != 0)
    {
        callNoList.insert(curProgramNo);
    }
    else
    {
        //printf("error 12: program index number == 0\n");
        DBG_LOG("error: program index number == 0\n");
        exit(1);
    }
    if(curProgramIdx != 0 )
    {
        map<unsigned int, programContext>::iterator it = allProgramList.find(curProgramIdx);
        callNoList.insert(it->second.glCreateProgramNo);
        callNoList.insert(it->second.glAttachShaderNo.gl_vertex_shaderNo);
        callNoList.insert(it->second.glAttachShaderNo.gl_fragment_shaderNo);
        callNoList.insert(it->second.glAttachShaderNo.gl_compute_shaderNo);
        callNoList.insert(it->second.glAttachShaderNo.gl_tess_control_shaderNo);
        callNoList.insert(it->second.glAttachShaderNo.gl_tess_evaluation_shaderNo);
        callNoList.insert(it->second.glAttachShaderNo.gl_geometry_shaderNo);
        callNoList.insert(it->second.GL_VERTEX_SHADERstateNo.glCreateShaderNo);
        callNoList.insert(it->second.GL_VERTEX_SHADERstateNo.glShaderSourceNo);
        callNoList.insert(it->second.GL_VERTEX_SHADERstateNo.glCompileShaderNo);
        callNoList.insert(it->second.GL_FRAGMENT_SHADERstateNo.glCreateShaderNo);
        callNoList.insert(it->second.GL_FRAGMENT_SHADERstateNo.glShaderSourceNo);
        callNoList.insert(it->second.GL_FRAGMENT_SHADERstateNo.glCompileShaderNo);
        callNoList.insert(it->second.GL_COMPUTE_SHADERstateNo.glCreateShaderNo);
        callNoList.insert(it->second.GL_COMPUTE_SHADERstateNo.glShaderSourceNo);
        callNoList.insert(it->second.GL_COMPUTE_SHADERstateNo.glCompileShaderNo);
        callNoList.insert(it->second.GL_TESS_CONTROL_SHADERstateNo.glCreateShaderNo);
        callNoList.insert(it->second.GL_TESS_CONTROL_SHADERstateNo.glShaderSourceNo);
        callNoList.insert(it->second.GL_TESS_CONTROL_SHADERstateNo.glCompileShaderNo);
        callNoList.insert(it->second.GL_TESS_EVALUATION_SHADERstateNo.glCreateShaderNo);
        callNoList.insert(it->second.GL_TESS_EVALUATION_SHADERstateNo.glShaderSourceNo);
        callNoList.insert(it->second.GL_TESS_EVALUATION_SHADERstateNo.glCompileShaderNo);
        callNoList.insert(it->second.GL_GEOMETRY_SHADERstateNo.glCreateShaderNo);
        callNoList.insert(it->second.GL_GEOMETRY_SHADERstateNo.glShaderSourceNo);
        callNoList.insert(it->second.GL_GEOMETRY_SHADERstateNo.glCompileShaderNo);
        callNoList.insert(it->second.glLinkProgramNo);
        //printf glBindAttribLocation map
        map<string, unsigned int>::iterator attrib = it->second.glBindAttribLocationNoList.begin();
        while(attrib != it->second.glBindAttribLocationNoList.end())
        {
            callNoList.insert(attrib->second);
            attrib++;
        }
        callNoList.insert(it->second.glValidateProgramNo);
        //printf glGetAttribLocation map
        map<string, programContext::withOneBinder>::iterator attrib2 = it->second.glGetAttribLocationNoList.begin();
        while(attrib2 != it->second.glGetAttribLocationNoList.end())
        {
            callNoList.insert(attrib2->second.binderCallNo);
            callNoList.insert(attrib2->second.thisCallNo);
            attrib2++;
        }
        //printf glGetUniformLocation map
        attrib = it->second.glGetUniformLocationNoList.begin();
        while(attrib != it->second.glGetUniformLocationNoList.end())
        {
            callNoList.insert(attrib->second);
            attrib++;
        }
        //printf glUniform_ map
        map<unsigned int, programContext::withBinder>::iterator unit = it->second.glUniform_NoList.begin();
        while(unit != it->second.glUniform_NoList.end())
        {
            callNoList.insert(unit->second.thisCallNo);
            callNoList.insert(unit->second.binderCallNo);
            callNoList.insert(unit->second.glGetUniformLocationNo);
            unit++;
        }
        callNoList.insert(it->second.glProgramBinaryNo);
        vector<unsigned int>::iterator itt = it->second.glUniformBlockBindingList.begin();
        while(itt != it->second.glUniformBlockBindingList.end())
        {
            callNoList.insert(*itt);
            itt++;
        }
    }//    if(curProgramIdx != 0 )
    else
    {
        //printf("warning 1:binding program idx is 0\n");
        DBG_LOG("warning:binding program idx is 0\n");
    }
}

void programState::clear()
{
    curShaderState.clear();
    curProgramIdx = 0;
    curProgramNo = 0;
    allProgramList.clear();
}

void textureState::fordebug()
{
/*    std::map<unsigned int, samplerContext>::iterator it = allSamplerList.begin();
    if(it != allSamplerList.end())
    {
        printf("~~~  %d  %d  %d\n",it->second.glGenSamplersNo,it->second.glSamplerParameterNo.gl_texture_wrap_sNo,it->second.glSamplerParameterNo.gl_texture_compare_funcNo);
        it++;
    }
*/
//allSamplerList.clear();
//printf("!!!!!!!!!!!1   %d \n", curActiveTextureNo);
}

void textureState::ff_glGenTextures(unsigned int size, unsigned int index[], unsigned int callNo)
{
    for(unsigned int i=0; i<size; i++)
    {
        textureContext newTexture;
        newTexture.glGenTexturesNo = callNo;
        map<unsigned int, textureContext>::iterator it = allTextureList.find(index[i]);
        if(it != allTextureList.end())
        {
            allTextureList.erase(it);
        }
        allTextureList.insert(pair<unsigned int, textureContext>(index[i], newTexture));
    }
}

void textureState::ff_glGenSamplers(unsigned int size, unsigned int index[], unsigned int callNo)
{
    for(unsigned int i=0; i<size; i++)
    {
        samplerContext newSampler;
        newSampler.glGenSamplersNo = callNo;
        map<unsigned int, samplerContext>::iterator it = allSamplerList.find(index[i]);
        if(it != allSamplerList.end())
        {
            allSamplerList.erase(it);
        }
        allSamplerList.insert(pair<unsigned int, samplerContext>(index[i], newSampler));
    }
}

void textureState::ff_glBindImageTexture(unsigned int unit, unsigned int index, unsigned int access, unsigned int callNo)
{
    if(access == GL_WRITE_ONLY)//35001)//GL_WRITE_ONLY
    {
        activeTextureNo[191-unit].curTextureIdx = index;//max texture idx == 191
        activeTextureNo[191-unit].curTextureNo = callNo;//activeTextureNo[] reserve 6 elements for GL_WRITE_ONLY (186 to 191) 
    }
    else if(access == GL_READ_ONLY)//35000)//GL_READ_ONLY)
    {
        activeTextureNo[185-unit].curTextureIdx = index;//activeTextureNo[] reserve 6 elements for GL_READ_ONLY (180 to 185)
        activeTextureNo[185-unit].curTextureNo = callNo;
    }
    else
    {
        //printf("error 13: glBindImageTexture no support parameter\n");
        DBG_LOG("error: glBindImageTexture no support parameter\n");
    }
}

void textureState::ff_glBindTexture(unsigned int index, unsigned int type, unsigned int callNo)
{
    //curActiveTextureIdx = lastActiveTextureIdx;
    //curActiveTextureNo = lastActiveTextureNo;
    map<unsigned int, textureContext>::iterator it = allTextureList.find(index);
    if(it == allTextureList.end())
    {
        textureContext newTexture;
        newTexture.glGenTexturesNo = 0;
        allTextureList.insert(pair<unsigned int, textureContext>(index, newTexture));
        DBG_LOG("error: no GenTextures before BindTexture !! in %d\n",callNo);
    }

        activeTextureNo[curActiveTextureIdx].curTextureIdx = index;
        activeTextureNo[curActiveTextureIdx].curTextureNo = callNo;
        activeTextureNo[curActiveTextureIdx].curActiveTextureIdx = curActiveTextureIdx;
        activeTextureNo[curActiveTextureIdx].curActiveTextureNo = curActiveTextureNo;
        if(type == GL_TEXTURE_CUBE_MAP||type == GL_TEXTURE_2D_ARRAY)
        {
            activeTextureNo[128+curActiveTextureIdx].curTextureIdx = index;//128 to 159 save for GL_TEXTURE_CUBE_MAP or GL_TEXTURE_2D_ARRAY
            activeTextureNo[128+curActiveTextureIdx].curTextureNo = callNo;
            activeTextureNo[128+curActiveTextureIdx].curActiveTextureIdx = curActiveTextureIdx;
            activeTextureNo[128+curActiveTextureIdx].curActiveTextureNo = curActiveTextureNo;
        }
        if(type == GL_TEXTURE_3D)
        {
            activeTextureNo[160+curActiveTextureIdx].curTextureIdx = index;//160 to 179 save for GL_TEXTURE_3D
            activeTextureNo[160+curActiveTextureIdx].curTextureNo = callNo;
            activeTextureNo[160+curActiveTextureIdx].curActiveTextureIdx = curActiveTextureIdx;
            activeTextureNo[160+curActiveTextureIdx].curActiveTextureNo = curActiveTextureNo;
        }
        else if(type == GL_TEXTURE_2D)
        {
            activeTextureNo[64+curActiveTextureIdx].curTextureIdx = index;//64 to 127 save for GL_TEXTURE_2D
            activeTextureNo[64+curActiveTextureIdx].curTextureNo = callNo;
            activeTextureNo[64+curActiveTextureIdx].curActiveTextureIdx = curActiveTextureIdx;
            activeTextureNo[64+curActiveTextureIdx].curActiveTextureNo = curActiveTextureNo;
        }
        if(type != GL_TEXTURE_CUBE_MAP && type != GL_TEXTURE_2D )
        {
            //printf("warning 2:bind texture type error  %X\n", type);
            //DBG_LOG("warning 2:bind texture type error  %X   index %d callno  %d\n", type, index, callNo);
        }
}

void textureState::ff_glTexParameter_Types(unsigned int Typeindex, unsigned int type, unsigned int callNo)
{
    if(activeTextureNo[curActiveTextureIdx].curTextureIdx != 0)
    {
        map<unsigned int, textureContext>::iterator it = allTextureList.find(activeTextureNo[curActiveTextureIdx].curTextureIdx);

        textureContext::withBinderForParameterState curBinder = {callNo, activeTextureNo[curActiveTextureIdx].curTextureNo, activeTextureNo[curActiveTextureIdx].curActiveTextureNo, curActiveTextureNo};

        if(type == GL_TEXTURE_MIN_FILTER)//10241)//GL_TEXTURE_MIN_FILTER
        {
            it->second.glTexParameterNo.gl_texture_min_filterNo = curBinder;
        }
        else if(type == GL_TEXTURE_MAG_FILTER)//10240)//GL_TEXTURE_MAG_FILTER
        {
            it->second.glTexParameterNo.gl_texture_mag_filterNo = curBinder;
        }
        else if(type == GL_TEXTURE_WRAP_S)//10242)//GL_TEXTURE_WRAP_S
        {
            it->second.glTexParameterNo.gl_texture_wrap_sNo = curBinder;
        }
        else if(type == GL_TEXTURE_WRAP_R)//32882)//GL_TEXTURE_WRAP_R
        {
            it->second.glTexParameterNo.gl_texture_wrap_rNo = curBinder;
        }
        else if(type == GL_TEXTURE_WRAP_T)//10243)//GL_TEXTURE_WRAP_T
        {
            it->second.glTexParameterNo.gl_texture_wrap_tNo = curBinder;
        }
        else if(type == GL_TEXTURE_MAX_LEVEL)//33085)//GL_TEXTURE_MAX_LEVEL
        {
            it->second.glTexParameterNo.gl_texture_max_levelNo = curBinder;
        }
        else if(type == GL_TEXTURE_SWIZZLE_A)//33085)//GL_TEXTURE_MAX_LEVEL
        {
            it->second.glTexParameterNo.gl_texture_swizzle_aNo = curBinder;
        }
        else if(type == GL_TEXTURE_SWIZZLE_R)//33085)//GL_TEXTURE_MAX_LEVEL
        {
            it->second.glTexParameterNo.gl_texture_swizzle_rNo = curBinder;
        }
        else if(type == GL_TEXTURE_SWIZZLE_G)//33085)//GL_TEXTURE_MAX_LEVEL
        {
            it->second.glTexParameterNo.gl_texture_swizzle_gNo = curBinder;
        }
        else if(type == GL_TEXTURE_SWIZZLE_B)//33085)//GL_TEXTURE_MAX_LEVEL
        {
            it->second.glTexParameterNo.gl_texture_swizzle_bNo = curBinder;
        }
        else if(type == GL_TEXTURE_BASE_LEVEL)//33084)//GL_TEXTURE_BASE_LEVEL
        {
            it->second.glTexParameterNo.gl_texture_base_levelNo = curBinder;
        }
        else if(type == GL_TEXTURE_COMPARE_MODE)//33084)//GL_TEXTURE_BASE_LEVEL
        {
            it->second.glTexParameterNo.gl_texture_compare_modeNo = curBinder;
        }
        else if(type == GL_TEXTURE_COMPARE_FUNC)//33084)//GL_TEXTURE_BASE_LEVEL
        {
            it->second.glTexParameterNo.gl_texture_compare_funcNo = curBinder;
        }
        else if(type == GL_TEXTURE_SRGB_DECODE_EXT)//35400)//GL_TEXTURE_SRGB_DECODE_EXT
        {
            it->second.glTexParameterNo.gl_texture_srgs_decode_extNo = curBinder;
        }
        else if(type == GL_TEXTURE_MIN_LOD)
        {
            it->second.glTexParameterNo.gl_texture_min_lodNo = curBinder;
        }
        else if(type == GL_TEXTURE_MAX_LOD)
        {
            it->second.glTexParameterNo.gl_texture_max_lodNo = curBinder;
        }
        else
        {
            //printf("error 14:no support type %d in %d\n", type, callNo);
            DBG_LOG("error:no support type %d in %d\n", type, callNo);
            exit(1);
        }
    }//if(activeTextureNo[curActiveTextureIdx].curTextureId!=0)
    else
    {
        //printf("error 15: glTexParameter  bindtexture = 0\n");
        DBG_LOG("error: glTexParameter  bindtexture = 0\n");
    }
}

void textureState::ff_glSamplerParameter_Types(unsigned int index, unsigned int type, unsigned int callNo)
{
    map<unsigned int, samplerContext>::iterator it = allSamplerList.find(index);
    if(it == allSamplerList.end())
    {
        //printf("error 16: no genSampler!\n");
        DBG_LOG("error: no genSampler!\n");
        exit(1);
    }

    if(type == GL_TEXTURE_MIN_FILTER)//GL_TEXTURE_MIN_FILTER
    {
        it->second.glSamplerParameterNo.gl_texture_min_filterNo = callNo;
    }
    else if(type == GL_TEXTURE_MAG_FILTER)//GL_TEXTURE_MAG_FILTER
    {
        it->second.glSamplerParameterNo.gl_texture_mag_filterNo = callNo;
    }
    else if(type == GL_TEXTURE_WRAP_S)//GL_TEXTURE_WRAP_S
    {
        it->second.glSamplerParameterNo.gl_texture_wrap_sNo = callNo;
    }
    else if(type == GL_TEXTURE_WRAP_R)//GL_TEXTURE_WRAP_R
    {
        it->second.glSamplerParameterNo.gl_texture_wrap_rNo = callNo;
    }
    else if(type == GL_TEXTURE_WRAP_T)//GL_TEXTURE_WRAP_T
    {
        it->second.glSamplerParameterNo.gl_texture_wrap_tNo = callNo;
    }
    else if(type == GL_TEXTURE_MAX_LOD)//GL_TEXTURE_MAX_LOD
    {
        it->second.glSamplerParameterNo.gl_texture_max_lodNo = callNo;
    }
    else if(type == GL_TEXTURE_MIN_LOD)//GL_TEXTURE_MIN_LOD
    {
        it->second.glSamplerParameterNo.gl_texture_min_lodNo = callNo;
    }
    else if(type == GL_TEXTURE_COMPARE_MODE)//GL_TEXTURE_COMPARE_MODE
    {
        it->second.glSamplerParameterNo.gl_texture_compare_modeNo = callNo;
    }
    else if(type == GL_TEXTURE_COMPARE_FUNC)//GL_TEXTURE_COMPARE_FUNC
    {
        it->second.glSamplerParameterNo.gl_texture_compare_funcNo = callNo;
    }
    else
    {
        //printf("error 17:no support type %d in %d\n", type, callNo);
        DBG_LOG("error:no support type %d in %d\n", type, callNo);
        exit(1);
    }
}

//and glCompressedTexImage2D
void textureState::ff_glTypes_TexImage2D(unsigned int TypeIndex, unsigned int GL_PIXEL_UNPACK_BUFFERNo, unsigned int callNo, unordered_set<unsigned int> &callNoList)
{
    map<unsigned int, textureContext>::iterator it = allTextureList.find(activeTextureNo[curActiveTextureIdx].curTextureIdx);
    textureContext::imageWithBinder curBinder = {callNo, activeTextureNo[curActiveTextureIdx].curTextureNo, activeTextureNo[curActiveTextureIdx].curActiveTextureNo, curActiveTextureNo, globalGlPixelStoreiNo.gl_pack_row_lengthNo,
 globalGlPixelStoreiNo.gl_pack_skip_pixelsNo, globalGlPixelStoreiNo.gl_pack_skip_rowsNo,globalGlPixelStoreiNo.gl_pack_alignmentNo, globalGlPixelStoreiNo.gl_unpack_row_lengthNo, globalGlPixelStoreiNo.gl_unpack_image_heightNo,
 globalGlPixelStoreiNo.gl_unpack_skip_pixelsNo, globalGlPixelStoreiNo.gl_unpack_skip_rowsNo, globalGlPixelStoreiNo.gl_unpack_skip_imagesNo, globalGlPixelStoreiNo.gl_unpack_alignmentNo,
 globalGlPixelStoreiNo.gl_unpack_alignmentBinderNo, globalGlPixelStoreiNo.gl_unpack_alignmentActiveNo, GL_PIXEL_UNPACK_BUFFERNo, it->second.glTexParameterNo.gl_texture_min_filterNo};
    //first number shoul be zero
    it->second.glTexImage2DIdx++;
    it->second.glTexImage2DNoList.insert(pair<unsigned int, textureContext::imageWithBinder>(it->second.glTexImage2DIdx, curBinder));
    if(GL_PIXEL_UNPACK_BUFFERNo != 0&&callNoList.size()!=0)
    {
        unordered_set<unsigned int> newSet;
        newSet.insert(callNoList.begin(), callNoList.end());
        it->second.glTexImage2DGL_PIXEL_UNPACK_BUFFERList.insert(pair<unsigned int, unordered_set<unsigned int>>(it->second.glTexImage2DIdx, newSet));
    }
}

void textureState::ff_glPixelStorei(unsigned int type, unsigned int param, unsigned int callNo)
{
    if(type == GL_PACK_ROW_LENGTH)
    {
        globalGlPixelStoreiNo.gl_pack_row_lengthNo = callNo;
    }
    else if(type == GL_PACK_SKIP_PIXELS)
    {
        globalGlPixelStoreiNo.gl_pack_skip_pixelsNo = callNo;
    }
    else if(type == GL_PACK_SKIP_ROWS)
    {
        globalGlPixelStoreiNo.gl_pack_skip_rowsNo = callNo;
    }
    else if(type == GL_PACK_ALIGNMENT)
    {
        globalGlPixelStoreiNo.gl_pack_alignmentNo = callNo;
    }
    else if(type == GL_UNPACK_ROW_LENGTH)
    {
        globalGlPixelStoreiNo.gl_unpack_row_lengthNo = callNo;
    }
    else if(type == GL_UNPACK_IMAGE_HEIGHT)
    {
        globalGlPixelStoreiNo.gl_unpack_image_heightNo = callNo;
    }
    else if(type == GL_UNPACK_SKIP_PIXELS)
    {
        globalGlPixelStoreiNo.gl_unpack_skip_pixelsNo = callNo;
    }
    else if(type == GL_UNPACK_SKIP_ROWS)
    {
        globalGlPixelStoreiNo.gl_unpack_skip_rowsNo = callNo;
    }
    else if(type == GL_UNPACK_SKIP_IMAGES)
    {
        globalGlPixelStoreiNo.gl_unpack_skip_imagesNo = callNo;
    }
    else if(type == GL_UNPACK_ALIGNMENT)
    {
        globalGlPixelStoreiNo.gl_unpack_alignmentNo = callNo;
        globalGlPixelStoreiNo.gl_unpack_alignmentBinderNo = activeTextureNo[curActiveTextureIdx].curTextureNo;
        globalGlPixelStoreiNo.gl_unpack_alignmentActiveNo = curActiveTextureNo;
    }
    else
    {
        //printf("error 18:no support type %d\n", type);
        DBG_LOG("error:no support type %d\n", type);
        exit(1);
    }
}

void textureState::ff_glActiveTexture(unsigned int TypeIndex, unsigned int callNo)
{
        //GL_TEXTURE0  :  33984
        if(TypeIndex<=33984+60)//support max num is GL_TEXTURE60
        {
            curActiveTextureIdx = TypeIndex - 33984;
            curActiveTextureNo = callNo;
            //lastActiveTextureIdx = TypeIndex - 33984;
            //lastActiveTextureNo = callNo;
        }
        else
        {
            //printf("warning 4 :too big index in activetexture %d\n", TypeIndex - 33984);
            DBG_LOG("warning: too big index in activetexture %d\n", TypeIndex - 33984);
        }
}

void textureState::ff_glTypes_TexSubImage2D(unsigned int Typeindex, unsigned int GL_PIXEL_UNPACK_BUFFERNo, unsigned int callNo, unordered_set<unsigned int> &callNoList)
{
    map<unsigned int, textureContext>::iterator it = allTextureList.find(activeTextureNo[curActiveTextureIdx].curTextureIdx);
    if(Typeindex == GL_TEXTURE_3D)
    {
        it = allTextureList.find(activeTextureNo[curActiveTextureIdx+160].curTextureIdx);//160 to 179 save for GL_TEXTURE_3D
    }
    if(it == allTextureList.end())
    {
        DBG_LOG("warning: no texture %d\n", callNo);
    }
    else
    {
        textureContext::imageWithBinder curBinder = {callNo, activeTextureNo[curActiveTextureIdx].curTextureNo, activeTextureNo[curActiveTextureIdx].curActiveTextureNo, curActiveTextureNo, globalGlPixelStoreiNo.gl_pack_row_lengthNo,
        globalGlPixelStoreiNo.gl_pack_skip_pixelsNo, globalGlPixelStoreiNo.gl_pack_skip_rowsNo, globalGlPixelStoreiNo.gl_pack_alignmentNo, globalGlPixelStoreiNo.gl_unpack_row_lengthNo, globalGlPixelStoreiNo.gl_unpack_image_heightNo,
        globalGlPixelStoreiNo.gl_unpack_skip_pixelsNo, globalGlPixelStoreiNo.gl_unpack_skip_rowsNo, globalGlPixelStoreiNo.gl_unpack_skip_imagesNo, globalGlPixelStoreiNo.gl_unpack_alignmentNo,
        globalGlPixelStoreiNo.gl_unpack_alignmentBinderNo, globalGlPixelStoreiNo.gl_unpack_alignmentActiveNo, GL_PIXEL_UNPACK_BUFFERNo, it->second.glTexParameterNo.gl_texture_min_filterNo};
        //first number should be zero
        it->second.glTexSubImage2DIdx++;
        it->second.glTexSubImage2DNoList.insert(pair<unsigned int, textureContext::imageWithBinder>(it->second.glTexSubImage2DIdx, curBinder));
        if(GL_PIXEL_UNPACK_BUFFERNo != 0 && callNoList.size() != 0)
        {
            unordered_set<unsigned int> newSet;
            newSet.insert(callNoList.begin(), callNoList.end());
            it->second.glTexSubImage2DGL_PIXEL_UNPACK_BUFFERList.insert(pair<unsigned int, unordered_set<unsigned int>>(it->second.glTexSubImage2DIdx, newSet));
        }
    }//else
}

void textureState::ff_glGenerateMipmap(unsigned int callNo)
{
    map<unsigned int, textureContext>::iterator it = allTextureList.find(activeTextureNo[curActiveTextureIdx].curTextureIdx);
    textureContext::withBinderForGenerate curBinder = {callNo, activeTextureNo[curActiveTextureIdx].curTextureNo, activeTextureNo[curActiveTextureIdx].curActiveTextureNo, curActiveTextureNo};
    it->second.glGenerateMipmapNo = curBinder;
}

void textureState::ff_glBindSampler(unsigned int unit, unsigned int index, unsigned int callNo)
{
    if(unit >= MAX_SAMPLER_NUM)
    {
        //DBG_LOG("error: sampler unit %d bigger than 31\n", unit);
        //exit(1);
    }
    else
    {
        map<unsigned int, textureContext>::iterator it = allTextureList.find(activeTextureNo[curActiveTextureIdx].curTextureIdx);
        textureContext::withBinderSampler curBinder = {callNo, activeTextureNo[curActiveTextureIdx].curTextureNo, activeTextureNo[curActiveTextureIdx].curActiveTextureNo, index};
        if(it != allTextureList.end())
        {
        it->second.glBindSamplerNo[unit] = curBinder;
        }
        else
        {
            //printf("no texture before sampler\n");//for chineseghoststory
        }
        map<unsigned int, unsigned int>::iterator it2 = allSamplerBindingList.find(unit);
        if(it2 != allSamplerBindingList.end())
        {
            it2->second = callNo;
        }
        else
        {
            allSamplerBindingList.insert(pair<unsigned int, unsigned int>(unit, callNo));
        }
    }//else < 31
}

void textureState::ff_glCopyImageSubData(unsigned int srcName, unsigned int destName, unsigned int callNo)
{
    map<unsigned int, textureContext>::iterator it = allTextureList.find(destName);
    if(it == allTextureList.end())
    {
        DBG_LOG("warning: no texture idx %d in %d\n", destName, callNo);
    }
    else
    {
        it->second.glCopyImageSubDataNoList.push_back(callNo);

        vector<unsigned int> copyImageIdxList;
        copyImageIdxList.push_back(srcName);
        findTextureContext(copyImageIdxList, it->second.glCopyImageSubData_textureContextNoList);
    }
}

void textureState::findTextureContext(vector<unsigned int> &textureList, unordered_set<unsigned int> &callNoList)
{
    for(unsigned int i=0; i<textureList.size(); i++)
    {
        if(textureList[i] == 0)continue;//not
        textureList[i] = textureList[i] % CUBE_MAP_INDEX;
        map<unsigned int, textureContext>::iterator it = allTextureList.find(textureList[i]);
            callNoList.insert(it->second.glGenTexturesNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_sNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_sNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_sNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_sNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_rNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_rNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_rNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_rNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_tNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_tNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_tNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_tNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_mag_filterNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_mag_filterNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_mag_filterNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_mag_filterNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_min_filterNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_min_filterNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_min_filterNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_min_filterNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_max_levelNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_max_levelNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_max_levelNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_max_levelNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_aNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_aNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_aNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_aNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_rNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_rNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_rNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_rNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_gNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_gNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_gNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_gNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_bNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_bNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_bNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_bNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_base_levelNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_base_levelNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_base_levelNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_base_levelNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_modeNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_modeNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_modeNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_modeNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_funcNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_funcNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_funcNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_funcNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.activeCallNo2);

            callNoList.insert(it->second.glTexParameterNo.gl_texture_min_lodNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_min_lodNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_min_lodNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_min_lodNo.activeCallNo2);

            callNoList.insert(it->second.glTexParameterNo.gl_texture_max_lodNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_max_lodNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_max_lodNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_max_lodNo.activeCallNo2);
            for(unsigned unit=0; unit<MAX_SAMPLER_NUM; unit++)
            {
            callNoList.insert(it->second.glBindSamplerNo[unit].thisCallNo);
            callNoList.insert(it->second.glBindSamplerNo[unit].binderCallNo);
            callNoList.insert(it->second.glBindSamplerNo[unit].activeCallNo);
            if(it->second.glBindSamplerNo[unit].bindSamplerIdx != 0)
            {
                map<unsigned int, samplerContext>::iterator itsa = allSamplerList.find(it->second.glBindSamplerNo[unit].bindSamplerIdx);
                callNoList.insert(itsa->second.glGenSamplersNo);
                callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_wrap_sNo);
                callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_wrap_rNo);
                callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_wrap_tNo);
                callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_mag_filterNo);
                callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_min_filterNo);
                callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_min_lodNo);
                callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_max_lodNo);
                callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_compare_modeNo);
                callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_compare_funcNo);
            }
            }
            map<unsigned int, unsigned int>::iterator itsa2 = allSamplerBindingList.begin();
            while(itsa2 != allSamplerBindingList.end())
            {
                callNoList.insert(itsa2->second);
                itsa2++;
            }
            callNoList.insert(it->second.glGenerateMipmapNo.thisCallNo);
            callNoList.insert(it->second.glGenerateMipmapNo.binderCallNo);
            callNoList.insert(it->second.glGenerateMipmapNo.activeCallNo);
            callNoList.insert(it->second.glGenerateMipmapNo.nowActiveCallNo);
            //printf glTexImage2DNoList
            map<unsigned int, textureContext::imageWithBinder>::iterator potr = it->second.glTexImage2DNoList.begin();
            while(potr != it->second.glTexImage2DNoList.end())
            {
                callNoList.insert(potr->second.thisCallNo);
                callNoList.insert(potr->second.binderCallNo);
                callNoList.insert(potr->second.activeCallNo);
                callNoList.insert(potr->second.activeCallNo2);
                callNoList.insert(potr->second.gl_pack_row_lengthNo);
                callNoList.insert(potr->second.gl_pack_skip_pixelsNo);
                callNoList.insert(potr->second.gl_pack_skip_rowsNo);
                callNoList.insert(potr->second.gl_pack_alignmentNo);
                callNoList.insert(potr->second.gl_unpack_row_lengthNo);
                callNoList.insert(potr->second.gl_unpack_image_heightNo);
                callNoList.insert(potr->second.gl_unpack_skip_pixelsNo);
                callNoList.insert(potr->second.gl_unpack_skip_rowsNo);
                callNoList.insert(potr->second.gl_unpack_skip_imagesNo);
                callNoList.insert(potr->second.gl_unpack_alignmentNo);
                callNoList.insert(potr->second.gl_unpack_alignmentBinderNo);
                callNoList.insert(potr->second.gl_unpack_alignmentActiveNo);

                callNoList.insert(potr->second.GL_PIXEL_UNPACK_BUFFERNo);

                callNoList.insert(potr->second.gl_texture_min_filterNo.thisCallNo);
                callNoList.insert(potr->second.gl_texture_min_filterNo.binderCallNo);
                callNoList.insert(potr->second.gl_texture_min_filterNo.activeCallNo);
                callNoList.insert(potr->second.gl_texture_min_filterNo.activeCallNo2);
                potr++;
            }
            //printf glTexSubImage2DNoList
            potr = it->second.glTexSubImage2DNoList.begin();
            while(potr != it->second.glTexSubImage2DNoList.end())
            {
                callNoList.insert(potr->second.thisCallNo);
                callNoList.insert(potr->second.binderCallNo);
                callNoList.insert(potr->second.activeCallNo);
                callNoList.insert(potr->second.activeCallNo2);
                callNoList.insert(potr->second.gl_pack_row_lengthNo);
                callNoList.insert(potr->second.gl_pack_skip_pixelsNo);
                callNoList.insert(potr->second.gl_pack_skip_rowsNo);
                callNoList.insert(potr->second.gl_pack_alignmentNo);
                callNoList.insert(potr->second.gl_unpack_row_lengthNo);
                callNoList.insert(potr->second.gl_unpack_image_heightNo);
                callNoList.insert(potr->second.gl_unpack_skip_pixelsNo);
                callNoList.insert(potr->second.gl_unpack_skip_rowsNo);
                callNoList.insert(potr->second.gl_unpack_skip_imagesNo);
                callNoList.insert(potr->second.gl_unpack_alignmentNo);
                callNoList.insert(potr->second.gl_unpack_alignmentBinderNo);
                callNoList.insert(potr->second.gl_unpack_alignmentActiveNo);

                callNoList.insert(potr->second.GL_PIXEL_UNPACK_BUFFERNo);

                callNoList.insert(potr->second.gl_texture_min_filterNo.thisCallNo);
                callNoList.insert(potr->second.gl_texture_min_filterNo.binderCallNo);
                callNoList.insert(potr->second.gl_texture_min_filterNo.activeCallNo);
                callNoList.insert(potr->second.gl_texture_min_filterNo.activeCallNo2);
                potr++;
            }
            map<unsigned int, unordered_set<unsigned int>>::iterator potr2 = it->second.glTexImage2DGL_PIXEL_UNPACK_BUFFERList.begin();
            while(potr2 != it->second.glTexImage2DGL_PIXEL_UNPACK_BUFFERList.end())
            {
                callNoList.insert(potr2->second.begin(), potr2->second.end());
                callNoList.insert(potr2->second.begin(), potr2->second.end());
                potr2++;
            }
            potr2 = it->second.glTexSubImage2DGL_PIXEL_UNPACK_BUFFERList.begin();
            while(potr2 != it->second.glTexSubImage2DGL_PIXEL_UNPACK_BUFFERList.end())
            {
                callNoList.insert(potr2->second.begin(), potr2->second.end());
                callNoList.insert(potr2->second.begin(), potr2->second.end());
                potr2++;
            }

            list<unsigned int>::iterator potr3 = it->second.glCopyImageSubDataNoList.begin();
            while(potr3 != it->second.glCopyImageSubDataNoList.end())
            {
                callNoList.insert(*potr3);
                potr3++;
            }

            callNoList.insert(it->second.glCopyImageSubData_textureContextNoList.begin(), it->second.glCopyImageSubData_textureContextNoList.end());

    }//for
}
/*
void textureState::readCurTextureState(unordered_set<unsigned int> &callNoList)
{
    callNoList.insert(curActiveTextureNo);
    for(int i=0; i<MAX_ACTIVE_TEXTURE_NUM; i++)
    {
        if(activeTextureNo[i].curTextureNo != 0)
        {
            callNoList.insert(activeTextureNo[i].curTextureNo);
            callNoList.insert(activeTextureNo[i].curActiveTextureNo);
            if(activeTextureNo[i].curTextureIdx == 0)continue;
            map<unsigned int, textureContext>::iterator it = allTextureList.find(activeTextureNo[i].curTextureIdx);
            if(it == allTextureList.end())
            {
                //printf("error 19: bind textures that no exist!!!\n");
                DBG_LOG("error: bind textures that no exist!!!\n");
                continue;
            }
            callNoList.insert(it->second.glGenTexturesNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_sNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_sNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_sNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_sNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_rNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_rNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_rNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_rNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_tNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_tNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_tNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_tNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_mag_filterNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_mag_filterNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_mag_filterNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_mag_filterNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_min_filterNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_min_filterNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_min_filterNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_min_filterNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_max_levelNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_max_levelNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_max_levelNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_max_levelNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_aNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_aNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_aNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_aNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_rNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_rNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_rNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_rNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_gNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_gNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_gNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_gNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_bNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_bNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_bNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_bNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_base_levelNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_base_levelNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_base_levelNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_base_levelNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_modeNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_modeNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_modeNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_modeNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_funcNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_funcNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_funcNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_funcNo.activeCallNo2);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.activeCallNo2);

            callNoList.insert(it->second.glTexParameterNo.gl_texture_min_lodNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_min_lodNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_min_lodNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_min_lodNo.activeCallNo2);

            callNoList.insert(it->second.glTexParameterNo.gl_texture_max_lodNo.thisCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_max_lodNo.binderCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_max_lodNo.activeCallNo);
            callNoList.insert(it->second.glTexParameterNo.gl_texture_max_lodNo.activeCallNo2);
            for(unsigned unit=0; unit<MAX_SAMPLER_NUM; unit++)
            {
            callNoList.insert(it->second.glBindSamplerNo[unit].thisCallNo);
            callNoList.insert(it->second.glBindSamplerNo[unit].binderCallNo);
            callNoList.insert(it->second.glBindSamplerNo[unit].activeCallNo);
            if(it->second.glBindSamplerNo[unit].bindSamplerIdx != 0)
            {
                map<unsigned int, samplerContext>::iterator itsa = allSamplerList.find(it->second.glBindSamplerNo[unit].bindSamplerIdx);
                callNoList.insert(itsa->second.glGenSamplersNo);
                callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_wrap_sNo);
                callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_wrap_rNo);
                callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_wrap_tNo);
                callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_mag_filterNo);
                callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_min_filterNo);
                callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_min_lodNo);
                callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_max_lodNo);
                callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_compare_modeNo);
                callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_compare_funcNo);
            }
            }
            map<unsigned int, unsigned int>::iterator itsa2 = allSamplerBindingList.begin();
            while(itsa2 != allSamplerBindingList.end())
            {
                callNoList.insert(itsa2->second);
                itsa2++;
            }
            callNoList.insert(it->second.glGenerateMipmapNo.thisCallNo);
            callNoList.insert(it->second.glGenerateMipmapNo.binderCallNo);
            callNoList.insert(it->second.glGenerateMipmapNo.activeCallNo);
            callNoList.insert(it->second.glGenerateMipmapNo.nowActiveCallNo);
            map<unsigned int, textureContext::imageWithBinder>::iterator potr = it->second.glTexImage2DNoList.begin();
            while(potr != it->second.glTexImage2DNoList.end())
            {
                callNoList.insert(potr->second.thisCallNo);
                callNoList.insert(potr->second.binderCallNo);
                callNoList.insert(potr->second.activeCallNo);
                callNoList.insert(potr->second.activeCallNo2);
                callNoList.insert(potr->second.gl_pack_row_lengthNo);
                callNoList.insert(potr->second.gl_pack_skip_pixelsNo);
                callNoList.insert(potr->second.gl_pack_skip_rowsNo);
                callNoList.insert(potr->second.gl_pack_alignmentNo);
                callNoList.insert(potr->second.gl_unpack_row_lengthNo);
                callNoList.insert(potr->second.gl_unpack_image_heightNo);
                callNoList.insert(potr->second.gl_unpack_skip_pixelsNo);
                callNoList.insert(potr->second.gl_unpack_skip_rowsNo);
                callNoList.insert(potr->second.gl_unpack_skip_imagesNo);
                callNoList.insert(potr->second.gl_unpack_alignmentNo);
                callNoList.insert(potr->second.gl_unpack_alignmentBinderNo);
                callNoList.insert(potr->second.gl_unpack_alignmentActiveNo);

                callNoList.insert(potr->second.GL_PIXEL_UNPACK_BUFFERNo);

                callNoList.insert(potr->second.gl_texture_min_filterNo.thisCallNo);
                callNoList.insert(potr->second.gl_texture_min_filterNo.binderCallNo);
                callNoList.insert(potr->second.gl_texture_min_filterNo.activeCallNo);
                callNoList.insert(potr->second.gl_texture_min_filterNo.activeCallNo2);
                potr++;
            }
            //printf glTexSubImage2DNoList
            potr = it->second.glTexSubImage2DNoList.begin();
            while(potr != it->second.glTexSubImage2DNoList.end())
            {
                callNoList.insert(potr->second.thisCallNo);
                callNoList.insert(potr->second.binderCallNo);
                callNoList.insert(potr->second.activeCallNo);
                callNoList.insert(potr->second.activeCallNo2);
                callNoList.insert(potr->second.gl_pack_row_lengthNo);
                callNoList.insert(potr->second.gl_pack_skip_pixelsNo);
                callNoList.insert(potr->second.gl_pack_skip_rowsNo);
                callNoList.insert(potr->second.gl_pack_alignmentNo);
                callNoList.insert(potr->second.gl_unpack_row_lengthNo);
                callNoList.insert(potr->second.gl_unpack_image_heightNo);
                callNoList.insert(potr->second.gl_unpack_skip_pixelsNo);
                callNoList.insert(potr->second.gl_unpack_skip_rowsNo);
                callNoList.insert(potr->second.gl_unpack_skip_imagesNo);
                callNoList.insert(potr->second.gl_unpack_alignmentNo);
                callNoList.insert(potr->second.gl_unpack_alignmentBinderNo);
                callNoList.insert(potr->second.gl_unpack_alignmentActiveNo);

                callNoList.insert(potr->second.GL_PIXEL_UNPACK_BUFFERNo);

                callNoList.insert(potr->second.gl_texture_min_filterNo.thisCallNo);
                callNoList.insert(potr->second.gl_texture_min_filterNo.binderCallNo);
                callNoList.insert(potr->second.gl_texture_min_filterNo.activeCallNo);
                callNoList.insert(potr->second.gl_texture_min_filterNo.activeCallNo2);
                potr++;
            }
            map<unsigned int, unordered_set<unsigned int>>::iterator potr2 = it->second.glTexImage2DGL_PIXEL_UNPACK_BUFFERList.begin();
            while(potr2 != it->second.glTexImage2DGL_PIXEL_UNPACK_BUFFERList.end())
            {
                callNoList.insert(potr2->second.begin(), potr2->second.end());
                callNoList.insert(potr2->second.begin(), potr2->second.end());
                potr2++;
            }
            potr2 = it->second.glTexSubImage2DGL_PIXEL_UNPACK_BUFFERList.begin();
            while(potr2 != it->second.glTexSubImage2DGL_PIXEL_UNPACK_BUFFERList.end())
            {
                callNoList.insert(potr2->second.begin(), potr2->second.end());
                callNoList.insert(potr2->second.begin(), potr2->second.end());
                potr2++;
            }
        }//if
    }//for
}
*/

void textureState::readCurTextureState(unordered_set<unsigned int> &callNoList, bool * callNoListJudge)
{
    if(curActiveTextureNo != 0 && callNoListJudge[curActiveTextureNo] == false)
    {
        callNoList.insert(curActiveTextureNo);
        callNoListJudge[curActiveTextureNo] = true;
    }
    for(unsigned int i=0; i<MAX_ACTIVE_TEXTURE_NUM; i++)
    {
        if(activeTextureNo[i].curTextureNo != 0)
        {
            if(activeTextureNo[i].curTextureNo != 0 && callNoListJudge[activeTextureNo[i].curTextureNo] == false)
            {
                callNoList.insert(activeTextureNo[i].curTextureNo);
                callNoListJudge[activeTextureNo[i].curTextureNo] = true;
            }
            if(activeTextureNo[i].curActiveTextureNo != 0 && callNoListJudge[activeTextureNo[i].curActiveTextureNo] == false)
            {
                callNoList.insert(activeTextureNo[i].curActiveTextureNo);
                callNoListJudge[activeTextureNo[i].curActiveTextureNo] = true;
            }
            if(activeTextureNo[i].curTextureIdx == 0)continue;
            map<unsigned int, textureContext>::iterator it = allTextureList.find(activeTextureNo[i].curTextureIdx);
            if(it == allTextureList.end())
            {
                //printf("error 19: bind textures that no exist!!!\n");
                DBG_LOG("error: bind textures that no exist!!!\n");
                continue;
            }
            if(it->second.glGenTexturesNo != 0 && callNoListJudge[it->second.glGenTexturesNo] == false)
            {
                callNoList.insert(it->second.glGenTexturesNo);
                callNoListJudge[it->second.glGenTexturesNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_wrap_sNo.thisCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_sNo.thisCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_sNo.thisCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_sNo.thisCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_wrap_sNo.binderCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_sNo.binderCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_sNo.binderCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_sNo.binderCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_wrap_sNo.activeCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_sNo.activeCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_sNo.activeCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_sNo.activeCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_wrap_sNo.activeCallNo2 != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_sNo.activeCallNo2] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_sNo.activeCallNo2);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_sNo.activeCallNo2] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_wrap_rNo.thisCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_rNo.thisCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_rNo.thisCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_rNo.thisCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_wrap_rNo.binderCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_rNo.binderCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_rNo.binderCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_rNo.binderCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_wrap_rNo.activeCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_rNo.activeCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_rNo.activeCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_rNo.activeCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_wrap_rNo.activeCallNo2 != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_rNo.activeCallNo2] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_rNo.activeCallNo2);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_rNo.activeCallNo2] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_wrap_tNo.thisCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_tNo.thisCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_tNo.thisCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_tNo.thisCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_wrap_tNo.binderCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_tNo.binderCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_tNo.binderCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_tNo.binderCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_wrap_tNo.activeCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_tNo.activeCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_tNo.activeCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_tNo.activeCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_wrap_tNo.activeCallNo2 != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_tNo.activeCallNo2] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_wrap_tNo.activeCallNo2);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_wrap_tNo.activeCallNo2] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_mag_filterNo.thisCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_mag_filterNo.thisCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_mag_filterNo.thisCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_mag_filterNo.thisCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_mag_filterNo.binderCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_mag_filterNo.binderCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_mag_filterNo.binderCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_mag_filterNo.binderCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_mag_filterNo.activeCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_mag_filterNo.activeCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_mag_filterNo.activeCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_mag_filterNo.activeCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_mag_filterNo.activeCallNo2 != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_mag_filterNo.activeCallNo2] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_mag_filterNo.activeCallNo2);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_mag_filterNo.activeCallNo2] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_min_filterNo.thisCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_min_filterNo.thisCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_min_filterNo.thisCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_min_filterNo.thisCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_min_filterNo.binderCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_min_filterNo.binderCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_min_filterNo.binderCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_min_filterNo.binderCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_min_filterNo.activeCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_min_filterNo.activeCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_min_filterNo.activeCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_min_filterNo.activeCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_min_filterNo.activeCallNo2 != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_min_filterNo.activeCallNo2] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_min_filterNo.activeCallNo2);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_min_filterNo.activeCallNo2] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_max_levelNo.thisCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_max_levelNo.thisCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_max_levelNo.thisCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_max_levelNo.thisCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_max_levelNo.binderCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_max_levelNo.binderCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_max_levelNo.binderCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_max_levelNo.binderCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_max_levelNo.activeCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_max_levelNo.activeCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_max_levelNo.activeCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_max_levelNo.activeCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_max_levelNo.activeCallNo2 != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_max_levelNo.activeCallNo2] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_max_levelNo.activeCallNo2);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_max_levelNo.activeCallNo2] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_swizzle_aNo.thisCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_aNo.thisCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_aNo.thisCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_aNo.thisCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_swizzle_aNo.binderCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_aNo.binderCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_aNo.binderCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_aNo.binderCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_swizzle_aNo.activeCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_aNo.activeCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_aNo.activeCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_aNo.activeCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_swizzle_aNo.activeCallNo2 != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_aNo.activeCallNo2] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_aNo.activeCallNo2);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_aNo.activeCallNo2] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_swizzle_rNo.thisCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_rNo.thisCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_rNo.thisCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_rNo.thisCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_swizzle_rNo.binderCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_rNo.binderCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_rNo.binderCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_rNo.binderCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_swizzle_rNo.activeCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_rNo.activeCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_rNo.activeCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_rNo.activeCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_swizzle_rNo.activeCallNo2 != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_rNo.activeCallNo2] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_rNo.activeCallNo2);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_rNo.activeCallNo2] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_swizzle_gNo.thisCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_gNo.thisCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_gNo.thisCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_gNo.thisCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_swizzle_gNo.binderCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_gNo.binderCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_gNo.binderCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_gNo.binderCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_swizzle_gNo.activeCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_gNo.activeCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_gNo.activeCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_gNo.activeCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_swizzle_gNo.activeCallNo2 != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_gNo.activeCallNo2] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_gNo.activeCallNo2);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_gNo.activeCallNo2] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_swizzle_bNo.thisCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_bNo.thisCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_bNo.thisCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_bNo.thisCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_swizzle_bNo.binderCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_bNo.binderCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_bNo.binderCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_bNo.binderCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_swizzle_bNo.activeCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_bNo.activeCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_bNo.activeCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_bNo.activeCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_swizzle_bNo.activeCallNo2 != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_bNo.activeCallNo2] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_swizzle_bNo.activeCallNo2);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_swizzle_bNo.activeCallNo2] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_base_levelNo.thisCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_base_levelNo.thisCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_base_levelNo.thisCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_base_levelNo.thisCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_base_levelNo.binderCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_base_levelNo.binderCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_base_levelNo.binderCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_base_levelNo.binderCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_base_levelNo.activeCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_base_levelNo.activeCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_base_levelNo.activeCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_base_levelNo.activeCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_base_levelNo.activeCallNo2 != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_base_levelNo.activeCallNo2] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_base_levelNo.activeCallNo2);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_base_levelNo.activeCallNo2] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_compare_modeNo.thisCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_compare_modeNo.thisCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_modeNo.thisCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_compare_modeNo.thisCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_compare_modeNo.binderCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_compare_modeNo.binderCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_modeNo.binderCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_compare_modeNo.binderCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_compare_modeNo.activeCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_compare_modeNo.activeCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_modeNo.activeCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_compare_modeNo.activeCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_compare_modeNo.activeCallNo2 != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_compare_modeNo.activeCallNo2] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_modeNo.activeCallNo2);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_compare_modeNo.activeCallNo2] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_compare_funcNo.thisCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_compare_funcNo.thisCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_funcNo.thisCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_compare_funcNo.thisCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_compare_funcNo.binderCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_compare_funcNo.binderCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_funcNo.binderCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_compare_funcNo.binderCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_compare_funcNo.activeCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_compare_funcNo.activeCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_funcNo.activeCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_compare_funcNo.activeCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_compare_funcNo.activeCallNo2 != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_compare_funcNo.activeCallNo2] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_compare_funcNo.activeCallNo2);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_compare_funcNo.activeCallNo2] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.thisCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.thisCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.thisCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.thisCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.binderCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.binderCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.binderCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.binderCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.activeCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.activeCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.activeCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.activeCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.activeCallNo2 != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.activeCallNo2] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.activeCallNo2);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_srgs_decode_extNo.activeCallNo2] = true;
            }

            if(it->second.glTexParameterNo.gl_texture_min_lodNo.thisCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_min_lodNo.thisCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_min_lodNo.thisCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_min_lodNo.thisCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_min_lodNo.binderCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_min_lodNo.binderCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_min_lodNo.binderCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_min_lodNo.binderCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_min_lodNo.activeCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_min_lodNo.activeCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_min_lodNo.activeCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_min_lodNo.activeCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_min_lodNo.activeCallNo2 != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_min_lodNo.activeCallNo2] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_min_lodNo.activeCallNo2);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_min_lodNo.activeCallNo2] = true;
            }

            if(it->second.glTexParameterNo.gl_texture_max_lodNo.thisCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_max_lodNo.thisCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_max_lodNo.thisCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_max_lodNo.thisCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_max_lodNo.binderCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_max_lodNo.binderCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_max_lodNo.binderCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_max_lodNo.binderCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_max_lodNo.activeCallNo != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_max_lodNo.activeCallNo] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_max_lodNo.activeCallNo);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_max_lodNo.activeCallNo] = true;
            }
            if(it->second.glTexParameterNo.gl_texture_max_lodNo.activeCallNo2 != 0 && callNoListJudge[it->second.glTexParameterNo.gl_texture_max_lodNo.activeCallNo2] == false)
            {
                callNoList.insert(it->second.glTexParameterNo.gl_texture_max_lodNo.activeCallNo2);
                callNoListJudge[it->second.glTexParameterNo.gl_texture_max_lodNo.activeCallNo2] = true;
            }
            for(unsigned unit=0; unit<MAX_SAMPLER_NUM; unit++)
            {
                if(it->second.glBindSamplerNo[unit].thisCallNo != 0 && callNoListJudge[it->second.glBindSamplerNo[unit].thisCallNo] == false)
                {
                    callNoList.insert(it->second.glBindSamplerNo[unit].thisCallNo);
                    callNoListJudge[it->second.glBindSamplerNo[unit].thisCallNo] = true;
                }
                if(it->second.glBindSamplerNo[unit].binderCallNo != 0 && callNoListJudge[it->second.glBindSamplerNo[unit].binderCallNo] == false)
                {
                    callNoList.insert(it->second.glBindSamplerNo[unit].binderCallNo);
                    callNoListJudge[it->second.glBindSamplerNo[unit].binderCallNo] = true;
                }
                if(it->second.glBindSamplerNo[unit].activeCallNo != 0 && callNoListJudge[it->second.glBindSamplerNo[unit].activeCallNo] == false)
                {
                    callNoList.insert(it->second.glBindSamplerNo[unit].activeCallNo);
                    callNoListJudge[it->second.glBindSamplerNo[unit].activeCallNo] = true;
                }
                if(it->second.glBindSamplerNo[unit].bindSamplerIdx != 0)
                {
                    map<unsigned int, samplerContext>::iterator itsa = allSamplerList.find(it->second.glBindSamplerNo[unit].bindSamplerIdx);
                    if(itsa->second.glGenSamplersNo != 0 && callNoListJudge[itsa->second.glGenSamplersNo] == false)
                    {
                        callNoList.insert(itsa->second.glGenSamplersNo);
                        callNoListJudge[itsa->second.glGenSamplersNo] = true;
                    }
                    if(itsa->second.glSamplerParameterNo.gl_texture_wrap_sNo != 0 && callNoListJudge[itsa->second.glSamplerParameterNo.gl_texture_wrap_sNo] == false)
                    {
                        callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_wrap_sNo);
                        callNoListJudge[itsa->second.glSamplerParameterNo.gl_texture_wrap_sNo] = true;
                    }
                    if(itsa->second.glSamplerParameterNo.gl_texture_wrap_rNo != 0 && callNoListJudge[itsa->second.glSamplerParameterNo.gl_texture_wrap_rNo] == false)
                    {
                        callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_wrap_rNo);
                        callNoListJudge[itsa->second.glSamplerParameterNo.gl_texture_wrap_rNo] = true;
                    }
                    if(itsa->second.glSamplerParameterNo.gl_texture_wrap_tNo != 0 && callNoListJudge[itsa->second.glSamplerParameterNo.gl_texture_wrap_tNo] == false)
                    {
                        callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_wrap_tNo);
                        callNoListJudge[itsa->second.glSamplerParameterNo.gl_texture_wrap_tNo] = true;
                    }
                    if(itsa->second.glSamplerParameterNo.gl_texture_mag_filterNo != 0 && callNoListJudge[itsa->second.glSamplerParameterNo.gl_texture_mag_filterNo] == false)
                    {
                        callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_mag_filterNo);
                        callNoListJudge[itsa->second.glSamplerParameterNo.gl_texture_mag_filterNo] = true;
                    }
                    if(itsa->second.glSamplerParameterNo.gl_texture_min_filterNo != 0 && callNoListJudge[itsa->second.glSamplerParameterNo.gl_texture_min_filterNo] == false)
                    {
                        callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_min_filterNo);
                        callNoListJudge[itsa->second.glSamplerParameterNo.gl_texture_min_filterNo] = true;
                    }
                    if(itsa->second.glSamplerParameterNo.gl_texture_min_lodNo != 0 && callNoListJudge[itsa->second.glSamplerParameterNo.gl_texture_min_lodNo] == false)
                    {
                        callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_min_lodNo);
                        callNoListJudge[itsa->second.glSamplerParameterNo.gl_texture_min_lodNo] = true;
                    }
                    if(itsa->second.glSamplerParameterNo.gl_texture_max_lodNo != 0 && callNoListJudge[itsa->second.glSamplerParameterNo.gl_texture_max_lodNo] == false)
                    {
                        callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_max_lodNo);
                        callNoListJudge[itsa->second.glSamplerParameterNo.gl_texture_max_lodNo] = true;
                    }
                    if(itsa->second.glSamplerParameterNo.gl_texture_compare_modeNo != 0 && callNoListJudge[itsa->second.glSamplerParameterNo.gl_texture_compare_modeNo] == false)
                    {
                        callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_compare_modeNo);
                        callNoListJudge[itsa->second.glSamplerParameterNo.gl_texture_compare_modeNo] = true;
                    }
                    if(itsa->second.glSamplerParameterNo.gl_texture_compare_funcNo != 0 && callNoListJudge[itsa->second.glSamplerParameterNo.gl_texture_compare_funcNo] == false)
                    {
                        callNoList.insert(itsa->second.glSamplerParameterNo.gl_texture_compare_funcNo);
                        callNoListJudge[itsa->second.glSamplerParameterNo.gl_texture_compare_funcNo] = true;
                    }
                }
            }
            map<unsigned int, unsigned int>::iterator itsa2 = allSamplerBindingList.begin();
            while(itsa2 != allSamplerBindingList.end())
            {
                if(itsa2->second != 0 && callNoListJudge[itsa2->second] == false)
                {
                    callNoList.insert(itsa2->second);
                    callNoListJudge[itsa2->second] = true;
                }
                itsa2++;
            }
            if(it->second.glGenerateMipmapNo.thisCallNo != 0 && callNoListJudge[it->second.glGenerateMipmapNo.thisCallNo] == false)
            {
                callNoList.insert(it->second.glGenerateMipmapNo.thisCallNo);
                callNoListJudge[it->second.glGenerateMipmapNo.thisCallNo] = true;
            }
            if(it->second.glGenerateMipmapNo.binderCallNo != 0 && callNoListJudge[it->second.glGenerateMipmapNo.binderCallNo] == false)
            {
                callNoList.insert(it->second.glGenerateMipmapNo.binderCallNo);
                callNoListJudge[it->second.glGenerateMipmapNo.binderCallNo] = true;
            }
            if(it->second.glGenerateMipmapNo.activeCallNo != 0 && callNoListJudge[it->second.glGenerateMipmapNo.activeCallNo] == false)
            {
                callNoList.insert(it->second.glGenerateMipmapNo.activeCallNo);
                callNoListJudge[it->second.glGenerateMipmapNo.activeCallNo] = true;
            }
            if(it->second.glGenerateMipmapNo.nowActiveCallNo != 0 && callNoListJudge[it->second.glGenerateMipmapNo.nowActiveCallNo] == false)
            {
                callNoList.insert(it->second.glGenerateMipmapNo.nowActiveCallNo);
                callNoListJudge[it->second.glGenerateMipmapNo.nowActiveCallNo] = true;
            }
            map<unsigned int, textureContext::imageWithBinder>::iterator potr = it->second.glTexImage2DNoList.begin();
            while(potr != it->second.glTexImage2DNoList.end())
            {
                if(potr->second.thisCallNo != 0 && callNoListJudge[potr->second.thisCallNo] == false)
                {
                    callNoList.insert(potr->second.thisCallNo);
                    callNoListJudge[potr->second.thisCallNo] = true;
                }
                if(potr->second.binderCallNo != 0 && callNoListJudge[potr->second.binderCallNo] == false)
                {
                    callNoList.insert(potr->second.binderCallNo);
                    callNoListJudge[potr->second.binderCallNo] = true;
                }
                if(potr->second.activeCallNo != 0 && callNoListJudge[potr->second.activeCallNo] == false)
                {
                    callNoList.insert(potr->second.activeCallNo);
                    callNoListJudge[potr->second.activeCallNo] = true;
                }
                if(potr->second.activeCallNo2 != 0 && callNoListJudge[potr->second.activeCallNo2] == false)
                {
                    callNoList.insert(potr->second.activeCallNo2);
                    callNoListJudge[potr->second.activeCallNo2] = true;
                }
                if(potr->second.gl_pack_row_lengthNo != 0 && callNoListJudge[potr->second.gl_pack_row_lengthNo] == false)
                {
                    callNoList.insert(potr->second.gl_pack_row_lengthNo);
                    callNoListJudge[potr->second.gl_pack_row_lengthNo] = true;
                }
                if(potr->second.gl_pack_skip_pixelsNo != 0 && callNoListJudge[potr->second.gl_pack_skip_pixelsNo] == false)
                {
                    callNoList.insert(potr->second.gl_pack_skip_pixelsNo);
                    callNoListJudge[potr->second.gl_pack_skip_pixelsNo] = true;
                }
                if(potr->second.gl_pack_skip_rowsNo != 0 && callNoListJudge[potr->second.gl_pack_skip_rowsNo] == false)
                {
                    callNoList.insert(potr->second.gl_pack_skip_rowsNo);
                    callNoListJudge[potr->second.gl_pack_skip_rowsNo] = true;
                }
                if(potr->second.gl_pack_alignmentNo != 0 && callNoListJudge[potr->second.gl_pack_alignmentNo] == false)
                {
                    callNoList.insert(potr->second.gl_pack_alignmentNo);
                    callNoListJudge[potr->second.gl_pack_alignmentNo] = true;
                }
                if(potr->second.gl_unpack_row_lengthNo != 0 && callNoListJudge[potr->second.gl_unpack_row_lengthNo] == false)
                {
                    callNoList.insert(potr->second.gl_unpack_row_lengthNo);
                    callNoListJudge[potr->second.gl_unpack_row_lengthNo] = true;
                }
                if(potr->second.gl_unpack_image_heightNo != 0 && callNoListJudge[potr->second.gl_unpack_image_heightNo] == false)
                {
                    callNoList.insert(potr->second.gl_unpack_image_heightNo);
                    callNoListJudge[potr->second.gl_unpack_image_heightNo] = true;
                }
                if(potr->second.gl_unpack_skip_pixelsNo != 0 && callNoListJudge[potr->second.gl_unpack_skip_pixelsNo] == false)
                {
                    callNoList.insert(potr->second.gl_unpack_skip_pixelsNo);
                    callNoListJudge[potr->second.gl_unpack_skip_pixelsNo] = true;
                }
                if(potr->second.gl_unpack_skip_rowsNo != 0 && callNoListJudge[potr->second.gl_unpack_skip_rowsNo] == false)
                {
                    callNoList.insert(potr->second.gl_unpack_skip_rowsNo);
                    callNoListJudge[potr->second.gl_unpack_skip_rowsNo] = true;
                }
                if(potr->second.gl_unpack_skip_imagesNo != 0 && callNoListJudge[potr->second.gl_unpack_skip_imagesNo] == false)
                {
                    callNoList.insert(potr->second.gl_unpack_skip_imagesNo);
                    callNoListJudge[potr->second.gl_unpack_skip_imagesNo] = true;
                }
                if(potr->second.gl_unpack_alignmentNo != 0 && callNoListJudge[potr->second.gl_unpack_alignmentNo] == false)
                {
                    callNoList.insert(potr->second.gl_unpack_alignmentNo);
                    callNoListJudge[potr->second.gl_unpack_alignmentNo] = true;
                }
                if(potr->second.gl_unpack_alignmentBinderNo != 0 && callNoListJudge[potr->second.gl_unpack_alignmentBinderNo] == false)
                {
                    callNoList.insert(potr->second.gl_unpack_alignmentBinderNo);
                    callNoListJudge[potr->second.gl_unpack_alignmentBinderNo] = true;
                }
                if(potr->second.gl_unpack_alignmentActiveNo != 0 && callNoListJudge[potr->second.gl_unpack_alignmentActiveNo] == false)
                {
                    callNoList.insert(potr->second.gl_unpack_alignmentActiveNo);
                    callNoListJudge[potr->second.gl_unpack_alignmentActiveNo] = true;
                }

                if(potr->second.GL_PIXEL_UNPACK_BUFFERNo != 0 && callNoListJudge[potr->second.GL_PIXEL_UNPACK_BUFFERNo] == false)
                {
                    callNoList.insert(potr->second.GL_PIXEL_UNPACK_BUFFERNo);
                    callNoListJudge[potr->second.GL_PIXEL_UNPACK_BUFFERNo] = true;
                }

                if(potr->second.gl_texture_min_filterNo.thisCallNo != 0 && callNoListJudge[potr->second.gl_texture_min_filterNo.thisCallNo] == false)
                {
                    callNoList.insert(potr->second.gl_texture_min_filterNo.thisCallNo);
                    callNoListJudge[potr->second.gl_texture_min_filterNo.thisCallNo] = true;
                }
                if(potr->second.gl_texture_min_filterNo.binderCallNo != 0 && callNoListJudge[potr->second.gl_texture_min_filterNo.binderCallNo] == false)
                {
                    callNoList.insert(potr->second.gl_texture_min_filterNo.binderCallNo);
                    callNoListJudge[potr->second.gl_texture_min_filterNo.binderCallNo] = true;
                }
                if(potr->second.gl_texture_min_filterNo.activeCallNo != 0 && callNoListJudge[potr->second.gl_texture_min_filterNo.activeCallNo] == false)
                {
                    callNoList.insert(potr->second.gl_texture_min_filterNo.activeCallNo);
                    callNoListJudge[potr->second.gl_texture_min_filterNo.activeCallNo] = true;
                }
                if(potr->second.gl_texture_min_filterNo.activeCallNo2 != 0 && callNoListJudge[potr->second.gl_texture_min_filterNo.activeCallNo2] == false)
                {
                    callNoList.insert(potr->second.gl_texture_min_filterNo.activeCallNo2);
                    callNoListJudge[potr->second.gl_texture_min_filterNo.activeCallNo2] = true;
                }
                potr++;
            }
            //printf glTexSubImage2DNoList
            potr = it->second.glTexSubImage2DNoList.begin();
            while(potr != it->second.glTexSubImage2DNoList.end())
            {
                if(potr->second.thisCallNo != 0 && callNoListJudge[potr->second.thisCallNo] == false)
                {
                    callNoList.insert(potr->second.thisCallNo);
                    callNoListJudge[potr->second.thisCallNo] = true;
                }
                if(potr->second.binderCallNo != 0 && callNoListJudge[potr->second.binderCallNo] == false)
                {
                    callNoList.insert(potr->second.binderCallNo);
                    callNoListJudge[potr->second.binderCallNo] = true;
                }
                if(potr->second.activeCallNo != 0 && callNoListJudge[potr->second.activeCallNo] == false)
                {
                    callNoList.insert(potr->second.activeCallNo);
                    callNoListJudge[potr->second.activeCallNo] = true;
                }
                if(potr->second.activeCallNo2 != 0 && callNoListJudge[potr->second.activeCallNo2] == false)
                {
                    callNoList.insert(potr->second.activeCallNo2);
                    callNoListJudge[potr->second.activeCallNo2] = true;
                }
                if(potr->second.gl_pack_row_lengthNo != 0 && callNoListJudge[potr->second.gl_pack_row_lengthNo] == false)
                {
                    callNoList.insert(potr->second.gl_pack_row_lengthNo);
                    callNoListJudge[potr->second.gl_pack_row_lengthNo] = true;
                }
                if(potr->second.gl_pack_skip_pixelsNo != 0 && callNoListJudge[potr->second.gl_pack_skip_pixelsNo] == false)
                {
                    callNoList.insert(potr->second.gl_pack_skip_pixelsNo);
                    callNoListJudge[potr->second.gl_pack_skip_pixelsNo] = true;
                }
                if(potr->second.gl_pack_skip_rowsNo != 0 && callNoListJudge[potr->second.gl_pack_skip_rowsNo] == false)
                {
                    callNoList.insert(potr->second.gl_pack_skip_rowsNo);
                    callNoListJudge[potr->second.gl_pack_skip_rowsNo] = true;
                }
                if(potr->second.gl_pack_alignmentNo != 0 && callNoListJudge[potr->second.gl_pack_alignmentNo] == false)
                {
                    callNoList.insert(potr->second.gl_pack_alignmentNo);
                    callNoListJudge[potr->second.gl_pack_alignmentNo] = true;
                }
                if(potr->second.gl_unpack_row_lengthNo != 0 && callNoListJudge[potr->second.gl_unpack_row_lengthNo] == false)
                {
                    callNoList.insert(potr->second.gl_unpack_row_lengthNo);
                    callNoListJudge[potr->second.gl_unpack_row_lengthNo] = true;
                }
                if(potr->second.gl_unpack_image_heightNo != 0 && callNoListJudge[potr->second.gl_unpack_image_heightNo] == false)
                {
                    callNoList.insert(potr->second.gl_unpack_image_heightNo);
                    callNoListJudge[potr->second.gl_unpack_image_heightNo] = true;
                }
                if(potr->second.gl_unpack_skip_pixelsNo != 0 && callNoListJudge[potr->second.gl_unpack_skip_pixelsNo] == false)
                {
                    callNoList.insert(potr->second.gl_unpack_skip_pixelsNo);
                    callNoListJudge[potr->second.gl_unpack_skip_pixelsNo] = true;
                }
                if(potr->second.gl_unpack_skip_rowsNo != 0 && callNoListJudge[potr->second.gl_unpack_skip_rowsNo] == false)
                {
                    callNoList.insert(potr->second.gl_unpack_skip_rowsNo);
                    callNoListJudge[potr->second.gl_unpack_skip_rowsNo] = true;
                }
                if(potr->second.gl_unpack_skip_imagesNo != 0 && callNoListJudge[potr->second.gl_unpack_skip_imagesNo] == false)
                {
                    callNoList.insert(potr->second.gl_unpack_skip_imagesNo);
                    callNoListJudge[potr->second.gl_unpack_skip_imagesNo] = true;
                }
                if(potr->second.gl_unpack_alignmentNo != 0 && callNoListJudge[potr->second.gl_unpack_alignmentNo] == false)
                {
                    callNoList.insert(potr->second.gl_unpack_alignmentNo);
                    callNoListJudge[potr->second.gl_unpack_alignmentNo] = true;
                }
                if(potr->second.gl_unpack_alignmentBinderNo != 0 && callNoListJudge[potr->second.gl_unpack_alignmentBinderNo] == false)
                {
                    callNoList.insert(potr->second.gl_unpack_alignmentBinderNo);
                    callNoListJudge[potr->second.gl_unpack_alignmentBinderNo] = true;
                }
                if(potr->second.gl_unpack_alignmentActiveNo != 0 && callNoListJudge[potr->second.gl_unpack_alignmentActiveNo] == false)
                {
                    callNoList.insert(potr->second.gl_unpack_alignmentActiveNo);
                    callNoListJudge[potr->second.gl_unpack_alignmentActiveNo] = true;
                }

                if(potr->second.GL_PIXEL_UNPACK_BUFFERNo != 0 && callNoListJudge[potr->second.GL_PIXEL_UNPACK_BUFFERNo] == false)
                {
                    callNoList.insert(potr->second.GL_PIXEL_UNPACK_BUFFERNo);
                    callNoListJudge[potr->second.GL_PIXEL_UNPACK_BUFFERNo] = true;
                }

                if(potr->second.gl_texture_min_filterNo.thisCallNo != 0 && callNoListJudge[potr->second.gl_texture_min_filterNo.thisCallNo] == false)
                {
                    callNoList.insert(potr->second.gl_texture_min_filterNo.thisCallNo);
                    callNoListJudge[potr->second.gl_texture_min_filterNo.thisCallNo] = true;
                }
                if(potr->second.gl_texture_min_filterNo.binderCallNo != 0 && callNoListJudge[potr->second.gl_texture_min_filterNo.binderCallNo] == false)
                {
                    callNoList.insert(potr->second.gl_texture_min_filterNo.binderCallNo);
                    callNoListJudge[potr->second.gl_texture_min_filterNo.binderCallNo] = true;
                }
                if(potr->second.gl_texture_min_filterNo.activeCallNo != 0 && callNoListJudge[potr->second.gl_texture_min_filterNo.activeCallNo] == false)
                {
                    callNoList.insert(potr->second.gl_texture_min_filterNo.activeCallNo);
                    callNoListJudge[potr->second.gl_texture_min_filterNo.activeCallNo] = true;
                }
                if(potr->second.gl_texture_min_filterNo.activeCallNo2 != 0 && callNoListJudge[potr->second.gl_texture_min_filterNo.activeCallNo2] == false)
                {
                    callNoList.insert(potr->second.gl_texture_min_filterNo.activeCallNo2);
                    callNoListJudge[potr->second.gl_texture_min_filterNo.activeCallNo2] = true;
                }
                potr++;
            }
            map<unsigned int, unordered_set<unsigned int>>::iterator potr2 = it->second.glTexImage2DGL_PIXEL_UNPACK_BUFFERList.begin();
            while(potr2 != it->second.glTexImage2DGL_PIXEL_UNPACK_BUFFERList.end())
            {
                callNoList.insert(potr2->second.begin(), potr2->second.end());
                //callNoList.insert(potr2->second.begin(), potr2->second.end());
                potr2++;
            }
            potr2 = it->second.glTexSubImage2DGL_PIXEL_UNPACK_BUFFERList.begin();
            while(potr2 != it->second.glTexSubImage2DGL_PIXEL_UNPACK_BUFFERList.end())
            {
                callNoList.insert(potr2->second.begin(), potr2->second.end());
                //callNoList.insert(potr2->second.begin(), potr2->second.end());
                potr2++;
            }

            list<unsigned int>::iterator potr3 = it->second.glCopyImageSubDataNoList.begin();
            while(potr3 != it->second.glCopyImageSubDataNoList.end())
            {
                callNoList.insert(*potr3);
                potr3++;
            }

            callNoList.insert(it->second.glCopyImageSubData_textureContextNoList.begin(), it->second.glCopyImageSubData_textureContextNoList.end());

        }//if
    }//for
}

void textureState::readCurTextureIdx(vector<unsigned int> &textureList)
{
    for(unsigned int i=0; i<MAX_ACTIVE_TEXTURE_NUM; i++)
    {
        if(activeTextureNo[i].curTextureNo != 0)
        {
            if(activeTextureNo[i].curTextureIdx != 0)textureList.push_back(activeTextureNo[i].curTextureIdx);
        }
    }//for
}

unsigned int textureState::readCurTextureNIdx(unsigned int n)
{
    return activeTextureNo[n].curTextureIdx;
}

void textureState::saveContext()
{

    share_context_curActiveTextureIdx = curActiveTextureIdx;
    share_context_curActiveTextureNo = curActiveTextureNo;

    for(unsigned int i=0; i<MAX_ACTIVE_TEXTURE_NUM; i++)
    {
        share_context_activeTextureNo[i] = activeTextureNo[i];
    }

}

void textureState::readContext(int ctx)
{
    if(share_ctx !=ctx)
    {
        //exchange the context
        activeTextureState change_activeTextureNo = {0, 0, 0, 0};
        unsigned int change_curActiveTextureIdx = curActiveTextureIdx;
        unsigned int change_curActiveTextureNo = curActiveTextureNo;

        curActiveTextureIdx = share_context_curActiveTextureIdx;
        curActiveTextureNo = share_context_curActiveTextureNo;

        share_context_curActiveTextureIdx = change_curActiveTextureIdx;
        share_context_curActiveTextureNo = change_curActiveTextureNo;

        for(unsigned int i=0; i<MAX_ACTIVE_TEXTURE_NUM; i++)
        {
            change_activeTextureNo = activeTextureNo[i];
            activeTextureNo[i] = share_context_activeTextureNo[i];
            share_context_activeTextureNo[i] = change_activeTextureNo;
        }
    share_ctx = ctx;
    }

}


void textureState::clear()
{
    share_ctx = 0;
    curActiveTextureIdx = 0;
    curActiveTextureNo = 0;
    share_context_curActiveTextureIdx = 0;
    share_context_curActiveTextureNo = 0;
    lastActiveTextureIdx = 0;
    lastActiveTextureNo = 0;
    //the least is 64, the nvidia is 192
    //64-128 is cube map
    for(unsigned int i=0; i<MAX_ACTIVE_TEXTURE_NUM; i++)
    {
        activeTextureNo[i] = {0, 0, 0, 0};
    }
    //    activeTextureState activeCubeTextureNo[64];
    allTextureList.clear();
    allSamplerList.clear();
    allSamplerBindingList.clear();
    globalGlPixelStoreiNo = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
}

void bufferState::readCurDeleteBuffers(unordered_set<unsigned int> &callNoList)
{
    map<unsigned int, vector<unsigned>>::iterator it = GlobalDeleteIndexList.begin();
    while(it!=GlobalDeleteIndexList.end())
    {
        for(unsigned int i=0; i<it->second.size(); i++)
        {
            map<unsigned int, vector<unsigned>>::iterator potr = GlobalGenIndexList.find(it->first);
            if(potr==GlobalGenIndexList.end())
            {
                //printf("error 20: not fastforward.cpp 1100\n");
                DBG_LOG("error: not fastforward.cpp 1100\n");
                exit(1);
            }
            for(unsigned int j=0; j<potr->second.size(); j++)
            {
                unordered_set<unsigned int>::iterator judge = callNoList.find(potr->second[j]);
                if(potr->second[j] < it->second[i] && judge != callNoList.end())
                {
                    printf("it->first  %d\n", it->first);
                    //callNoList.insert(it->second[i]);
                    //continue;
                }
            }
        }//for
        it++;
    }//while
}

void bufferState::ff_glDeleteBuffers(unsigned int size, unsigned int index[], unsigned int callNo)
{
    for(unsigned int i=0; i<size; i++)
    {
    //to fix add delete
        map<unsigned int, bufferContext>::iterator it = allBufferList.find(index[i]);
        if(it != allBufferList.end())
        {
            allBufferList.erase(it);
        }
        else
        {
            //printf("error 21: no %d buffer in %d\n", index[i], callNo);
            DBG_LOG("error: no %d buffer in %d\n", index[i], callNo);
        }
/*
        if(index[i] == curGL_ARRAY_BUFFERIdx)
        {
            curGL_ARRAY_BUFFERIdx = 0;
            curGL_ARRAY_BUFFERNo = 0;
        }

        if(index[i] == curGL_ELEMENT_ARRAY_BUFFERIdx)
        {
            curGL_ELEMENT_ARRAY_BUFFERIdx = 0;
            curGL_ELEMENT_ARRAY_BUFFERNo = callNo;
        }

        if(index[i] == curGL_PIXEL_UNPACK_BUFFERIdx)
        {
            curGL_PIXEL_UNPACK_BUFFERIdx = 0;
            curGL_PIXEL_UNPACK_BUFFERNo = 0;
        }

        if(index[i] == curGL_COPY_WRITE_BUFFERIdx)
        {
            curGL_COPY_WRITE_BUFFERIdx = 0;
            curGL_COPY_WRITE_BUFFERNo = 0;
        }

        if(index[i] == curGL_DRAW_INDIRECT_BUFFERIdx)
        {
            curGL_DRAW_INDIRECT_BUFFERIdx = 0;
            curGL_DRAW_INDIRECT_BUFFERNo = 0;
        }

        if(index[i] == curGL_ATOMIC_COUNTER_BUFFERIdx)
        {
            curGL_ATOMIC_COUNTER_BUFFERIdx = 0;
            curGL_ATOMIC_COUNTER_BUFFERNo = 0;
        }
*/
    }
}

void bufferState::ff_glGenBuffers(unsigned int size, unsigned int index[], unsigned int callNo)
{
    for(unsigned int i=0; i<size; i++)
    {
        bufferContext newBuffer;
        newBuffer.glGenBuffersNo = callNo;
        //to fix add delete
        map<unsigned int, bufferContext>::iterator it = allBufferList.find(index[i]);
        if(it != allBufferList.end())
        {
            allBufferList.erase(it);
            //printf("error 22: already have %d buffer in %d\n", index[i], callNo);
            DBG_LOG("error: already have %d buffer in %d\n", index[i], callNo);
        }
        allBufferList.insert(pair<unsigned int, bufferContext>(index[i], newBuffer));
    }
}

void bufferState::ff_glGenVertexArrays(unsigned int size, unsigned int index[], unsigned int callNo)
{
    for(unsigned int i=0; i<size; i++)
    {
        vertexArrayContext newVao;
        newVao.glGenVertexArraysNo = callNo;
        //to fix add delete
        map<unsigned int, vertexArrayContext>::iterator it = allVaoList.find(index[i]);
        if(it != allVaoList.end())
        {
            allVaoList.erase(it);
            //printf("error 23: already have %d buffer in %d\n", index[i], callNo);
            DBG_LOG("error: already have %d buffer in %d\n", index[i], callNo);
        }
        allVaoList.insert(pair<unsigned int, vertexArrayContext>(index[i], newVao));
    }
}

void bufferState::ff_glBindBufferForVao(unsigned int type, unsigned int index, unsigned int callNo)
{
    if(type == GL_ELEMENT_ARRAY_BUFFER && curVertexArrayObjectIdx != 0)
    {
        map<unsigned int, vertexArrayContext>::iterator it =  allVaoList.find(curVertexArrayObjectIdx);
        if(it == allVaoList.end())
        {
            //printf("error 24:no genvertexarray before binding %d\n", callNo);
            DBG_LOG("error:no genvertexarray before binding %d\n", callNo);
            exit(1);
        }
        it->second.gl_element_array_bufferNo = {curVertexArrayObjectNo, callNo, index};
    }
}

void bufferState::ff_glBindBuffer(unsigned int type, unsigned int index, unsigned int callNo)
{
    if(type == GL_ARRAY_BUFFER)
    {
        curGL_ARRAY_BUFFERIdx = index;
        curGL_ARRAY_BUFFERNo = callNo;
    }
    else if(type == GL_ELEMENT_ARRAY_BUFFER)
    {
        curGL_ELEMENT_ARRAY_BUFFERIdx = index;
        curGL_ELEMENT_ARRAY_BUFFERNo = callNo;
    }
    else if(type == GL_PIXEL_UNPACK_BUFFER)
    {
        curGL_PIXEL_UNPACK_BUFFERIdx = index;
        curGL_PIXEL_UNPACK_BUFFERNo = callNo;
    }
    else if(type == GL_COPY_WRITE_BUFFER)
    {
        curGL_COPY_WRITE_BUFFERIdx = index;
        curGL_COPY_WRITE_BUFFERNo = callNo;
    }
    else if(type >= 37074 && type < 37074+MAX_GL_SHADER_STORAGE_BUFFER_NUM)//GL_SHADER_STORAGE_BUFFER0 - GL_SHADER_STORAGE_BUFFER31
    {
        curGL_SHADER_STORAGE_BUFFER0Idx[type - 37074] = index;
        curGL_SHADER_STORAGE_BUFFER0No[type - 37074] = callNo;
    }
    else if(type >= GL_UNIFORM_BUFFER && type < GL_UNIFORM_BUFFER+MAX_GL_UNIFORM_BUFFER_NUM)
    {
        curGL_UNIFORM_BUFFER0Idx[type - GL_UNIFORM_BUFFER] = index;
        curGL_UNIFORM_BUFFER0No[type - GL_UNIFORM_BUFFER] = callNo;
    }
    else if(type == GL_DRAW_INDIRECT_BUFFER)//36671)// GL_DRAW_INDIRECT_BUFFER
    {
        curGL_DRAW_INDIRECT_BUFFERIdx = index;
        curGL_DRAW_INDIRECT_BUFFERNo = callNo;
    }
    else if(type == GL_ATOMIC_COUNTER_BUFFER)//37568)//GL_ATOMIC_COUNTER_BUFFER
    {
        curGL_ATOMIC_COUNTER_BUFFERIdx = index;
        curGL_ATOMIC_COUNTER_BUFFERNo = callNo;
    }
    else if(type == GL_COPY_READ_BUFFER  || type == GL_DISPATCH_INDIRECT_BUFFER)//37102)//GL_DISPATCH_INDIRECT_BUFFER
    {
        // for Garden antutu
        //printf("warning 4: for antutu  %x %d in %d\n", type, index, callNo);
        DBG_LOG("warning: for antutu  %x %d in %d\n", type, index, callNo);
    }
    else
    {
        //printf("error 25: no parameters in buffer in %x  %d\n", type, callNo);
        DBG_LOG("error: no parameters in buffer in %x  %d index %d\n", type, callNo,index);
    }
}

void bufferState::ff_glBindVertexArray(unsigned int index, unsigned int callNo)
{
    unsigned int patch = 0;
    if(curVertexArrayObjectIdx != 0)
    {
        map<unsigned int, vertexArrayContext>::iterator it =  allVaoList.find(curVertexArrayObjectIdx);
        if(it == allVaoList.end())
        {
            //printf("error 26: no genvertexarray before binding %d\n", callNo);
            DBG_LOG("error: no genvertexarray before binding %d\n", callNo);
            exit(1);
        }
        map<unsigned int, vertexArrayContext::withBinder>::iterator attrib = it->second.glEnableVertexAttribArrayNoList.begin();
        while(attrib != it->second.glEnableVertexAttribArrayNoList.end())
        {
            if(attrib->second.isUsing == 1)
            {
                patch = 1;
                attrib->second.lastBinderCallNo = callNo;
                attrib->second.isUsing = 0;
            }
            attrib++;
        }
        if(patch == 0)
        {
            vertexArrayContext::withBinder newBinder = {curVertexArrayObjectNo, callNo, 0, 0};
            it->second.glEnableVertexAttribArrayNoList.insert(pair<unsigned int, vertexArrayContext::withBinder>(it->second.patchNum--, newBinder));
            it->second.patchNum--;
        }

    }
    curVertexArrayObjectIdx = index;
    curVertexArrayObjectNo = callNo;
}

unsigned int bufferState::getVAOindex()
{
    return curVertexArrayObjectIdx;
}

void bufferState::ff_glBufferData(unsigned int type, unsigned int callNo)
{
    map<unsigned int, bufferContext>::iterator it;
    bufferContext::withBinder curBinder;
    if(type == GL_ARRAY_BUFFER)
    {
        curBinder = {callNo, curGL_ARRAY_BUFFERNo};
        it = allBufferList.find(curGL_ARRAY_BUFFERIdx);
    }
    else if(type == GL_ELEMENT_ARRAY_BUFFER)
    {
        curBinder = {callNo, curGL_ELEMENT_ARRAY_BUFFERNo};
        it = allBufferList.find(curGL_ELEMENT_ARRAY_BUFFERIdx);
    }
    else if(type == GL_PIXEL_UNPACK_BUFFER)
    {
        curBinder = {callNo, curGL_PIXEL_UNPACK_BUFFERNo};
        it = allBufferList.find(curGL_PIXEL_UNPACK_BUFFERIdx);
    }
    else if(type == GL_COPY_WRITE_BUFFER)
    {
        curBinder = {callNo, curGL_COPY_WRITE_BUFFERNo};
        it = allBufferList.find(curGL_COPY_WRITE_BUFFERIdx);
    }
    else if(type >= GL_UNIFORM_BUFFER  && type < GL_UNIFORM_BUFFER+MAX_GL_UNIFORM_BUFFER_NUM)
    {
        curBinder = {callNo, curGL_UNIFORM_BUFFER0No[type - GL_UNIFORM_BUFFER]};
        it = allBufferList.find(curGL_UNIFORM_BUFFER0Idx[type - GL_UNIFORM_BUFFER]);
    }
    else if(type == GL_SHADER_STORAGE_BUFFER)//37074)//GL_SHADER_STORAGE_BUFFER
    {
        curBinder = {callNo, curGL_SHADER_STORAGE_BUFFER0No[0]};
        it = allBufferList.find(curGL_SHADER_STORAGE_BUFFER0Idx[0]);
    }
    else if(type == GL_ATOMIC_COUNTER_BUFFER)//37568)//GL_ATOMIC_COUNTER_BUFFER
    {
        curBinder = {callNo, curGL_ATOMIC_COUNTER_BUFFERNo};
        it = allBufferList.find(curGL_ATOMIC_COUNTER_BUFFERIdx);
    }
    else if(type == GL_DRAW_INDIRECT_BUFFER)
    {
        curBinder = {callNo, curGL_DRAW_INDIRECT_BUFFERNo};
        it = allBufferList.find(curGL_DRAW_INDIRECT_BUFFERIdx);
    }
    else if(type == GL_COPY_READ_BUFFER || type == GL_DISPATCH_INDIRECT_BUFFER)//37102)///
    {
        //        curGL_PIXEL_UNPACK_BUFFERIdx = index;
        //      for Garden antutu
    }
    else
    {
        //printf("error 27: no parameters in buffer in %d\n", callNo);
        DBG_LOG("error: no parameters in buffer in %d\n", callNo);
        exit(1);
    }
    //first number shoul be zero
    //TO FIX...
    if(type == GL_COPY_READ_BUFFER  || type == GL_DISPATCH_INDIRECT_BUFFER)//37102)///
    {
        //        curGL_PIXEL_UNPACK_BUFFERIdx = index;
        //      for Garden antutu
    }
    else
    {
        if(it->second.glBufferDataNoList.size() >= 3)
        {
            //it->second.glBufferDataNoList.pop_front();
        }
        it->second.glBufferDataIdx++;
        it->second.glBufferDataNoList.push_front(curBinder);
    }//for marooned test
}

void bufferState::ff_glBufferSubData(unsigned int type, unsigned int offset, unsigned int size, unsigned int callNo)
{
    map<unsigned int, bufferContext>::iterator it;
    bufferContext::withBinderSubData curBinder;
    if(type == GL_ARRAY_BUFFER)
    {
        curBinder = {callNo, curGL_ARRAY_BUFFERNo, offset, size};
        it = allBufferList.find(curGL_ARRAY_BUFFERIdx);
    }
    else if(type == GL_ELEMENT_ARRAY_BUFFER)
    {
        curBinder = {callNo, curGL_ELEMENT_ARRAY_BUFFERNo, offset, size};
        it = allBufferList.find(curGL_ELEMENT_ARRAY_BUFFERIdx);
    }
    else if(type == GL_PIXEL_UNPACK_BUFFER)
    {
        curBinder = {callNo, curGL_PIXEL_UNPACK_BUFFERNo, offset, size};
        it = allBufferList.find(curGL_PIXEL_UNPACK_BUFFERIdx);
    }
    else if(type == GL_SHADER_STORAGE_BUFFER)//37074)//GL_SHADER_STORAGE_BUFFER
    {
        curBinder = {callNo, curGL_SHADER_STORAGE_BUFFER0No[0]};
        it = allBufferList.find(curGL_SHADER_STORAGE_BUFFER0Idx[0]);
    }
    else if(type == GL_UNIFORM_BUFFER)
    {
        curBinder = {callNo, curGL_UNIFORM_BUFFER0No[0], offset, size};
        it = allBufferList.find(curGL_UNIFORM_BUFFER0Idx[0]);
    }
    else if(type == GL_ATOMIC_COUNTER_BUFFER)//37568)// GL_ATOMIC_COUNTER_BUFFER
    {
        curBinder = {callNo, curGL_ATOMIC_COUNTER_BUFFERNo, offset, size};
        it = allBufferList.find(curGL_ATOMIC_COUNTER_BUFFERIdx);
    }
    else
    {
        //printf("error 28: no parameters in buffer %d\n", callNo);
        DBG_LOG("error: no parameters in buffer %d\n", callNo);
        exit(1);
    }
    //first number shoul be zero
    it->second.glBufferSubDataIdx++;
    it->second.glBufferSubDataNoList.push_front(curBinder);
}

unsigned int bufferState::getCurGL_ARRAY_BUFFERIdx()
{
    return curGL_ARRAY_BUFFERIdx;
}

unsigned int bufferState::getCurGL_ARRAY_BUFFERNo()
{
    return curGL_ARRAY_BUFFERNo;
}

unsigned int bufferState::getCurGL_ELEMENT_ARRAY_BUFFERNo()
{
    return curGL_ELEMENT_ARRAY_BUFFERNo;
}

unsigned int bufferState::getCurGL_ELEMENT_ARRAY_BUFFERIdx()
{
    return curGL_ELEMENT_ARRAY_BUFFERIdx;
}

unsigned int bufferState::getCurGL_PIXEL_UNPACK_BUFFERIdx()
{
    return curGL_PIXEL_UNPACK_BUFFERIdx;
}

unsigned int bufferState::getCurGL_PIXEL_UNPACK_BUFFERNo()
{
    return curGL_PIXEL_UNPACK_BUFFERNo;
}

void bufferState::readCurBufferState(unsigned int type, vector<unsigned int> &bufferList, unordered_set<unsigned int> &callNoList)
{
    unsigned int firstOffset;
    unsigned int lastOffset;
    if(type == 0)//glDrawArrays
    {
        callNoList.insert(curGL_ARRAY_BUFFERNo);
        callNoList.insert(curGL_ELEMENT_ARRAY_BUFFERNo);
        if(curGL_ARRAY_BUFFERIdx != 0)
        {
            map<unsigned int, bufferContext>::iterator it = allBufferList.find(curGL_ARRAY_BUFFERIdx);
            callNoList.insert(it->second.glGenBuffersNo);

            list<bufferContext::withBinder>::iterator it2 = it->second.glBufferDataNoList.begin();
            while(it2 != it->second.glBufferDataNoList.end())
            {
                callNoList.insert((*it2).thisCallNo);
                callNoList.insert((*it2).binderCallNo);
                it2++;
            }
            //callNoList.insert(it->second.glBufferDataNoList.front().thisCallNo);
            //callNoList.insert(it->second.glBufferDataNoList.front().binderCallNo);
            //callNoList.insert(it->second.glBufferDataNoList.back().thisCallNo);
            //callNoList.insert(it->second.glBufferDataNoList.back().binderCallNo);
            list<bufferContext::withBinderSubData>::iterator potr2 = it->second.glBufferSubDataNoList.begin();
            map<unsigned int, bufferContext::mapBuffer>::iterator potr4;
            firstOffset = MAX_INT;
            lastOffset = 0;
            while(potr2 != it->second.glBufferSubDataNoList.end())
            {
                if(potr2->thisCallNo >= MAP_BUFFER_RANGE)
                {
                    potr4 =  it->second.glMapBufferRangeNoList.find(potr2->thisCallNo - MAP_BUFFER_RANGE);
                    if(potr4->second.glCopyClientSideBufferNo != 0)
                    {
                        callNoList.insert(potr4->second.glMapBufferRangeNo);
                        callNoList.insert(potr4->second.glMapBufferRangeBinderNo);
                        callNoList.insert(potr4->second.glCopyClientSideBufferNo);
                        callNoList.insert(potr4->second.glCopyClientSideBufferBinderNo);
                        callNoList.insert(potr4->second.glCopyClientSideBufferIdxNo%FIRST_10_BIT);
                        callNoList.insert(potr4->second.glCopyClientSideBufferIdxNo/FIRST_10_BIT);
                        callNoList.insert(potr4->second.glFlushMappedBufferRangeNo);
                        callNoList.insert(potr4->second.glFlushMappedBufferRangeBinderNo);
                        callNoList.insert(potr4->second.glUnmapBufferNo);
                    }
                }
                else
                {
                    if(1)///*potr2->offSet < firstOffset || potr2->offSet+potr2->size > lastOffset*/
                    {
                        if(potr2->offSet < firstOffset)
                        {
                            firstOffset = potr2->offSet;
                        }
                        if(potr2->offSet+potr2->size > lastOffset)
                        {
                            lastOffset = potr2->offSet+potr2->size;
                        }
                            callNoList.insert(potr2->thisCallNo);
                            callNoList.insert(potr2->binderCallNo);
                    }
                }
                potr2++;
            }
        }//if !=0
    }
    else if(type == 1)//glDrawElements
    {
        if(curGL_ELEMENT_ARRAY_BUFFERIdx != 0)
        {
            callNoList.insert(curGL_ELEMENT_ARRAY_BUFFERNo);
            map<unsigned int, bufferContext>::iterator it = allBufferList.find(curGL_ELEMENT_ARRAY_BUFFERIdx);
            callNoList.insert(it->second.glGenBuffersNo);

            list<bufferContext::withBinder>::iterator it2 = it->second.glBufferDataNoList.begin();
            while(it2 != it->second.glBufferDataNoList.end())
            {
                callNoList.insert((*it2).thisCallNo);
                callNoList.insert((*it2).binderCallNo);
                it2++;
            }
            //callNoList.insert(it->second.glBufferDataNoList.front().thisCallNo);
            //callNoList.insert(it->second.glBufferDataNoList.front().binderCallNo);
            //callNoList.insert(it->second.glBufferDataNoList.back().thisCallNo);
            //callNoList.insert(it->second.glBufferDataNoList.back().binderCallNo);
            list<bufferContext::withBinderSubData>::iterator potr2 = it->second.glBufferSubDataNoList.begin();
            map<unsigned int, bufferContext::mapBuffer>::iterator potr4;
            firstOffset = MAX_INT;
            lastOffset = 0;
            while(potr2 != it->second.glBufferSubDataNoList.end())
            {
                if(potr2->thisCallNo>=MAP_BUFFER_RANGE)
                {
                    potr4 =  it->second.glMapBufferRangeNoList.find(potr2->thisCallNo-MAP_BUFFER_RANGE);
                    if(potr4->second.glCopyClientSideBufferNo != 0)
                    {
                        callNoList.insert(potr4->second.glMapBufferRangeNo);
                        callNoList.insert(potr4->second.glMapBufferRangeBinderNo);
                        callNoList.insert(potr4->second.glCopyClientSideBufferNo);
                        callNoList.insert(potr4->second.glCopyClientSideBufferBinderNo);
                        callNoList.insert(potr4->second.glCopyClientSideBufferIdxNo%FIRST_10_BIT);
                        callNoList.insert(potr4->second.glCopyClientSideBufferIdxNo/FIRST_10_BIT);
                        callNoList.insert(potr4->second.glFlushMappedBufferRangeNo);
                        callNoList.insert(potr4->second.glFlushMappedBufferRangeBinderNo);
                        callNoList.insert(potr4->second.glUnmapBufferNo);
                    }
                }
                else
                {
                    if(1)///*potr2->offSet < firstOffset || potr2->offSet+potr2->size > lastOffset*/
                    {
                        if(potr2->offSet < firstOffset)
                        {
                            firstOffset = potr2->offSet;
                        }
                        if(potr2->offSet+potr2->size > lastOffset)
                        {
                            lastOffset = potr2->offSet + potr2->size;
                        }
                        callNoList.insert(potr2->thisCallNo);
                        callNoList.insert(potr2->binderCallNo);
                    }
                }
                potr2++;
            }
        }
        callNoList.insert(curGL_ARRAY_BUFFERNo);
        callNoList.insert(curGL_ELEMENT_ARRAY_BUFFERNo);
        if(curGL_ARRAY_BUFFERIdx != 0)
        {
            map<unsigned int, bufferContext>::iterator it = allBufferList.find(curGL_ARRAY_BUFFERIdx);
            callNoList.insert(it->second.glGenBuffersNo);

            list<bufferContext::withBinder>::iterator it2 = it->second.glBufferDataNoList.begin();
            while(it2 != it->second.glBufferDataNoList.end())
            {
                callNoList.insert((*it2).thisCallNo);
                callNoList.insert((*it2).binderCallNo);
                it2++;
            }
            //callNoList.insert(it->second.glBufferDataNoList.front().thisCallNo);
            //callNoList.insert(it->second.glBufferDataNoList.front().binderCallNo);
            //callNoList.insert(it->second.glBufferDataNoList.back().thisCallNo);
            //callNoList.insert(it->second.glBufferDataNoList.back().binderCallNo);
            list<bufferContext::withBinderSubData>::iterator potr2 = it->second.glBufferSubDataNoList.begin();
            map<unsigned int, bufferContext::mapBuffer>::iterator potr4;
            firstOffset = MAX_INT;
            lastOffset = 0;
            while(potr2 != it->second.glBufferSubDataNoList.end())
            {
                if(potr2->thisCallNo>=MAP_BUFFER_RANGE)
                {
                    potr4 =  it->second.glMapBufferRangeNoList.find(potr2->thisCallNo-MAP_BUFFER_RANGE);
                    if(potr4->second.glCopyClientSideBufferNo != 0)
                    {
                        callNoList.insert(potr4->second.glMapBufferRangeNo);
                        callNoList.insert(potr4->second.glMapBufferRangeBinderNo);
                        callNoList.insert(potr4->second.glCopyClientSideBufferNo);
                        callNoList.insert(potr4->second.glCopyClientSideBufferBinderNo);
                        callNoList.insert(potr4->second.glCopyClientSideBufferIdxNo%FIRST_10_BIT);
                        callNoList.insert(potr4->second.glCopyClientSideBufferIdxNo/FIRST_10_BIT);
                        callNoList.insert(potr4->second.glFlushMappedBufferRangeNo);
                        callNoList.insert(potr4->second.glFlushMappedBufferRangeBinderNo);
                        callNoList.insert(potr4->second.glUnmapBufferNo);
                    }
                }
                else
                {
                    if(1)///*potr2->offSet < firstOffset || potr2->offSet+potr2->size > lastOffset */
                    {
                        if(potr2->offSet < firstOffset)
                        {
                            firstOffset = potr2->offSet;
                        }
                        if(potr2->offSet+potr2->size > lastOffset)
                        {
                            lastOffset = potr2->offSet + potr2->size;
                        }
                        callNoList.insert(potr2->thisCallNo);
                        callNoList.insert(potr2->binderCallNo);
                    }
                }
                potr2++;
            }
        }//if !=0
    }//else if
    else
    {
        //printf("error 29: no parameters in buffer\n");
        DBG_LOG("error: no parameters in buffer\n");
        exit(1);
    }
    callNoList.insert(curVertexArrayObjectNo);
    if(curVertexArrayObjectIdx != DEFAULT_VAO_INDEX && curVertexArrayObjectIdx != 0)
    {
        map<unsigned int, vertexArrayContext>::iterator itvao =  allVaoList.find(curVertexArrayObjectIdx);
        callNoList.insert(itvao->second.glGenVertexArraysNo);
        map<unsigned int, vertexArrayContext::withPointerBuffer>::iterator potrvao = itvao->second.glVertexAttribPointerNoList.begin();

        while(potrvao != itvao->second.glVertexAttribPointerNoList.end())
        {
            callNoList.insert(potrvao->second.binderNo);
            callNoList.insert(potrvao->second.thisCallNo);
            callNoList.insert(potrvao->second.bufferNo);
            if(potrvao->second.clientSideBufferIdx != NO_IDX)
            {
                //printf("error 30:need clintSidebuffer!\n");
                DBG_LOG("error:need clintSidebuffer!\n");
                exit(1);
            }
            if(potrvao->second.bufferIdx != 0)
            {
                bufferList.push_back(potrvao->second.bufferIdx);
            }
            potrvao++;
        }//while
        map<unsigned int, vertexArrayContext::withBinder>::iterator potrvao2 = itvao->second.glEnableVertexAttribArrayNoList.begin();
        while(potrvao2 != itvao->second.glEnableVertexAttribArrayNoList.end())
        {
            callNoList.insert(potrvao2->second.thisCallNo);
            callNoList.insert(potrvao2->second.binderCallNo);
            callNoList.insert(potrvao2->second.lastBinderCallNo);
            potrvao2++;
        }

        callNoList.insert(itvao->second.gl_element_array_bufferNo.thisCallNo);
        callNoList.insert(itvao->second.gl_element_array_bufferNo.binderCallNo);
        bufferList.push_back(itvao->second.gl_element_array_bufferNo.elementArrayBufferIdx);
    }
    else if(curVertexArrayObjectIdx == DEFAULT_VAO_INDEX)
    {
        map<unsigned int, vertexArrayContext>::iterator itvao =  allVaoList.find(curVertexArrayObjectIdx);
        map<unsigned int, vertexArrayContext::withBinder>::iterator potrvao2 = itvao->second.glEnableVertexAttribArrayNoList.begin();
        while(potrvao2 != itvao->second.glEnableVertexAttribArrayNoList.end())
        {
            callNoList.insert(potrvao2->second.thisCallNo);
            callNoList.insert(potrvao2->second.binderCallNo);
            callNoList.insert(potrvao2->second.lastBinderCallNo);
            potrvao2++;
        }

        callNoList.insert(itvao->second.gl_element_array_bufferNo.thisCallNo);
        callNoList.insert(itvao->second.gl_element_array_bufferNo.binderCallNo);
        bufferList.push_back(itvao->second.gl_element_array_bufferNo.elementArrayBufferIdx);
    }
    else
    {
        DBG_LOG("error:VAO == 0!\n");
    }
    //uniform insert
    for(unsigned int i =0; i<MAX_GL_UNIFORM_BUFFER_NUM; i++)
    {
        bufferList.push_back(curGL_UNIFORM_BUFFER0Idx[i]);
        callNoList.insert(curGL_UNIFORM_BUFFER0No[i]);
    }
    bufferList.push_back(curGL_COPY_WRITE_BUFFERIdx);
    callNoList.insert(curGL_COPY_WRITE_BUFFERNo);
    bufferList.push_back(curGL_DRAW_INDIRECT_BUFFERIdx);
    callNoList.insert(curGL_DRAW_INDIRECT_BUFFERNo);
    bufferList.push_back(curGL_ATOMIC_COUNTER_BUFFERIdx);
    callNoList.insert(curGL_ATOMIC_COUNTER_BUFFERNo);
    for(unsigned int i=0; i<bufferList.size(); i++)
    {
        if(bufferList[i] == 0)
        {
            continue;
        }
        map<unsigned int, bufferContext>::iterator it = allBufferList.find(bufferList[i]);
        if(it == allBufferList.end())
        {
            //printf("warning 5: no %d buffer in the bufferList\n", bufferList[i]);
            DBG_LOG("warning: no %d buffer in the bufferList\n", bufferList[i]);
            continue;
        }
        callNoList.insert(it->second.glGenBuffersNo);

        list<bufferContext::withBinder>::iterator it2 = it->second.glBufferDataNoList.begin();
        while(it2 != it->second.glBufferDataNoList.end())
        {
            callNoList.insert((*it2).thisCallNo);
            callNoList.insert((*it2).binderCallNo);
            it2++;
        }
        //callNoList.insert(it->second.glBufferDataNoList.front().thisCallNo);
        //callNoList.insert(it->second.glBufferDataNoList.front().binderCallNo);
        //callNoList.insert(it->second.glBufferDataNoList.back().thisCallNo);
        //callNoList.insert(it->second.glBufferDataNoList.back().binderCallNo);

        list<bufferContext::withBinderSubData>::iterator potr2 = it->second.glBufferSubDataNoList.begin();
        map<unsigned int, bufferContext::mapBuffer>::iterator potr4;
        firstOffset = MAX_INT;
        lastOffset = 0;
        while(potr2 != it->second.glBufferSubDataNoList.end())
        {
            if(potr2->thisCallNo >= MAP_BUFFER_RANGE)
            {
                potr4 =  it->second.glMapBufferRangeNoList.find(potr2->thisCallNo - MAP_BUFFER_RANGE);
                if(potr4->second.glCopyClientSideBufferNo != 0)
                {
                    callNoList.insert(potr4->second.glMapBufferRangeNo);
                    callNoList.insert(potr4->second.glMapBufferRangeBinderNo);
                    callNoList.insert(potr4->second.glCopyClientSideBufferNo);
                    callNoList.insert(potr4->second.glCopyClientSideBufferBinderNo);
                    callNoList.insert(potr4->second.glCopyClientSideBufferIdxNo%FIRST_10_BIT);
                    callNoList.insert(potr4->second.glCopyClientSideBufferIdxNo/FIRST_10_BIT);
                    callNoList.insert(potr4->second.glFlushMappedBufferRangeNo);
                    callNoList.insert(potr4->second.glFlushMappedBufferRangeBinderNo);
                    callNoList.insert(potr4->second.glUnmapBufferNo);
                }
            }
            else
            {
                //if(1)///*potr2->offSet < firstOffset || potr2->offSet+potr2->size > lastOffset */

                if(potr2->offSet < firstOffset)
                {
                    firstOffset = potr2->offSet;
                }
                if(potr2->offSet+potr2->size > lastOffset)
                {
                    lastOffset = potr2->offSet + potr2->size;
                }
                callNoList.insert(potr2->thisCallNo);
                callNoList.insert(potr2->binderCallNo);
            }
            potr2++;
        }
    }
}

void bufferState::findABufferState(unsigned int index, unordered_set<unsigned int> &callNoList)
{
    unsigned int firstOffset;
    unsigned int lastOffset;
    map<unsigned int, bufferContext>::iterator it = allBufferList.find(index);
    if(it == allBufferList.end())
    {
        //printf("warning 6:no %d buffer in the bufferList\n", index);
        DBG_LOG("warning:no %d buffer in the bufferList\n", index);
    }
    else
    {
        callNoList.insert(it->second.glGenBuffersNo);

        list<bufferContext::withBinder>::iterator it2 = it->second.glBufferDataNoList.begin();
        while(it2 != it->second.glBufferDataNoList.end())
        {
            callNoList.insert((*it2).thisCallNo);
            callNoList.insert((*it2).binderCallNo);
            it2++;
        }
        //callNoList.insert(it->second.glBufferDataNoList.front().thisCallNo);
        //callNoList.insert(it->second.glBufferDataNoList.front().binderCallNo);
        //callNoList.insert(it->second.glBufferDataNoList.back().thisCallNo);
        //callNoList.insert(it->second.glBufferDataNoList.back().binderCallNo);

        list<bufferContext::withBinderSubData>::iterator potr2 = it->second.glBufferSubDataNoList.begin();
        map<unsigned int, bufferContext::mapBuffer>::iterator potr4;
        firstOffset = MAX_INT;
        lastOffset = 0;
        while(potr2 != it->second.glBufferSubDataNoList.end())
        {
            if(potr2->thisCallNo>=MAP_BUFFER_RANGE)
            {
                potr4 =  it->second.glMapBufferRangeNoList.find(potr2->thisCallNo - MAP_BUFFER_RANGE);
                if(potr4->second.glCopyClientSideBufferNo != 0)
                {
                    callNoList.insert(potr4->second.glMapBufferRangeNo);
                    callNoList.insert(potr4->second.glMapBufferRangeBinderNo);
                    callNoList.insert(potr4->second.glCopyClientSideBufferNo);
                    callNoList.insert(potr4->second.glCopyClientSideBufferBinderNo);
                    callNoList.insert(potr4->second.glCopyClientSideBufferIdxNo%FIRST_10_BIT);
                    callNoList.insert(potr4->second.glCopyClientSideBufferIdxNo/FIRST_10_BIT);
                    callNoList.insert(potr4->second.glFlushMappedBufferRangeNo);
                    callNoList.insert(potr4->second.glFlushMappedBufferRangeBinderNo);
                    callNoList.insert(potr4->second.glUnmapBufferNo);
                }
            }
            else
            {
            //if(1)///*potr2->offSet < firstOffset || potr2->offSet+potr2->size > lastOffset*/

                if(potr2->offSet < firstOffset)
                {
                    firstOffset = potr2->offSet;
                }
                if(potr2->offSet+potr2->size > lastOffset)
                {
                    lastOffset = potr2->offSet+potr2->size;
                }
                    callNoList.insert(potr2->thisCallNo);
                    callNoList.insert(potr2->binderCallNo);
            }
            potr2++;
        }
    }//else....
}

void bufferState::ff_glMapBufferRange(unsigned int target, unsigned int offset, unsigned int size, unsigned int callNo)
{
    unsigned int binderNo = 0;
    unsigned int binderIdx = 0;
    if(target == GL_ARRAY_BUFFER)
    {
        binderNo = curGL_ARRAY_BUFFERNo;
        binderIdx = curGL_ARRAY_BUFFERIdx;
    }
    else if(target == GL_ELEMENT_ARRAY_BUFFER)
    {
        binderNo = curGL_ELEMENT_ARRAY_BUFFERNo;
        binderIdx = curGL_ELEMENT_ARRAY_BUFFERIdx;
    }
    else if(target == GL_PIXEL_UNPACK_BUFFER)
    {
        binderNo = curGL_PIXEL_UNPACK_BUFFERNo;
        binderIdx = curGL_PIXEL_UNPACK_BUFFERIdx;
    }
    else if(target == GL_DRAW_INDIRECT_BUFFER)
    {
        binderNo = curGL_DRAW_INDIRECT_BUFFERNo;
        binderIdx = curGL_DRAW_INDIRECT_BUFFERIdx;
    }
    else if(target == GL_COPY_WRITE_BUFFER)
    {
        binderNo = curGL_COPY_WRITE_BUFFERNo;
        binderIdx = curGL_COPY_WRITE_BUFFERIdx;
    }
    else if(target >= GL_UNIFORM_BUFFER  && target < GL_UNIFORM_BUFFER+MAX_GL_UNIFORM_BUFFER_NUM)
    {
        binderNo = curGL_UNIFORM_BUFFER0No[target - GL_UNIFORM_BUFFER];
        binderIdx = curGL_UNIFORM_BUFFER0Idx[target - GL_UNIFORM_BUFFER];
    }
    else
    {
        //printf("error 31: glMapBufferRange not support parameter!\n");
        DBG_LOG("error: glMapBufferRange not support parameter! callno %d\n", callNo);
    }
    map<unsigned int, bufferContext>::iterator it = allBufferList.find(binderIdx);
    if(it == allBufferList.end())
    {
        //printf("error 32: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
        DBG_LOG("error: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
        exit(1);
    }
    it->second.glBufferSubDataIdx++;
    bufferContext::withBinderSubData curBinder;
    if(target == GL_ARRAY_BUFFER)
    {
        it->second.glCopyClientSideBuffer_GL_ARRAY_BUFFERIdx = it->second.glBufferSubDataIdx;
        curBinder = {(it->second.glBufferSubDataIdx)+MAP_BUFFER_RANGE, 0, offset, size};
    }
    else if(target == GL_ELEMENT_ARRAY_BUFFER)
    {
        it->second.glCopyClientSideBuffer_GL_ELEMENT_ARRAY_BUFFERIdx = it->second.glBufferSubDataIdx;
        curBinder = {(it->second.glBufferSubDataIdx)+MAP_BUFFER_RANGE, 0, offset, size};
    }
    else if(target == GL_PIXEL_UNPACK_BUFFER)
    {
        it->second.glCopyClientSideBuffer_GL_PIXEL_UNPACK_BUFFERIdx = it->second.glBufferSubDataIdx;
        curBinder = {(it->second.glBufferSubDataIdx)+MAP_BUFFER_RANGE, 0, offset, size};
    }
    else if(target == GL_DRAW_INDIRECT_BUFFER)
    {
        it->second.glCopyClientSideBuffer_GL_DRAW_INDIRECT_BUFFERIdx = it->second.glBufferSubDataIdx;
        curBinder = {(it->second.glBufferSubDataIdx)+MAP_BUFFER_RANGE, 0, offset, size};
    }
    else if(target == GL_COPY_WRITE_BUFFER)
    {
        it->second.glCopyClientSideBuffer_GL_COPY_WRITE_BUFFERIdx = it->second.glBufferSubDataIdx;
        curBinder = {(it->second.glBufferSubDataIdx)+MAP_BUFFER_RANGE, 0, offset, size};
    }
    else if(target >= GL_UNIFORM_BUFFER  && target < GL_UNIFORM_BUFFER + MAX_GL_UNIFORM_BUFFER_NUM)
    {
        it->second.glCopyClientSideBuffer_GL_UNIFORM_BUFFERIdx = it->second.glBufferSubDataIdx;
        curBinder = {(it->second.glBufferSubDataIdx)+MAP_BUFFER_RANGE, 0, offset, size};
    }

    it->second.glBufferSubDataNoList.push_front(curBinder);
    map<unsigned int, bufferContext::mapBuffer>::iterator potr = it->second.glMapBufferRangeNoList.find(it->second.glBufferSubDataIdx);

    if(potr == it->second.glMapBufferRangeNoList.end())
    {
        bufferContext::mapBuffer newMap = {0, 0, 0, 0, 0, 0, 0, 0};
        newMap.glMapBufferRangeNo = callNo;
        newMap.glMapBufferRangeBinderNo = binderNo;
        it->second.glMapBufferRangeNoList.insert(pair<unsigned int, bufferContext::mapBuffer>(it->second.glBufferSubDataIdx, newMap));
    }
    else
    {
        potr->second.glCopyClientSideBufferNo = 0;
        potr->second.glCopyClientSideBufferBinderNo = 0;
        potr->second.glCopyClientSideBufferIdxNo = 0;
        potr->second.glUnmapBufferNo = 0;
        potr->second.glFlushMappedBufferRangeNo = 0;
        potr->second.glFlushMappedBufferRangeBinderNo = 0;
        potr->second.glMapBufferRangeNo = callNo;
        potr->second.glMapBufferRangeBinderNo = binderNo;
    }
}

void bufferState::ff_glCopyClientSideBuffer(unsigned int target, unsigned long long nameNo, unsigned int callNo)
{
    unsigned int binderNo;
    unsigned int binderIdx;
    unsigned int glMapBufferRangeNoListIdx;
    map<unsigned int, bufferContext>::iterator it;
    if(target == GL_ARRAY_BUFFER)
    {
        binderNo = curGL_ARRAY_BUFFERNo;
        binderIdx = curGL_ARRAY_BUFFERIdx;
        it = allBufferList.find(binderIdx);
        if(it == allBufferList.end())
        {
            //printf("error 33:no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            DBG_LOG("error:no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            exit(1);
        }
        glMapBufferRangeNoListIdx = it->second.glCopyClientSideBuffer_GL_ARRAY_BUFFERIdx;
        it->second.glMapBufferRange_GL_ARRAY_BUFFEREnd = true;
    }
    else if(target == GL_ELEMENT_ARRAY_BUFFER)
    {
        binderNo = curGL_ELEMENT_ARRAY_BUFFERNo;
        binderIdx = curGL_ELEMENT_ARRAY_BUFFERIdx;
        it = allBufferList.find(binderIdx);
        if(it == allBufferList.end())
        {
            //printf("error 34: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            DBG_LOG("error: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            exit(1);
        }
        glMapBufferRangeNoListIdx = it->second.glCopyClientSideBuffer_GL_ELEMENT_ARRAY_BUFFERIdx;
        it->second.glMapBufferRange_GL_ELEMENT_ARRAY_BUFFEREnd = true;
    }
    else if(target == GL_PIXEL_UNPACK_BUFFER)
    {
        binderNo = curGL_PIXEL_UNPACK_BUFFERNo;
        binderIdx = curGL_PIXEL_UNPACK_BUFFERIdx;
        it = allBufferList.find(binderIdx);
        if(it == allBufferList.end())
        {
            //printf("error 35: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            DBG_LOG("error: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            exit(1);
        }
        glMapBufferRangeNoListIdx = it->second.glCopyClientSideBuffer_GL_PIXEL_UNPACK_BUFFERIdx;
        it->second.glMapBufferRange_GL_PIXEL_UNPACK_BUFFEREnd = true;
    }
    else if(target == GL_DRAW_INDIRECT_BUFFER)
    {
        binderNo = curGL_DRAW_INDIRECT_BUFFERNo;
        binderIdx = curGL_DRAW_INDIRECT_BUFFERIdx;
        it = allBufferList.find(binderIdx);
        if(it == allBufferList.end())
        {
            //printf("error 35: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            DBG_LOG("error: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            exit(1);
        }
        glMapBufferRangeNoListIdx = it->second.glCopyClientSideBuffer_GL_DRAW_INDIRECT_BUFFERIdx;
        it->second.glMapBufferRange_GL_DRAW_INDIRECT_BUFFEREnd = true;
    }
    else if(target == GL_COPY_WRITE_BUFFER)
    {
        binderNo = curGL_COPY_WRITE_BUFFERNo;
        binderIdx = curGL_COPY_WRITE_BUFFERIdx;
        it = allBufferList.find(binderIdx);
        if(it == allBufferList.end())
        {
            //printf("error 35: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            DBG_LOG("error: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            exit(1);
        }
        glMapBufferRangeNoListIdx = it->second.glCopyClientSideBuffer_GL_COPY_WRITE_BUFFERIdx;
        it->second.glMapBufferRange_GL_COPY_WRITE_BUFFEREnd = true;
    }
    else if(target >= GL_UNIFORM_BUFFER  && target < GL_UNIFORM_BUFFER + MAX_GL_UNIFORM_BUFFER_NUM)
    {
        binderNo = curGL_UNIFORM_BUFFER0No[target - GL_UNIFORM_BUFFER];
        binderIdx = curGL_UNIFORM_BUFFER0Idx[target - GL_UNIFORM_BUFFER];
        it = allBufferList.find(binderIdx);
        if(it == allBufferList.end())
        {
            //printf("error 36:no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            DBG_LOG("error:no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            exit(1);
        }
        glMapBufferRangeNoListIdx = it->second.glCopyClientSideBuffer_GL_UNIFORM_BUFFERIdx;
        it->second.glMapBufferRange_GL_UNIFORM_BUFFEREnd = true;
    }
    else
    {
        binderNo = 0;
        //printf("error 37: glCopyBufferRange not support parameter!\n");
        DBG_LOG("error: glCopyBufferRange not support parameter!\n");
    }
    map<unsigned int, bufferContext::mapBuffer>::iterator potr = it->second.glMapBufferRangeNoList.find(glMapBufferRangeNoListIdx);
    if(potr == it->second.glMapBufferRangeNoList.end())
    {
        //printf("warning 6: no mapbuffer before copyclientsidebuffer in %d\n", callNo);//exit(1);
        DBG_LOG("warning: no mapbuffer before copyclientsidebuffer in %d\n", callNo);
    }
    else
    {
        potr->second.glCopyClientSideBufferNo = callNo;
        potr->second.glCopyClientSideBufferBinderNo = binderNo;
        potr->second.glCopyClientSideBufferIdxNo = nameNo;
    }
}

void bufferState::ff_glFlushMappedBufferRange(unsigned int target, unsigned int callNo)
{
    unsigned int binderNo;
    unsigned int binderIdx;
    unsigned int glMapBufferRangeNoListIdx;
    map<unsigned int, bufferContext>::iterator it;
    if(target == GL_ARRAY_BUFFER)
    {
        binderNo = curGL_ARRAY_BUFFERNo;
        binderIdx = curGL_ARRAY_BUFFERIdx;
        it = allBufferList.find(binderIdx);
        if(it == allBufferList.end())
        {
            //printf("error 38: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            DBG_LOG("error: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            exit(1);
        }
        glMapBufferRangeNoListIdx = it->second.glCopyClientSideBuffer_GL_ARRAY_BUFFERIdx;
    }
    else if(target == GL_ELEMENT_ARRAY_BUFFER)
    {
        binderNo = curGL_ELEMENT_ARRAY_BUFFERNo;
        binderIdx = curGL_ELEMENT_ARRAY_BUFFERIdx;
        it = allBufferList.find(binderIdx);
        if(it == allBufferList.end())
        {
            //printf("error 39: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            DBG_LOG("error: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            exit(1);
        }
        glMapBufferRangeNoListIdx = it->second.glCopyClientSideBuffer_GL_ELEMENT_ARRAY_BUFFERIdx;
    }
    else if(target == GL_PIXEL_UNPACK_BUFFER)
    {
        binderNo = curGL_PIXEL_UNPACK_BUFFERNo;
        binderIdx = curGL_PIXEL_UNPACK_BUFFERIdx;
        it = allBufferList.find(binderIdx);
        if(it == allBufferList.end())
        {
            //printf("error 40: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            DBG_LOG("error: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            exit(1);
        }
        glMapBufferRangeNoListIdx = it->second.glCopyClientSideBuffer_GL_PIXEL_UNPACK_BUFFERIdx;
    }
    else if(target == GL_COPY_WRITE_BUFFER)
    {
        binderNo = curGL_COPY_WRITE_BUFFERNo;
        binderIdx = curGL_COPY_WRITE_BUFFERIdx;
        it = allBufferList.find(binderIdx);
        if(it == allBufferList.end())
        {
            //printf("error 40: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            DBG_LOG("error: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            exit(1);
        }
        glMapBufferRangeNoListIdx = it->second.glCopyClientSideBuffer_GL_COPY_WRITE_BUFFERIdx;
    }
    else
    {
        binderNo = 0;
        //printf("error 41: glCopyBufferRange not support parameter!\n");
        DBG_LOG("error: glCopyBufferRange not support parameter!\n");
    }
    map<unsigned int, bufferContext::mapBuffer>::iterator potr = it->second.glMapBufferRangeNoList.find(glMapBufferRangeNoListIdx);
    if(potr == it->second.glMapBufferRangeNoList.end())
    {
        //printf("error 42: no mapbuffer before copyclientsidebuffer in %d\n", callNo);//exit(1);
        DBG_LOG("error: no mapbuffer before copyclientsidebuffer in %d\n", callNo);//exit(1);
    }
    else
    {
        potr->second.glFlushMappedBufferRangeNo = callNo;
        potr->second.glFlushMappedBufferRangeBinderNo = binderNo;
    }
}

void bufferState::ff_glUnmapBuffer(unsigned int target, unsigned int callNo)
{
    unsigned int binderNo= 0;
    unsigned int binderIdx = 0;
    unsigned int glMapBufferRangeNoListIdx = 0;
    map<unsigned int, bufferContext>::iterator it;
    if(target == GL_ARRAY_BUFFER)
    {
        binderNo = curGL_ARRAY_BUFFERNo;
        binderIdx = curGL_ARRAY_BUFFERIdx;
        it = allBufferList.find(binderIdx);
        if(it == allBufferList.end())
        {
            //printf("error 43:no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            DBG_LOG("error:no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            exit(1);
        }
    }
    else if(target == GL_ELEMENT_ARRAY_BUFFER)
    {
        binderNo = curGL_ELEMENT_ARRAY_BUFFERNo;
        binderIdx = curGL_ELEMENT_ARRAY_BUFFERIdx;
        it = allBufferList.find(binderIdx);
        if(it == allBufferList.end())
        {
            //printf("error 44:no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            DBG_LOG("error:no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            exit(1);
        }
    }
    else if(target == GL_PIXEL_UNPACK_BUFFER)
    {
        binderNo = curGL_PIXEL_UNPACK_BUFFERNo;
        binderIdx = curGL_PIXEL_UNPACK_BUFFERIdx;
        it = allBufferList.find(binderIdx);
        if(it == allBufferList.end())
        {
            //printf("error 45: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            DBG_LOG("error: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            exit(1);
        }
    }
    else if(target == GL_DRAW_INDIRECT_BUFFER)
    {
        binderNo = curGL_DRAW_INDIRECT_BUFFERNo;
        binderIdx = curGL_DRAW_INDIRECT_BUFFERIdx;
        it = allBufferList.find(binderIdx);
        if(it == allBufferList.end())
        {
            //printf("error 45: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            DBG_LOG("error: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            exit(1);
        }
    }
    else if(target == GL_COPY_WRITE_BUFFER)
    {
        binderNo = curGL_COPY_WRITE_BUFFERNo;
        binderIdx = curGL_COPY_WRITE_BUFFERIdx;
        it = allBufferList.find(binderIdx);
        if(it == allBufferList.end())
        {
            //printf("error 45: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            DBG_LOG("error: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            exit(1);
        }
    }
    else if(target >= GL_UNIFORM_BUFFER && target < GL_UNIFORM_BUFFER + MAX_GL_UNIFORM_BUFFER_NUM)
    {
        binderNo = curGL_UNIFORM_BUFFER0No[target - GL_UNIFORM_BUFFER];
        binderIdx = curGL_UNIFORM_BUFFER0Idx[target - GL_UNIFORM_BUFFER];
        it = allBufferList.find(binderIdx);
        if(it == allBufferList.end())
        {
            //printf("error 46: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            DBG_LOG("error: no %d buffer in the bufferList in %d\n", binderIdx, callNo);
            exit(1);
        }
    }
    else
    {
        //printf("error 47: glunmapbuffer not support parameter!\n");
        DBG_LOG("error: glunmapbuffer not support parameter!\n");
    }
    if(target == GL_ARRAY_BUFFER /*&& it->second.glMapBufferRange_GL_ARRAY_BUFFEREnd == true*/)//for XCOMEEnemyWithin_1.2.0 no mapbuffer before unmapbuffer
    {
        glMapBufferRangeNoListIdx = it->second.glCopyClientSideBuffer_GL_ARRAY_BUFFERIdx;
        it->second.glMapBufferRange_GL_ARRAY_BUFFEREnd = false;
    }
    else if(target == GL_ELEMENT_ARRAY_BUFFER /*&& it->second.glMapBufferRange_GL_ELEMENT_ARRAY_BUFFEREnd == true*/)//for XCOMEEnemyWithin_1.2.0 no mapbuffer before unmapbuffer
    {
        glMapBufferRangeNoListIdx = it->second.glCopyClientSideBuffer_GL_ELEMENT_ARRAY_BUFFERIdx;
        it->second.glMapBufferRange_GL_ELEMENT_ARRAY_BUFFEREnd = false;
    }
    else if(target == GL_PIXEL_UNPACK_BUFFER && it->second.glMapBufferRange_GL_PIXEL_UNPACK_BUFFEREnd == true)
    {
        glMapBufferRangeNoListIdx = it->second.glCopyClientSideBuffer_GL_PIXEL_UNPACK_BUFFERIdx;
        it->second.glMapBufferRange_GL_PIXEL_UNPACK_BUFFEREnd = false;
    }
    else if(target == GL_DRAW_INDIRECT_BUFFER && it->second.glMapBufferRange_GL_DRAW_INDIRECT_BUFFEREnd == true)
    {
        glMapBufferRangeNoListIdx = it->second.glCopyClientSideBuffer_GL_DRAW_INDIRECT_BUFFERIdx;
        it->second.glMapBufferRange_GL_DRAW_INDIRECT_BUFFEREnd = false;
    }
    else if(target == GL_COPY_WRITE_BUFFER && it->second.glMapBufferRange_GL_COPY_WRITE_BUFFEREnd == true)
    {
        glMapBufferRangeNoListIdx = it->second.glCopyClientSideBuffer_GL_COPY_WRITE_BUFFERIdx;
        it->second.glMapBufferRange_GL_COPY_WRITE_BUFFEREnd = false;
    }
    else if((target >= GL_UNIFORM_BUFFER && target<GL_UNIFORM_BUFFER+MAX_GL_UNIFORM_BUFFER_NUM) && it->second.glMapBufferRange_GL_UNIFORM_BUFFEREnd == true)
    {
        glMapBufferRangeNoListIdx = it->second.glCopyClientSideBuffer_GL_UNIFORM_BUFFERIdx;
        it->second.glMapBufferRange_GL_UNIFORM_BUFFEREnd = false;
    }
    else
    {
        //printf("error 48: glunmapbuffer not support parameter!\n");
        DBG_LOG("error: glunmapbuffer not support parameter!\n");
    }
    if(binderNo == 0 && binderIdx == 0 && glMapBufferRangeNoListIdx == 0)
    {
        //nothing
    }
    else
    {
        map<unsigned int, bufferContext::mapBuffer>::iterator potr = it->second.glMapBufferRangeNoList.find(glMapBufferRangeNoListIdx);
        if(potr!=it->second.glMapBufferRangeNoList.end())
        {
            potr->second.glUnmapBufferNo = callNo;
        }
        else
        {
            it->second.glBufferDataNoList.push_front({callNo, binderNo});
            //printf("error 49:no mapbuffer before unmapbuffer in %d\n", callNo);
            //DBG_LOG("error:no mapbuffer before unmapbuffer in %d\n", callNo);
        }
    }
}

void bufferState::ff_glVertexAttribPointerForVao(unsigned int LocationIndex, unsigned int callNo, unsigned int clientSideBufferIndex, unsigned int bufferIndex, unsigned int bufferNo)
{
    if(curVertexArrayObjectIdx != 0)
    {
        map<unsigned int, vertexArrayContext>::iterator it =  allVaoList.find(curVertexArrayObjectIdx);
        if(it == allVaoList.end())
        {
            //printf("error 50: no genvertexarray before binding %d\n", callNo);
            DBG_LOG("error: no genvertexarray before binding %d\n", callNo);
            exit(1);
        }
        map<unsigned int, vertexArrayContext::withPointerBuffer>::iterator attrib = it->second.glVertexAttribPointerNoList.find(LocationIndex);
        vertexArrayContext::withPointerBuffer curBinder = {curVertexArrayObjectNo, callNo, clientSideBufferIndex, bufferIndex, bufferNo};
        if(attrib == it->second.glVertexAttribPointerNoList.end())
        {
            it->second.glVertexAttribPointerNoList.insert(pair<unsigned int, vertexArrayContext::withPointerBuffer>(LocationIndex, curBinder));
        }
        else
        {
            attrib->second = curBinder;
        }
    }//if curvertexarrayobject != 0
}

void bufferState::ff_glEnableVertexAttribArrayForVao(unsigned int LocationIndex, unsigned int callNo)
{
    if(curVertexArrayObjectIdx != 0)
    {
        map<unsigned int, vertexArrayContext>::iterator it =  allVaoList.find(curVertexArrayObjectIdx);
        if(it == allVaoList.end())
        {
            //printf("error 51:no genvertexarray before binding %d\n", callNo);
            DBG_LOG("error:no genvertexarray before binding %d\n", callNo);
            exit(1);
        }
        vertexArrayContext::withBinder newBinder = {curVertexArrayObjectNo, callNo, 1, 0};
        map<unsigned int, vertexArrayContext::withBinder>::iterator attrib = it->second.glEnableVertexAttribArrayNoList.find(LocationIndex);
        if(attrib == it->second.glEnableVertexAttribArrayNoList.end())
        {
            it->second.glEnableVertexAttribArrayNoList.insert(pair<unsigned int, vertexArrayContext::withBinder>(LocationIndex, newBinder));
        }
        else
        {
            attrib->second = newBinder;
        }
    }//if curvertexarrayobject != 0
}

void bufferState::debug(unsigned int callNo)
{
    map<unsigned int, bufferContext>::iterator it = allBufferList.find(47);
    if(it!=allBufferList.end())
    {
        unsigned int kkk = it->second.glBufferDataNoList.size();
        printf("~~~~ %d   callNo %d\n",kkk,callNo);
printf("bug   %d %d %d %d %d\n",it->second.glGenBuffersNo,it->second.glBufferDataNoList.front().thisCallNo,it->second.glBufferDataNoList.front().binderCallNo,
it->second.glBufferDataNoList.back().thisCallNo,it->second.glBufferDataNoList.back().binderCallNo);

    }
}

void bufferState::clear()
{
    curGL_SHADER_STORAGE_BUFFER1Idx = 0;
    curGL_SHADER_STORAGE_BUFFER1No = 0;
    for(unsigned int i=0; i<MAX_GL_UNIFORM_BUFFER_NUM; i++)
    {
        curGL_UNIFORM_BUFFER0Idx[i] = 0;
        curGL_UNIFORM_BUFFER0No[i] = 0;
    }
    for(unsigned int i=0; i<MAX_GL_SHADER_STORAGE_BUFFER_NUM; i++)
    {
        curGL_SHADER_STORAGE_BUFFER0Idx[i] = 0;
        curGL_SHADER_STORAGE_BUFFER0No[i] = 0;
    }
    //for mapbuffer
    glMapBufferRange_GL_ARRAY_BUFFERNo = {0, 0, 0};
    glMapBufferRange_GL_ELEMENT_ARRAY_BUFFERNo = {0, 0, 0};
    glMapBufferRange_GL_PIXEL_UNPACK_BUFFERNo = {0, 0, 0};
    glCopyClientSideBuffer_GL_ARRAY_BUFFERIdx = 0;
    glCopyClientSideBuffer_GL_ELEMENT_ARRAY_BUFFERIdx = 0;
    glCopyClientSideBuffer_GL_PIXEL_UNPACK_BUFFERIdx = 0;
    glMapBufferRange_GL_ARRAY_BUFFEREnd = 0;
    glMapBufferRange_GL_ELEMENT_ARRAY_BUFFEREnd = 0;
    glMapBufferRange_GL_PIXEL_UNPACK_BUFFEREnd = 0;
    //for mapbuffer
    curGL_ARRAY_BUFFERIdx = 0;
    curGL_ARRAY_BUFFERNo = 0;
    curGL_ELEMENT_ARRAY_BUFFERIdx = 0;
    curGL_ELEMENT_ARRAY_BUFFERNo = 0;
    curGL_PIXEL_UNPACK_BUFFERIdx = 0;
    curGL_PIXEL_UNPACK_BUFFERNo = 0;
    curGL_COPY_WRITE_BUFFERIdx = 0;
    curGL_COPY_WRITE_BUFFERNo = 0;
    curGL_DRAW_INDIRECT_BUFFERIdx = 0;
    curGL_DRAW_INDIRECT_BUFFERNo = 0;
    curGL_ATOMIC_COUNTER_BUFFERIdx = 0;
    curGL_ATOMIC_COUNTER_BUFFERNo = 0;
    curVertexArrayObjectNo = 0;
    curVertexArrayObjectIdx = DEFAULT_VAO_INDEX;
    allBufferList.clear();
    GlobalGenIndexList.clear();
    GlobalDeleteIndexList.clear();
    allVaoList.clear();
    vertexArrayContext newVao;
    newVao.glGenVertexArraysNo = 0;
    allVaoList.insert(pair<unsigned int, vertexArrayContext>(DEFAULT_VAO_INDEX, newVao));
}

void clientSideBufferState::ff_glClientSideBufferData(unsigned int index, unsigned int callNo)
{
    map<unsigned int, unsigned long long>::iterator it = glClientSideBuffDataNoList->find(index);
    if(it == glClientSideBuffDataNoList->end())
    {
        glClientSideBuffDataNoList->insert(pair<unsigned int, unsigned long long>(index, callNo));
    }
    else
    {
        unsigned long long callNo2 = callNo;
        it->second = (it->second % FIRST_10_BIT) + callNo2 * FIRST_10_BIT;
    }
}

unsigned long long clientSideBufferState::findAClientSideBufferState(unsigned int name)
{
    map<unsigned int, unsigned long long>::iterator it = glClientSideBuffDataNoList->find(name);
    if(it != glClientSideBuffDataNoList->end())
    {
        return it->second;
    }
    else
    {
     //   printf("can't find a ClientSideBufferState No %d!\n", name);//for chinese ghost story
        return 0;
    }
}

void clientSideBufferState::readClientSideBufferState(vector<unsigned int> &clientSideBufferList, unordered_set<unsigned int> &callNoList)
{
    for(unsigned int i=0; i<clientSideBufferList.size(); i++)
    {
        if(clientSideBufferList[i] == NO_IDX)
        {
            continue;//not clientSideBuffer data
        }
        map<unsigned int, unsigned long long>::iterator it = glClientSideBuffDataNoList->find(clientSideBufferList[i]);
        if(it == glClientSideBuffDataNoList->end())
        {
            //printf("error 52: no idx in clientSideBuffer! %d\n", clientSideBufferList[i]);
            DBG_LOG("error: no idx in clientSideBuffer! %d\n", clientSideBufferList[i]);
        }
        else
        {
            callNoList.insert(it->second%FIRST_10_BIT);
            callNoList.insert(it->second/FIRST_10_BIT);
        }
    }
}
void clientSideBufferState::clear()
{
    glClientSideBuffDataNoList->clear();
}

void globalState::ff_glFrontFace(unsigned int type, unsigned int callNo)
{
    glFrontFaceNo = callNo;
}

void globalState::ff_glEnableDisable(unsigned int type, unsigned int callNo)
{
    if(type == GL_BLEND)//3042)//GL_BLEND
    {
        glEnableNo.gl_blendNo = callNo;
    }
    else if(type == GL_CULL_FACE)//2884)//GL_CULL_FACE
    {
        glEnableNo.gl_cull_faceNo = callNo;
    }
    else if(type == GL_DEPTH_TEST)//2929)//GL_DEPTH_TEST
    {
        glEnableNo.gl_depth_testNo = callNo;
    }
    else if(type == GL_DITHER)//3024)//GL_DITHER
    {
        glEnableNo.gl_ditherNo = callNo;
    }
    else if(type == GL_POLYGON_OFFSET_FILL)//32823)//GL_POLYGON_OFFSET_FILL
    {
        glEnableNo.gl_polygon_offset_fillNo = callNo;
    }
    else if(type == GL_PRIMITIVE_RESTART_FIXED_INDEX)//36201)//GL_PRIMITIVE_RESTART_FIXED_INDEX
    {
        glEnableNo.gl_primitive_restart_fixed_indexNo = callNo;
    }
    else if(type == GL_RASTERIZER_DISCARD)//35977)//GL_RASTERIZER_DISCARD
    {
        glEnableNo.gl_rasterizer_discardNo = callNo;
    }
    else if(type == GL_SAMPLE_ALPHA_TO_COVERAGE)//32926)//GL_SAMPLE_ALPHA_TO_COVERAGE
    {
        glEnableNo.gl_sample_alpha_to_coverageNo = callNo;
    }
    else if(type == GL_SAMPLE_COVERAGE)//32928)//GL_SAMPLE_COVERAGE
    {
        glEnableNo.gl_sample_coverageNo = callNo;
    }
    else if(type == GL_SCISSOR_TEST)//3089)//GL_SCISSOR_TEST
    {
        glEnableNo.gl_scissor_testNo = callNo;
    }
    else if(type == GL_STENCIL_TEST)//2960)//GL_STENCIL_TEST
    {
        glEnableNo.gl_stencil_testNo = callNo;
    }
}

void globalState::ff_glCullFace(unsigned int type, unsigned int callNo)
{
    glCullFaceNo = callNo;
}

void globalState::ff_glDepthMask(unsigned int type, unsigned int callNo)
{
    glDepthMaskNo = callNo;
}

void globalState::ff_glDepthFunc(unsigned int type, unsigned int callNo)
{
    glDepthFuncNo = callNo;
}

void globalState::ff_glBlendFunc(unsigned int type, unsigned int callNo)
{
    glBlendFuncNo = callNo;
}

void globalState::ff_glBlendEquation(unsigned int type, unsigned int callNo)
{
    glBlendEquationNo = callNo;
}

void globalState::ff_glViewport(unsigned int callNo)
{
    glViewportNo = callNo;
}

void globalState::ff_glFinish(unsigned int callNo)
{
    glFinishNo = callNo;
}

void globalState::ff_glFlush(unsigned int callNo)
{
    glFlushNo = callNo;
}

void globalState::ff_glScissor(unsigned int callNo)
{
    glScissorNo = callNo;
}

void globalState::ff_glClearColor(unsigned int callNo)
{
    glClearColorNo = callNo;
}

void globalState::ff_glClearDepthf(unsigned int callNo)
{
    glClearDepthfNo = callNo;
}

void globalState::ff_glBlendFuncSeparate(unsigned int callNo)
{
    glBlendFuncSeparateNo = callNo;
}

void globalState::ff_glClear(unsigned int mask, unsigned int callNo, bool frameBuffer, unsigned int drawframebuffer)
{
    glClearNo.frameBuffer = frameBuffer;

    if((mask&GL_COLOR_BUFFER_BIT) != 0)
    {
        glClearNo.gl_color_buffer_bitNo = callNo;
        glClearNo.gl_color_buffer_bitBinderNo = drawframebuffer;
        glClearNo.glColorMaskBinderNo = glColorMaskNo;
        glClearNo.glClearColorNo = glClearColorNo;
    }
    if((mask&GL_DEPTH_BUFFER_BIT) != 0)
    {
        glClearNo.gl_depth_buffer_bitNo = callNo;
        glClearNo.gl_depth_buffer_bitBinderNo = drawframebuffer;
        glClearNo.glDepthMaskBinderNo = glDepthMaskNo;
    }
    if((mask&GL_STENCIL_BUFFER_BIT) != 0)
    {
        glClearNo.gl_stencil_buffer_bitNo = callNo;
        glClearNo.gl_stencil_buffer_bitBinderNo = drawframebuffer;
        glClearNo.glStencilMaskBinderNo = glStencilMaskNo;
    }
    glClearNo.glScissorBinderNo = glScissorNo;
}
void globalState::debug(unsigned int callNo)
{
    printf("callNo %d\n", callNo);
    map<unsigned int, withPointerBuffer>::iterator attrib = glVertexAttribPointerNoList.begin();
    printf("````````````````\n");
    while(attrib != glVertexAttribPointerNoList.end())
    {
        printf("a %d b %d c %d d %d\n", attrib->second.thisCallNo, attrib->second.clientSideBufferIdx, attrib->second.bufferIdx, attrib->second.bufferNo);
        attrib++;
    }
}

void globalState::ff_glEnableVertexAttribArray(unsigned int LocationIndex, unsigned int callNo)
{
    glEnableVertexAttribArrayList.insert(LocationIndex);
    map<unsigned int, unsigned int>::iterator attrib = glEnableVertexAttribArrayNoList.find(LocationIndex);

    if(attrib == glEnableVertexAttribArrayNoList.end())
    {
        glEnableVertexAttribArrayNoList.insert(pair<unsigned int, unsigned int>(LocationIndex, callNo));
    }
    else
    {
        attrib->second = callNo;
    }
}

void globalState::ff_glDisableVertexAttribArray(unsigned int LocationIndex, unsigned int callNo)
{
    set<unsigned int>::iterator it = glEnableVertexAttribArrayList.find(LocationIndex);
    if(it != glEnableVertexAttribArrayList.end())glEnableVertexAttribArrayList.erase(LocationIndex);
    map<unsigned int, unsigned int>::iterator attrib = glDisableVertexAttribArrayNoList.find(LocationIndex);
    if(attrib == glDisableVertexAttribArrayNoList.end())
    {
        glDisableVertexAttribArrayNoList.insert(pair<unsigned int, unsigned int>(LocationIndex, callNo));
    }
    else
    {
        attrib->second = callNo;
    }
}

void globalState::ff_glVertexAttribPointer(unsigned int LocationIndex, unsigned int callNo, unsigned int clientSideBufferIndex, unsigned int bufferIndex, unsigned int bufferNo)
{
    map<unsigned int, withPointerBuffer>::iterator attrib = glVertexAttribPointerNoList.find(LocationIndex);
    withPointerBuffer curBinder = {callNo, clientSideBufferIndex, bufferIndex, bufferNo};
    if(attrib == glVertexAttribPointerNoList.end())
    {
        glVertexAttribPointerNoList.insert(pair<unsigned int, withPointerBuffer>(LocationIndex, curBinder));
    }
    else
    {
        attrib->second = curBinder;
    }
}

void globalState::ff_glVertexAttribDivisor(unsigned int LocationIndex, unsigned int callNo)
{
    map<unsigned int, unsigned int>::iterator attrib = glVertexAttribDivisorList.find(LocationIndex);
    if(attrib == glVertexAttribDivisorList.end())
    {
        glVertexAttribDivisorList.insert(pair<unsigned int, unsigned int>(LocationIndex, callNo));
    }
    else
    {
        attrib->second = callNo;
    }
}

void globalState::ff_glVertexAttrib_Types(unsigned int LocationIndex, unsigned int callNo)
{
    map<unsigned int, unsigned int>::iterator attrib = glVertexAttribNoList.find(LocationIndex);
    if(attrib == glVertexAttribNoList.end())
    {
        glVertexAttribNoList.insert(pair<unsigned int, unsigned int>(LocationIndex, callNo));
    }
    else
    {
        attrib->second = callNo;
    }
}

void globalState::ff_glColorMask(unsigned int callNo)
{
    glColorMaskNo = callNo;
}

void globalState::ff_glBlendColor(unsigned int callNo)
{
    glBlendColorNo = callNo;
}

void globalState::ff_glStencilOpSeparate(unsigned int type, unsigned int callNo)
{
    if(type == GL_FRONT)
    {
        glStencilOpSeparateNo.gl_frontNo = callNo;
    }
    else if(type == GL_BACK)
    {
        glStencilOpSeparateNo.gl_backNo = callNo;
    }
    else if(type == GL_FRONT_AND_BACK)
    {
        glStencilOpSeparateNo.gl_backNo = callNo;
        glStencilOpSeparateNo.gl_frontNo = callNo;
    }
    else
    {
        //printf("unknown error\n");
        DBG_LOG("unknown error\n");
        exit(1);
    }
}

void globalState::ff_glStencilMask(unsigned int callNo)
{
    glStencilMaskNo = callNo;
}

void globalState::ff_glStencilFuncSeparate(unsigned int type, unsigned int callNo)
{
    if(type == GL_FRONT)
    {
        glStencilFuncSeparateNo.gl_frontNo = callNo;
    }
    else if(type == GL_BACK)
    {
        glStencilFuncSeparateNo.gl_backNo = callNo;
    }
    else if(type == GL_FRONT_AND_BACK)
    {
        glStencilFuncSeparateNo.gl_backNo = callNo;
        glStencilFuncSeparateNo.gl_frontNo = callNo;
    }
    else
    {
        //printf("unknown error\n");
        DBG_LOG("unknown error\n");
        exit(1);
    }
}

void globalState::ff_glPolygonOffset(unsigned int callNo)
{
    glPolygonOffsetNo = callNo;
}

void globalState::ff_glClearStencil(unsigned int callNo)
{
    glClearStencilNo = callNo;
   // glClearStencilNo.glStencilMaskBinderNo = glStencilMaskNo;
}

void globalState::ff_glBlendEquationSeparate(unsigned int callNo)
{
    glBlendEquationSeparateNo = callNo;
}

void globalState::ff_glHint(unsigned int target, unsigned int callNo)
{
    if(target == GL_FRAGMENT_SHADER_DERIVATIVE_HINT)
    {
        glHintNo.gl_fragment_shader_derivative_hintNo = callNo;
    }
    if(target == GL_GENERATE_MIPMAP_HINT)
    {
        glHintNo.gl_generate_mipmap_hintNo = callNo;
    }
}

void globalState::ff_glLineWidth(unsigned int callNo)
{
    glLineWidthNo = callNo;
}

void globalState::ff_glStencilMaskSeparate(unsigned int face, unsigned int callNo)
{
    if(face == GL_FRONT)
    {
        glStencilMaskSeparateNo.gl_frontNo = callNo;
    }
    else if(face == GL_BACK)
    {
        glStencilMaskSeparateNo.gl_backNo = callNo;
    }
    else if(face == GL_FRONT_AND_BACK)
    {
        glStencilMaskSeparateNo.gl_backNo = callNo;
        glStencilMaskSeparateNo.gl_frontNo = callNo;
    }
    else
    {
        //printf("unknown error\n");
        DBG_LOG("unknown error\n");
        exit(1);
    }
}

void globalState::ff_glDepthRangef(unsigned int callNo)
{
    glDepthRangefNo = callNo;
}

void globalState::ff_glInvalidateFramebuffer(unsigned int target, unsigned int num, unsigned int attachment[], unsigned int callNo)
{
    if(target == GL_READ_FRAMEBUFFER)
    {
        for(unsigned int i=0; i<num; i++)
        {
            if(attachment[i] == GL_COLOR_ATTACHMENT0)
            {
                glInvalidateFramebufferNo.GL_READ_FRAMEBFUFFER_GL_COLOR_ATTACHMENT0 = callNo;
            }
            else if(attachment[i] == GL_DEPTH_ATTACHMENT)
            {
                glInvalidateFramebufferNo.GL_READ_FRAMEBFUFFER_GL_DEPTH_ATTACHMENT = callNo;
            }
            else if(attachment[i] == GL_STENCIL_ATTACHMENT)
            {
                glInvalidateFramebufferNo.GL_READ_FRAMEBFUFFER_GL_STENCIL_ATTACHMENT = callNo;
            }
            else if(attachment[i] == GL_DEPTH_STENCIL_ATTACHMENT)
            {
                glInvalidateFramebufferNo.GL_READ_FRAMEBFUFFER_GL_DEPTH_ATTACHMENT = callNo;
                glInvalidateFramebufferNo.GL_READ_FRAMEBFUFFER_GL_STENCIL_ATTACHMENT = callNo;
            }
            else if(attachment[i] == GL_COLOR)
            {
                glInvalidateFramebufferNo.GL_READ_FRAMEBFUFFER_GL_COLOR = callNo;
            }
            else if(attachment[i] == GL_DEPTH)
            {
                glInvalidateFramebufferNo.GL_READ_FRAMEBFUFFER_GL_DEPTH = callNo;
            }
            else if(attachment[i] == GL_STENCIL)
            {
                glInvalidateFramebufferNo.GL_READ_FRAMEBFUFFER_GL_STENCIL = callNo;
            }
        }//...for
    }
    else if(target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER)
    {
        for(unsigned int i=0;i<num;i++)
        {
            if(attachment[i] == GL_COLOR_ATTACHMENT0)
            {
                glInvalidateFramebufferNo.GL_FRAMEBFUFFER_GL_COLOR_ATTACHMENT0 = callNo;
            }
            else if(attachment[i] == GL_DEPTH_ATTACHMENT)
            {
                glInvalidateFramebufferNo.GL_FRAMEBFUFFER_GL_DEPTH_ATTACHMENT = callNo;
            }
            else if(attachment[i] == GL_STENCIL_ATTACHMENT)
            {
                glInvalidateFramebufferNo.GL_FRAMEBFUFFER_GL_STENCIL_ATTACHMENT = callNo;
            }
            else if(attachment[i] == GL_DEPTH_STENCIL_ATTACHMENT)
            {
                glInvalidateFramebufferNo.GL_FRAMEBFUFFER_GL_DEPTH_ATTACHMENT = callNo;
                glInvalidateFramebufferNo.GL_READ_FRAMEBFUFFER_GL_STENCIL_ATTACHMENT = callNo;
            }
            else if(attachment[i] == GL_COLOR)
            {
                glInvalidateFramebufferNo.GL_FRAMEBFUFFER_GL_COLOR = callNo;
            }
            else if(attachment[i] == GL_DEPTH)
            {
                glInvalidateFramebufferNo.GL_FRAMEBFUFFER_GL_DEPTH = callNo;
            }
            else if(attachment[i] == GL_STENCIL)
            {
                glInvalidateFramebufferNo.GL_FRAMEBFUFFER_GL_STENCIL = callNo;
            }
        }//...for
    }
    else
    {
        //printf("error unknown!\n");
        DBG_LOG("unknown error\n");
        exit(1);
    }
}

void globalState::ff_glSampleCoverage(unsigned int callNo)
{
    glSampleCoverageNo = callNo;
}

void globalState::ff_glPatchParameteri(unsigned int callNo)
{
    glPatchParameteriNo = callNo;
}

void globalState::ff_glDrawBuffers(unsigned int callNo)
{
    glDrawBuffersNo = callNo;
}

void globalState::readCurGlobalStateForFrameBufferClear(unordered_set<unsigned int> &callNoList)
{
    callNoList.insert(glClearNo.gl_color_buffer_bitNo);
    callNoList.insert(glClearNo.gl_color_buffer_bitBinderNo);
    callNoList.insert(glClearNo.gl_depth_buffer_bitNo);
    callNoList.insert(glClearNo.gl_depth_buffer_bitBinderNo);
    callNoList.insert(glClearNo.gl_stencil_buffer_bitNo);
    callNoList.insert(glClearNo.gl_stencil_buffer_bitBinderNo);
    callNoList.insert(glClearNo.glColorMaskBinderNo);
    callNoList.insert(glClearNo.glClearColorNo);
    callNoList.insert(glClearNo.glDepthMaskBinderNo);
    callNoList.insert(glClearNo.glStencilMaskBinderNo);
    callNoList.insert(glClearNo.glScissorBinderNo);
    callNoList.insert(glStencilMaskNo);
    callNoList.insert(glDepthMaskNo);
    callNoList.insert(glColorMaskNo);
    callNoList.insert(glClearDepthfNo);
    callNoList.insert(glClearStencilNo);
    callNoList.insert(glClearColorNo);
    callNoList.insert(glDrawBuffersNo);
    callNoList.insert(glEnableNo.gl_scissor_testNo);
}

void globalState::readCurGlobalState(vector<unsigned int> &clientSideBufferList, vector<unsigned int> &bufferList, unordered_set<unsigned int> &callNoList, bool framebuffer)
{
    callNoList.insert(glFrontFaceNo);
    callNoList.insert(glEnableNo.gl_blendNo);
    callNoList.insert(glEnableNo.gl_cull_faceNo);
    callNoList.insert(glEnableNo.gl_debug_outputNo);
    callNoList.insert(glEnableNo.gl_debug_output_synchronousNo);
    callNoList.insert(glEnableNo.gl_depth_testNo);
    callNoList.insert(glEnableNo.gl_ditherNo);
    callNoList.insert(glEnableNo.gl_polygon_offset_fillNo);
    callNoList.insert(glEnableNo.gl_primitive_restart_fixed_indexNo);
    callNoList.insert(glEnableNo.gl_rasterizer_discardNo);
    callNoList.insert(glEnableNo.gl_sample_alpha_to_coverageNo);
    callNoList.insert(glEnableNo.gl_sample_coverageNo);
    callNoList.insert(glEnableNo.gl_sample_maskNo);
    callNoList.insert(glEnableNo.gl_scissor_testNo);
    callNoList.insert(glEnableNo.gl_stencil_testNo);
    callNoList.insert(glCullFaceNo);
    callNoList.insert(glDepthMaskNo);
    callNoList.insert(glDepthFuncNo);
    callNoList.insert(glBlendFuncNo);
    callNoList.insert(glBlendEquationNo);
    callNoList.insert(glViewportNo);
    callNoList.insert(glFinishNo);
    callNoList.insert(glFlushNo);
    callNoList.insert(glScissorNo);
    callNoList.insert(glClearColorNo);
    if(glClearNo.frameBuffer==false)
    {
        callNoList.insert(glClearNo.gl_color_buffer_bitNo);
        callNoList.insert(glClearNo.gl_color_buffer_bitBinderNo);
        callNoList.insert(glClearNo.gl_depth_buffer_bitNo);
        callNoList.insert(glClearNo.gl_depth_buffer_bitBinderNo);
        callNoList.insert(glClearNo.gl_stencil_buffer_bitNo);
        callNoList.insert(glClearNo.gl_stencil_buffer_bitBinderNo);
        callNoList.insert(glClearNo.glColorMaskBinderNo);
        callNoList.insert(glClearNo.glClearColorNo);
        callNoList.insert(glClearNo.glDepthMaskBinderNo);
        callNoList.insert(glClearNo.glStencilMaskBinderNo);
        callNoList.insert(glClearNo.glScissorBinderNo);
    }
    callNoList.insert(glBlendFuncSeparateNo);
    callNoList.insert(glClearDepthfNo);
    callNoList.insert(glColorMaskNo);
    callNoList.insert(glBlendColorNo);
    callNoList.insert(glStencilOpSeparateNo.gl_frontNo);
    callNoList.insert(glStencilOpSeparateNo.gl_backNo);
    callNoList.insert(glStencilMaskNo);
    callNoList.insert(glStencilFuncSeparateNo.gl_frontNo);
    callNoList.insert(glStencilFuncSeparateNo.gl_backNo);
    callNoList.insert(glPolygonOffsetNo);
    callNoList.insert(glClearStencilNo);
    callNoList.insert(glBlendEquationSeparateNo);

    //printf glEnableVertexAttribArrayNoList
    map<unsigned int, unsigned int>::iterator unit = glEnableVertexAttribArrayNoList.begin();
    while(unit != glEnableVertexAttribArrayNoList.end())
    {
        callNoList.insert(unit->second);
        unit++;
    }
    //printf glDisableVertexAttribArrayNoList
    unit = glDisableVertexAttribArrayNoList.begin();
    while(unit != glDisableVertexAttribArrayNoList.end())
    {
        callNoList.insert(unit->second);
        unit++;
    }
    map<unsigned int, withPointerBuffer>::iterator potr = glVertexAttribPointerNoList.begin();
    set<unsigned int>::iterator it2;
    while(potr != glVertexAttribPointerNoList.end())
    {
        it2 = glEnableVertexAttribArrayList.find(potr->first);
        if( it2 != glEnableVertexAttribArrayList.end())
        {
            callNoList.insert(potr->second.thisCallNo);
            callNoList.insert(potr->second.bufferNo);
            clientSideBufferList.push_back(potr->second.clientSideBufferIdx);//save the clientSideBufferData code
            bufferList.push_back(potr->second.bufferIdx);//save the bufferData code
        }
        potr++;
    }
    map<unsigned int, unsigned int>::iterator potr2 = glVertexAttribNoList.begin();
    while(potr2 != glVertexAttribNoList.end())
    {
        it2 = glEnableVertexAttribArrayList.find(potr2->first);
        if( it2 == glEnableVertexAttribArrayList.end())
        {
            callNoList.insert(potr2->second);
        }
        potr2++;
    }
    callNoList.insert(glHintNo.gl_fragment_shader_derivative_hintNo);
    callNoList.insert(glHintNo.gl_generate_mipmap_hintNo);
    callNoList.insert(glLineWidthNo);
    callNoList.insert(glStencilMaskSeparateNo.gl_frontNo);
    callNoList.insert(glStencilMaskSeparateNo.gl_backNo);
    callNoList.insert(glDepthRangefNo);
    callNoList.insert(glInvalidateFramebufferNo.GL_READ_FRAMEBFUFFER_GL_COLOR_ATTACHMENT0);
    callNoList.insert(glInvalidateFramebufferNo.GL_READ_FRAMEBFUFFER_GL_DEPTH_ATTACHMENT);
    callNoList.insert(glInvalidateFramebufferNo.GL_READ_FRAMEBFUFFER_GL_STENCIL_ATTACHMENT);
    callNoList.insert(glInvalidateFramebufferNo.GL_READ_FRAMEBFUFFER_GL_COLOR);
    callNoList.insert(glInvalidateFramebufferNo.GL_READ_FRAMEBFUFFER_GL_DEPTH);
    callNoList.insert(glInvalidateFramebufferNo.GL_READ_FRAMEBFUFFER_GL_STENCIL);
    callNoList.insert(glInvalidateFramebufferNo.GL_FRAMEBFUFFER_GL_COLOR_ATTACHMENT0);
    callNoList.insert(glInvalidateFramebufferNo.GL_FRAMEBFUFFER_GL_DEPTH_ATTACHMENT);
    callNoList.insert(glInvalidateFramebufferNo.GL_FRAMEBFUFFER_GL_STENCIL_ATTACHMENT);
    callNoList.insert(glInvalidateFramebufferNo.GL_FRAMEBFUFFER_GL_COLOR);
    callNoList.insert(glInvalidateFramebufferNo.GL_FRAMEBFUFFER_GL_DEPTH);
    callNoList.insert(glInvalidateFramebufferNo.GL_FRAMEBFUFFER_GL_STENCIL);
    callNoList.insert(glSampleCoverageNo);
    callNoList.insert(glDrawBuffersNo);
    callNoList.insert(glPatchParameteriNo);
    map<unsigned int, unsigned int>::iterator potr3 = glVertexAttribDivisorList.begin();
    while(potr3 != glVertexAttribDivisorList.end())
    {
        callNoList.insert(potr3->second);
        potr3++;
    }
}

void globalState::clear()
{
    TransformFeedbackSwitch = 0;
    computeMemoryBarrier = 0;
    glFrontFaceNo = 0;
    glEnableNo = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    glCullFaceNo = 0;
    glDepthMaskNo = 0;
    glDepthFuncNo = 0;
    glBlendFuncNo = 0;
    glBlendEquationNo = 0;
    glViewportNo = 0;
    glFinishNo = 0;
    glFlushNo = 0;
    glScissorNo = 0;
    glClearColorNo = 0;
    glClearNo = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    glClearDepthfNo = 0;
    glColorMaskNo = 0;
    glBlendColorNo = 0;
    glBlendFuncSeparateNo = 0;
    glStencilOpSeparateNo = {0, 0};
    glStencilMaskNo = 0;
    glStencilFuncSeparateNo = {0, 0};
    glPolygonOffsetNo = 0;
    glClearStencilNo = 0;
    glBlendEquationSeparateNo = 0;
    glEnableVertexAttribArrayNoList.clear();
    glDisableVertexAttribArrayNoList.clear();
    glVertexAttribPointerNoList.clear();
    glVertexAttribNoList.clear();
    glHintNo = {0, 0};
    glLineWidthNo = 0;
    glStencilMaskSeparateNo = {0, 0};
    glDepthRangefNo = 0;
    glInvalidateFramebufferNo = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    glEnableVertexAttribArrayList.clear();//for save enable num indexv
    glVertexAttribDivisorList.clear();
    glSampleCoverageNo = 0;
    glDrawBuffersNo = 0;
}

void eglState::ff_eglCreateContext(unsigned int callNo)
{
    eglCreateContextNoList.push_back(callNo);
}

void eglState::ff_eglCreateWindowSurface(unsigned int callNo)
{
    eglCreateWindowSurfaceNoList.push_back(callNo);
}

void eglState::ff_eglMakeCurrent(unsigned int callNo)
{
    eglMakeCurrentNoList.push_back(callNo);
}

void eglState::ff_eglDestroySurface(unsigned int callNo)
{
    eglDestroySurfaceNoList.push_back(callNo);
}

void eglState::readCurEglState(unordered_set<unsigned int> &callNoList)
{
    for(unsigned int i=0; i<eglCreateContextNoList.size(); i++)
    {
        callNoList.insert(eglCreateContextNoList[i]);
    }
    for(unsigned int i=0; i<eglCreateWindowSurfaceNoList.size(); i++)
    {
        callNoList.insert(eglCreateWindowSurfaceNoList[i]);
    }
    for(unsigned int i=0; i<eglMakeCurrentNoList.size(); i++)
    {
        callNoList.insert(eglMakeCurrentNoList[i]);
    }
    for(unsigned int i=0; i<eglDestroySurfaceNoList.size(); i++)
    {
        callNoList.insert(eglDestroySurfaceNoList[i]);
    }
}
void eglState::clear()
{
    eglCreateContextNoList.clear();
    eglCreateWindowSurfaceNoList.clear();
    eglMakeCurrentNoList.clear();
    eglDestroySurfaceNoList.clear();
}

void renderBufferState::ff_glGenRenderbuffers(unsigned int index, unsigned int callNo)
{
    renderBufferContext newRenderBuffer;
    newRenderBuffer.glGenRenderbuffersNo = callNo;
    map<unsigned int, renderBufferContext>::iterator it = allRenderBufferList.find(index);
    if(it != allRenderBufferList.end())
    {
        allRenderBufferList.erase(it);
    }
    allRenderBufferList.insert(pair<unsigned int, renderBufferContext>(index, newRenderBuffer));
}

void renderBufferState::ff_glBindRenderbuffer(unsigned int index, unsigned int callNo)
{
    curRenderBufferIdx = index;
    curRenderBufferNo = callNo;
}

void renderBufferState::ff_glRenderbufferStorage(unsigned int callNo)
{
    map<unsigned int, renderBufferContext>::iterator it = allRenderBufferList.find(curRenderBufferIdx);
    if(it == allRenderBufferList.end())
    {
        //printf("error 53: not bind renderbuffer first\n");
        DBG_LOG("error: not bind renderbuffer first\n");
        exit(1);
    }
    it->second.glRenderbufferStorageNo.binderCallNo = curRenderBufferNo;
    it->second.glRenderbufferStorageNo.thisCallNo = callNo;
}

void renderBufferState::readCurRenderBufferState(unsigned int index, unordered_set<unsigned int> &callNoList)
{
    map<unsigned int, renderBufferContext>::iterator it = allRenderBufferList.find(index);
    callNoList.insert(it->second.glGenRenderbuffersNo);
    callNoList.insert(it->second.glRenderbufferStorageNo.thisCallNo);
    callNoList.insert(it->second.glRenderbufferStorageNo.binderCallNo);
}

void renderBufferState::clear()
{
    curRenderBufferIdx = 0;
    curRenderBufferNo = 0;
    allRenderBufferList.clear();
}

void frameBufferState::ff_glGenFramebuffers(unsigned int n, unsigned int index[], unsigned int callNo)
{
    for(unsigned int i=0; i<n; i++)
    {
        frameBufferContext newFrameBuffer;
        newFrameBuffer.glGenFramebuffersNo = callNo;
        map<unsigned int, frameBufferContext>::iterator it = allFrameBufferList.find(index[i]);
        if(it != allFrameBufferList.end())
        {
            allFrameBufferList.erase(it);
        }
        allFrameBufferList.insert(pair<unsigned int, frameBufferContext>(index[i], newFrameBuffer));
    }
}

void frameBufferState::ff_glBindFramebuffer(unsigned int index, unsigned int type, unsigned int callNo)
{
    if(type == GL_DRAW_FRAMEBUFFER || type == GL_FRAMEBUFFER)
    {
        curDrawFrameBufferIdx = index;
        curDrawFrameBufferNo = callNo;
    }
    else if(type == GL_READ_FRAMEBUFFER)
    {
        curReadFrameBufferIdx = index;
        curReadFrameBufferNo = callNo;
    }
    else
    {
        //printf("error 54: not this type in glBindFramebuffer %d \n", type);
        DBG_LOG("error: not this type in glBindFramebuffer %d \n", type);
        exit(1);
    }
}

void frameBufferState::ff_glFramebufferTexture2D(unsigned int type, unsigned int attachment, unsigned int textarget, unsigned int index, unsigned int callNo)
{
    //save for glFramebufferTexture2D GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
    if(textarget == GL_TEXTURE_2D||textarget == GL_TEXTURE_CUBE_MAP_POSITIVE_X)
    {
        //nothing
    }
    else if (textarget == GL_TEXTURE_CUBE_MAP_NEGATIVE_X)
    {
        index = index + CUBE_MAP_INDEX;
    }
    else if (textarget == GL_TEXTURE_CUBE_MAP_POSITIVE_Y)
    {
        index = index + CUBE_MAP_INDEX*2;
    }
    else if (textarget == GL_TEXTURE_CUBE_MAP_NEGATIVE_Y)
    {
        index = index + CUBE_MAP_INDEX*3;
    }
    else if (textarget == GL_TEXTURE_CUBE_MAP_POSITIVE_Z)
    {
        index = index + CUBE_MAP_INDEX*4;
    }
    else if (textarget == GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)
    {
        index = index + CUBE_MAP_INDEX*5;
    }
    else
    {
        //printf("error FrameTexture2D\n");
        DBG_LOG("error FrameTexture2D\n");
        exit(1);
    }
    map<unsigned int, frameBufferContext>::iterator it;
    unsigned int binderCallNo;
    if(type == GL_DRAW_FRAMEBUFFER || type == GL_FRAMEBUFFER)
    {
        it = allFrameBufferList.find(curDrawFrameBufferIdx);
        binderCallNo = curDrawFrameBufferNo;
    }
    else if(type == GL_READ_FRAMEBUFFER)
    {
        it = allFrameBufferList.find(curReadFrameBufferIdx);
        binderCallNo = curReadFrameBufferNo;
    }
    else
    {
        binderCallNo =0;
    }
    if(attachment == GL_COLOR_ATTACHMENT0)
    {
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0Idx = index;
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0Type = 0;//0 means texture..
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0No.thisCallNo = callNo;
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0No.binderCallNo = binderCallNo;
    }
    else if(attachment == GL_COLOR_ATTACHMENT1)
    {
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT1Idx = index;
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT1Type = 0;//0 means texture..
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT1No.thisCallNo = callNo;
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT1No.binderCallNo = binderCallNo;
    }
    else if(attachment == GL_COLOR_ATTACHMENT2)
    {
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT2Idx = index;
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT2Type = 0;//0 means texture..
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT2No.thisCallNo = callNo;
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT2No.binderCallNo = binderCallNo;
    }
    else if(attachment == GL_COLOR_ATTACHMENT3)
    {
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT3Idx = index;
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT3Type = 0;//0 means texture..
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT3No.thisCallNo = callNo;
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT3No.binderCallNo = binderCallNo;
    }
    else if(attachment == GL_DEPTH_ATTACHMENT)
    {
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTIdx = index;
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTType = 0;//0 means texture..
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTNo.thisCallNo = callNo;
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTNo.binderCallNo = binderCallNo;
    }
    else if(attachment == GL_STENCIL_ATTACHMENT)
    {
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTIdx = index;
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTType = 0;//0 means texture..
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTNo.thisCallNo = callNo;
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTNo.binderCallNo = binderCallNo;
    }
    else if(attachment == GL_DEPTH_STENCIL_ATTACHMENT)
    {
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTIdx = index;
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTType = 0;//0 means texture..
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTNo.thisCallNo = callNo;
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTNo.binderCallNo = binderCallNo;
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTIdx = index;
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTType = 0;//0 means texture..
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTNo.thisCallNo = callNo;
        it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTNo.binderCallNo = binderCallNo;
    }
    else
    {
    //printf("error 55: not this parameter in glFramebufferTexture2D in %d\n", callNo);
    DBG_LOG("error: not this parameter in glFramebufferTexture2D in %d\n", callNo);
    exit(1);
    }
}

void frameBufferState::ff_glFramebufferRenderbuffer(unsigned int type, unsigned int attachment, unsigned int index, unsigned int callNo)
{
    map<unsigned int, frameBufferContext>::iterator it;
    unsigned int binderCallNo;
    if(type == GL_DRAW_FRAMEBUFFER || type == GL_FRAMEBUFFER)
    {
        it = allFrameBufferList.find(curDrawFrameBufferIdx);
        binderCallNo = curDrawFrameBufferNo;
    }
    else if(type == GL_READ_FRAMEBUFFER)
    {
        it = allFrameBufferList.find(curReadFrameBufferIdx);
        binderCallNo = curReadFrameBufferNo;
    }
    else
    {
        binderCallNo = 0;
    }
    if(it != allFrameBufferList.end())
    {
        if(attachment == GL_COLOR_ATTACHMENT0)
        {
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0Idx = index;
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0Type = 1;//1 means renderbuffer..
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0No.thisCallNo = callNo;
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0No.binderCallNo = binderCallNo;
        }
        else if(attachment == GL_COLOR_ATTACHMENT1)
        {
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT1Idx = index;
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT1Type = 1;//1 means texture..
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT1No.thisCallNo = callNo;
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT1No.binderCallNo = binderCallNo;
        }
        else if(attachment == GL_COLOR_ATTACHMENT2)
        {
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT2Idx = index;
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT2Type = 1;//1 means texture..
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT2No.thisCallNo = callNo;
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT2No.binderCallNo = binderCallNo;
        }
        else if(attachment == GL_COLOR_ATTACHMENT3)
        {
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT3Idx = index;
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT3Type = 1;//1 means texture..
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT3No.thisCallNo = callNo;
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT3No.binderCallNo = binderCallNo;
        }
        else if(attachment == GL_DEPTH_ATTACHMENT)
        {
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTIdx = index;
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTType = 1;//1 means renderbuffer..
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTNo.thisCallNo = callNo;
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTNo.binderCallNo = binderCallNo;
        }
        else if(attachment == GL_STENCIL_ATTACHMENT)
        {
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTIdx = index;
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTType = 1;//1 means renderbuffer..
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTNo.thisCallNo = callNo;
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTNo.binderCallNo = binderCallNo;
        }
        else if(attachment == GL_DEPTH_STENCIL_ATTACHMENT)
        {
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTIdx = index;
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTType = 1;//1 means renderbuffer..
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTNo.thisCallNo = callNo;
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTNo.binderCallNo = binderCallNo;
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTIdx = index;
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTType = 1;//1 means renderbuffer..
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTNo.thisCallNo = callNo;
            it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTNo.binderCallNo = binderCallNo;
        }
        else
        {
            //printf("error 56: not this parameter in glFramebufferTexture2D\n");
            DBG_LOG("error: not this parameter in glFramebufferTexture2D\n");
            exit(1);
        }
    }
    else
    {
        //printf("no framebuffer before renderbuffer\n");
        DBG_LOG("no framebuffer before renderbuffer\n");
    }
}

void frameBufferState::ff_glDeleteFramebuffers(unsigned int n, unsigned int index[], unsigned int callNo, std::unordered_set<unsigned int> &callNoList)
{
    for(unsigned int i =0; i<n; i++)
    {
        unordered_set<unsigned int>::iterator it = curFrameBufferIdx.find(index[i]);
        if(it != curFrameBufferIdx.end())
        {
            curFrameBufferIdx.erase(index[i]);
            callNoList.insert(callNo);
        }
         if(index[i] == curDrawFrameBufferIdx || index[i] == curReadFrameBufferIdx )
        {
            if(index[i] == curDrawFrameBufferIdx)
            {
                curDrawFrameBufferIdx = 0;
                curDrawFrameBufferNo = 0;
            }
            if(index[i] == curReadFrameBufferIdx)
            {
                curReadFrameBufferIdx = 0;
                curReadFrameBufferNo = 0;
            }
        }
    }
}

void frameBufferState::ff_glDrawBuffers(unsigned int callNo)
{
    map<unsigned int, frameBufferContext>::iterator it;
    if(curDrawFrameBufferIdx != 0)
    {
        it = allFrameBufferList.find(curDrawFrameBufferIdx);
        if(it == allFrameBufferList.end())
        {
            //printf("error 57: no drawframebuffer in %d\n", callNo);
            DBG_LOG("error: no drawframebuffer in %d\n", callNo);
            exit(1);
        }
        it->second.glDrawBuffersNo = callNo;
    }
    else
    {
        //printf("error 58: no drawframebuffer in %d\n", callNo);
        //DBG_LOG("error: no drawframebuffer in %d\n", callNo);
    }
}

void frameBufferState::ff_glBlitFramebuffer(unsigned int callNo)
{
    map<unsigned int, frameBufferContext>::iterator it;
    it = allFrameBufferList.find(curDrawFrameBufferIdx);
    if(it == allFrameBufferList.end())
    {
        //printf("error 59: no drawframebuffer in %d\n", callNo);
        DBG_LOG("error: no drawframebuffer in %d\n", callNo);
        exit(1);
    }
    frameBufferContext::glBlitFramebufferState newBlitFramebuffer;
    newBlitFramebuffer.readFrameBufferIdx = curReadFrameBufferIdx;
    newBlitFramebuffer.readFrameBufferNo = curReadFrameBufferNo;
    newBlitFramebuffer.drawFrameBufferIdx = curDrawFrameBufferIdx;
    newBlitFramebuffer.drawFrameBufferNo = curDrawFrameBufferNo;
    newBlitFramebuffer.thisCallNo = callNo;
    it->second.glBlitFramebufferNoList.push_front(newBlitFramebuffer);
}

void frameBufferState::clearBlitFramebuffer()
{
    map<unsigned int, frameBufferContext>::iterator it;
    it = allFrameBufferList.find(curDrawFrameBufferIdx);
    if(it == allFrameBufferList.end())
    {
        //printf("error 60: no drawframebuffer\n");
        DBG_LOG("error: no drawframebuffer\n");
        exit(1);
    }
    it->second.glBlitFramebufferNoList.clear();
}

void frameBufferState::readCurFrameBufferState(vector<unsigned int> &textureList, unordered_set<unsigned int> &callNoList)
{
    map<unsigned int, frameBufferContext>::iterator it;
    list<frameBufferContext::glBlitFramebufferState>::iterator potr;
    if(curReadFrameBufferNo != 0)
    {
        callNoList.insert(curReadFrameBufferNo);
        if(curReadFrameBufferIdx != 0)
        {
            it = allFrameBufferList.find(curReadFrameBufferIdx);
            curFrameBufferIdx.insert(curReadFrameBufferIdx);//for gldeleteframebuffer
            callNoList.insert(it->second.glGenFramebuffersNo);
            callNoList.insert(it->second.glDrawBuffersNo);
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0No.thisCallNo);
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0No.binderCallNo);
            if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0Type == 0)//texture
            {
                textureList.push_back(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0Idx);
            }
            else if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0Type == 1)//renderbuffer
            {
                curRenderBufferState.readCurRenderBufferState(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0Idx, callNoList);
            }
            else
            {
                //printf("error: unknown error\n");
                DBG_LOG("error: unknown error\n");
            }
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT1No.thisCallNo);
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT1No.binderCallNo);
            if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT1Type == 0)//texture
            {
                textureList.push_back(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT1Idx);
            }
            else if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT1Type == 1)//renderbuffer
            {
                curRenderBufferState.readCurRenderBufferState(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT1Idx, callNoList);
            }
            else
            {
                //printf("error: unknown error\n");
                DBG_LOG("error: unknown error\n");
            }
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT2No.thisCallNo);
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT2No.binderCallNo);
            if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT2Type == 0)//texture
            {
                textureList.push_back(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT2Idx);
            }
            else if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT2Type == 1)//renderbuffer
            {
                curRenderBufferState.readCurRenderBufferState(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT2Idx, callNoList);
            }
            else
            {
                //printf("error: unknown error\n");
                DBG_LOG("error: unknown error\n");
            }
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT3No.thisCallNo);
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT3No.binderCallNo);
            if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT3Type == 0)//texture
            {
                textureList.push_back(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT3Idx);
            }
            else if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT3Type == 1)//renderbuffer
            {
                curRenderBufferState.readCurRenderBufferState(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT3Idx, callNoList);
            }
            else
            {
                //printf("error: unknown error\n");
                DBG_LOG("error: unknown error\n");
            }
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTNo.thisCallNo);
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTNo.binderCallNo);
            if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTType == 0)//texture
            {
                textureList.push_back(it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTIdx);
            }
            else if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTType == 1)//renderbuffer
            {
                curRenderBufferState.readCurRenderBufferState(it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTIdx, callNoList);
            }
            else
            {
                //printf("error: unknown error\n");
                DBG_LOG("error: unknown error\n");
            }
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTNo.thisCallNo);
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTNo.binderCallNo);
            if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTType == 0)//texture
            {
                textureList.push_back(it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTIdx);
            }
            else if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTType == 1)//renderbuffer
            {
                curRenderBufferState.readCurRenderBufferState(it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTIdx, callNoList);
            }
            else
            {
                //printf("error: unknown error\n");
                DBG_LOG("error: unknown error\n");
            }
            potr = it->second.glBlitFramebufferNoList.begin();
            while(potr != it->second.glBlitFramebufferNoList.end())
            {
                callNoList.insert(potr->readFrameBufferNo);
                callNoList.insert(potr->drawFrameBufferNo);
                callNoList.insert(potr->thisCallNo);
                potr++;
            }
        }//if(curReadFrameBufferIdx != 0)
    }
    if(curDrawFrameBufferNo != 0)
    {
        callNoList.insert(curDrawFrameBufferNo);
        if(curDrawFrameBufferIdx != 0)
        {
            it = allFrameBufferList.find(curDrawFrameBufferIdx);
            curFrameBufferIdx.insert(curDrawFrameBufferIdx);//for gldeleteframebuffer
            callNoList.insert(it->second.glGenFramebuffersNo);
            callNoList.insert(it->second.glDrawBuffersNo);
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0No.thisCallNo);
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0No.binderCallNo);
            if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0Type == 0)//texture
            {
                textureList.push_back(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0Idx);
            }
            else if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0Type == 1)//renderbuffer
            {
                curRenderBufferState.readCurRenderBufferState(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0Idx, callNoList);
            }
            else
            {
                //printf("error: unknown error\n");
                DBG_LOG("error: unknown error\n");
            }
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT1No.thisCallNo);
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT1No.binderCallNo);
            if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT1Type == 0)//texture
            {
                textureList.push_back(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT1Idx);
            }
            else if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT1Type == 1)//renderbuffer
            {
                curRenderBufferState.readCurRenderBufferState(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT1Idx, callNoList);
            }
            else
            {
                //printf("error: unknown error\n");
                DBG_LOG("error: unknown error\n");
            }
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT2No.thisCallNo);
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT2No.binderCallNo);
            if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT2Type == 0)//texture
            {
                textureList.push_back(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT2Idx);
            }
            else if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT2Type == 1)//renderbuffer
            {
                curRenderBufferState.readCurRenderBufferState(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT2Idx, callNoList);
            }
            else
            {
                //printf("error: unknown error\n");
                DBG_LOG("error: unknown error\n");
            }
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT3No.thisCallNo);
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT3No.binderCallNo);
            if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT3Type == 0)//texture
            {
                textureList.push_back(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT3Idx);
            }
            else if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT3Type == 1)//renderbuffer
            {
                curRenderBufferState.readCurRenderBufferState(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT3Idx, callNoList);
            }
            else
            {
                //printf("error: unknown error\n");
                DBG_LOG("error: unknown error\n");
            }
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTNo.thisCallNo);
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTNo.binderCallNo);
            if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTType == 0)//texture
            {
                textureList.push_back(it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTIdx);
            }
            else if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTType == 1)//renderbuffer
            {
                curRenderBufferState.readCurRenderBufferState(it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTIdx, callNoList);
            }
            else
            {
                //printf("error: unknown error\n");
                DBG_LOG("error: unknown error\n");
            }
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTNo.thisCallNo);
            callNoList.insert(it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTNo.binderCallNo);
            if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTType == 0)//texture
            {
                textureList.push_back(it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTIdx);
            }
            else if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTType == 1)//renderbuffer
            {
                curRenderBufferState.readCurRenderBufferState(it->second.glFramebufferTexture2D_RenderbufferNo.GL_STENCIL_ATTACHMENTIdx, callNoList);
            }
            else
            {
                //printf("error: unknown error\n");
                DBG_LOG("error: unknown error\n");
            }
            potr = it->second.glBlitFramebufferNoList.begin();
            while(potr != it->second.glBlitFramebufferNoList.end())
            {
                callNoList.insert(potr->readFrameBufferNo);
                callNoList.insert(potr->drawFrameBufferNo);
                callNoList.insert(potr->thisCallNo);
                potr++;
            }
        }//if(curDrawFrameBufferIdx != 0)
    }
}

bool frameBufferState::isBindFrameBuffer()
{
    if(curDrawFrameBufferIdx != 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool frameBufferState::isBindReadFrameBuffer()
{
    if(curReadFrameBufferIdx != 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

unsigned int frameBufferState::getDrawTextureIdx()
{
    map<unsigned int, frameBufferContext>::iterator it = allFrameBufferList.find(curDrawFrameBufferIdx);
    if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0Idx != 0)
    {
        return it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0Idx;
    }
    else
    {
        return it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTIdx;
    }
}

unsigned int frameBufferState::getDrawTextureNo()
{
    return curDrawFrameBufferNo;
}

unsigned int frameBufferState::getReadTextureNo()
{
    return curReadFrameBufferNo;
}

unsigned int frameBufferState::getReadTextureIdx()
{
    map<unsigned int, frameBufferContext>::iterator it = allFrameBufferList.find(curReadFrameBufferIdx);
    if(it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0Idx != 0)
    {
        return it->second.glFramebufferTexture2D_RenderbufferNo.GL_COLOR_ATTACHMENT0Idx;
    }
    else
    {
        return it->second.glFramebufferTexture2D_RenderbufferNo.GL_DEPTH_ATTACHMENTIdx;
    }
}

void frameBufferState::debug()
{
    unordered_set<unsigned int>::iterator it = curFrameBufferIdx.begin();
    while(it !=curFrameBufferIdx.end())
    {
        //printf("curFrameBufferIdx %d\n", *it);
        DBG_LOG("curFrameBufferIdx %d\n", *it);
        it++;
    }
}

void frameBufferState::clear()
{
    curRenderBufferState.clear();
    curReadFrameBufferIdx = 0;
    curReadFrameBufferNo = 0;
    curDrawFrameBufferIdx = 0;
    curDrawFrameBufferNo = 0;
    allFrameBufferList.clear();
    curFrameBufferIdx.clear();
}

//indices for some draws have indices
void contextState::readAllState(unsigned int frameNo, unsigned int drawType, unsigned int indices, unordered_set<unsigned int> &callNoList, bool * callNoListJudge)
{
    long long start_time = os::getTime();
    vector<unsigned int> clientSideBufferList;
    vector<unsigned int> bufferList;
    vector<unsigned int> textureList;//for frameBuffer
    clientSideBufferList.push_back(indices);
    curProgramState.readCurProgramState(/*clientSideBufferList, bufferList, */callNoList);
    curTextureState.readCurTextureState(callNoList, callNoListJudge);
    curGlobalState.readCurGlobalState(clientSideBufferList, bufferList, callNoList, curFrameBufferState.isBindFrameBuffer());
    curEglState.readCurEglState(callNoList);
    curBufferState.readCurBufferState(drawType, bufferList, callNoList);
    curFrameBufferState.readCurFrameBufferState(textureList, callNoList);
    curTextureState.findTextureContext(textureList, callNoList);
    curClientSideBufferState.readClientSideBufferState(clientSideBufferList, callNoList);

    long long end_time = os::getTime();
    float duration = static_cast<float>(end_time - start_time) / os::timeFrequency;
    retracer::gRetracer.timeCount = retracer::gRetracer.timeCount +duration;
}

void contextState::readStateForDispatchCompute(unordered_set<unsigned int> &callNoList, bool * callNoListJudge)
{
    curProgramState.readCurProgramState(callNoList);///*clientSideBufferList, bufferList, */
    curTextureState.readCurTextureState(callNoList, callNoListJudge);
    for(unsigned int i=0; i<MAX_GL_SHADER_STORAGE_BUFFER_NUM; i++)
    {
        callNoList.insert(curBufferState.curGL_SHADER_STORAGE_BUFFER0No[i]);
        callNoList.insert(curBufferState.curGL_UNIFORM_BUFFER0No[i]);
        if(curBufferState.curGL_SHADER_STORAGE_BUFFER0Idx[i] != 0)
        {
            curBufferState.findABufferState(curBufferState.curGL_SHADER_STORAGE_BUFFER0Idx[i], callNoList);
        }
        if(curBufferState.curGL_UNIFORM_BUFFER0Idx[i] != 0)
        {
            curBufferState.findABufferState(curBufferState.curGL_UNIFORM_BUFFER0Idx[i], callNoList);
        }
    }
}

void contextState::clear()
{
    curProgramState.clear();
    curTextureState.clear();
    curGlobalState.clear();
    curEglState.clear();
    curBufferState.clear();
    curFrameBufferState.clear();
    curClientSideBufferState.clear();
}

}//namespace newfastforwad
