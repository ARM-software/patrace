#include <newfastforwarder/parser.hpp>
#include <dispatch/eglproc_auto.hpp>//save
#include <common/trace_model.hpp> //save
#include <vector>
#include <unordered_set>
using namespace common;
using namespace retracer;

struct __attribute__ ((packed)) IndirectCompute
{
    GLuint x, y,z;
};

struct __attribute__ ((packed)) IndirectDrawArrays
{
    GLuint count;
    GLuint primCount;
    GLuint first;
    GLuint baseInstance;
};

struct __attribute__ ((packed)) IndirectDrawElements
{
    GLuint count;
    GLuint instanceCount;
    GLuint firstIndex;
    GLint baseVertex;
    GLuint reservedMustBeZero;
};

const unsigned int FIRST_10_BIT = 1000000000;
const unsigned int NO_IDX = 4294967295;
const unsigned int UNIFORM_COUNT_BASE = 100000;

static void retrace_glActiveTexture(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int texture; // enum
    _src = ReadFixed(_src, texture);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glActiveTexture(texture);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curTextureState.ff_glActiveTexture(texture, gRetracer.mCurCallNo);

    //gRetracer.callNoList.insert(gRetracer.mCurCallNo);
}

static void retrace_glAttachShader(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    unsigned int shader; // literal
    _src = ReadFixed<unsigned int>(_src, shader);

    // ----------- look up handles ------------
    //    Context& context = gRetracer.getCurrentContext();
    //    GLuint programNew = context.getProgramMap().RValue(program);
    //    GLuint shaderNew = context.getShaderMap().RValue(shader);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glAttachShader(programNew, shaderNew);
    // ---------- register handles ------------
    //for fastforwad
    gRetracer.curContextState->curProgramState.ff_glAttachShader(program, shader, gRetracer.mCurCallNo);
}

static void retrace_glBindAttribLocation(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    char* name; // string
    _src = ReadString(_src, name);

    // ----------- look up handles ------------
    //    Context& context = gRetracer.getCurrentContext();
    //    GLuint programNew = context.getProgramMap().RValue(program);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glBindAttribLocation(programNew, index, name);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curProgramState.ff_glBindAttribLocation(program, name, gRetracer.mCurCallNo);
}

static void retrace_glBindBuffer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    unsigned int buffer; // literal
    _src = ReadFixed<unsigned int>(_src, buffer);

    // ----------- look up handles ------------
    /*    Context& context = gRetracer.getCurrentContext();
    unsigned int bufferNew = context.getBufferMap().RValue(buffer);
    if (bufferNew == 0 && buffer != 0)
    {
        glGenBuffers(1, &bufferNew);
        context.getBufferMap().LValue(buffer) = bufferNew;
    }*/
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //gRetracer.callNoList.insert(gRetracer.mCurCallNo);
    //    glBindBuffer(target, bufferNew);
    // ---------- register handles ------------
    //if(target == GL_UNIFORM_BUFFER)target=target+31;
    gRetracer.curContextState->curBufferState.ff_glBindBuffer(target, buffer, gRetracer.mCurCallNo);
    gRetracer.curContextState->curBufferState.ff_glBindBufferForVao(target, buffer, gRetracer.mCurCallNo);
    //if(buffer == 0 && target == GL_ELEMENT_ARRAY_BUFFER )gRetracer.callNoList.insert(gRetracer.mCurCallNo);
}

static void retrace_glBindFramebuffer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    unsigned int framebuffer; // literal
    _src = ReadFixed<unsigned int>(_src, framebuffer);

    // ----------- look up handles ------------
    /*    Context& context = gRetracer.getCurrentContext();
    unsigned int framebufferNew = context._framebuffer_map.RValue(framebuffer);
    if (framebufferNew == 0 && framebuffer != 0)
    {
        glGenFramebuffers(1, &framebufferNew);
        context._framebuffer_map.LValue(framebuffer) = framebufferNew;
    }*/
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    hardcode_glBindFramebuffer(target, framebufferNew);
    // ---------- register handles ------------
    //gRetracer.callNoList.insert(gRetracer.mCurCallNo);

    if(gRetracer.preExecute == true)
    {
        bool readBuffer = false;
        if(target == GL_READ_FRAMEBUFFER)
        {
            readBuffer = true;
        }
        if(gRetracer.curFrameStoreState.readEndReadFrameBuffer()==true && gRetracer.curContextState->curFrameBufferState.isBindReadFrameBuffer()==true && framebuffer ==0 && readBuffer == true)
        {
            int drawTextureIdx = gRetracer.curContextState->curFrameBufferState.getReadTextureIdx();
            std::unordered_set<unsigned int> &textureNoList = gRetracer.curFrameStoreState.queryTextureNoList(drawTextureIdx);
            textureNoList.insert(gRetracer.mCurCallNo);
            gRetracer.curFrameStoreState.setEndReadFrameBuffer(false);
        }
        if(gRetracer.curFrameStoreState.readEndDraw()==true && gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer()==true && framebuffer ==0 && readBuffer == false)
        {
            int drawTextureIdx = gRetracer.curContextState->curFrameBufferState.getDrawTextureIdx();
            std::unordered_set<unsigned int> &textureNoList = gRetracer.curFrameStoreState.queryTextureNoList(drawTextureIdx);
            textureNoList.insert(gRetracer.mCurCallNo);
            gRetracer.curFrameStoreState.setEndDraw(false);
        }
        if(gRetracer.curFrameStoreState.readEndDrawClear()==true && gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer()==true && framebuffer ==0 && readBuffer == false)
        {
            int drawTextureIdx = gRetracer.curContextState->curFrameBufferState.getDrawTextureIdx();
            std::unordered_set<unsigned int> &textureNoList = gRetracer.curFrameStoreState.queryTextureClearNoList(drawTextureIdx);
            textureNoList.insert(gRetracer.mCurCallNo);
            gRetracer.curFrameStoreState.setEndDrawClear(false);
        }
    }//if(gRetracer.preExecute == true)
    else
    {}
    gRetracer.curContextState->curFrameBufferState.ff_glBindFramebuffer(framebuffer, target, gRetracer.mCurCallNo);
}

static void retrace_glBindRenderbuffer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    unsigned int renderbuffer; // literal
    _src = ReadFixed<unsigned int>(_src, renderbuffer);

    /*   // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    unsigned int renderbufferNew = context.getRenderbufferMap().RValue(renderbuffer);
    if (renderbufferNew == 0 && renderbuffer != 0)
    {
        glGenRenderbuffers(1, &renderbufferNew);
        context.getRenderbufferMap().LValue(renderbuffer) = renderbufferNew;
    }
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glBindRenderbuffer(target, renderbufferNew);
    // ---------- register handles ------------
    */
    gRetracer.curContextState->curFrameBufferState.curRenderBufferState.ff_glBindRenderbuffer(renderbuffer, gRetracer.mCurCallNo);
}

static void retrace_glBindTexture(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    unsigned int texture; // literal
    _src = ReadFixed<unsigned int>(_src, texture);

    // ----------- look up handles ------------
    /*    Context& context = gRetracer.getCurrentContext();
    unsigned int textureNew = context.getTextureMap().RValue(texture);
    if (textureNew == 0 && texture != 0)
    {
        glGenTextures(1, &textureNew);
        context.getTextureMap().LValue(texture) = textureNew;
    }*/
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glBindTexture(target, textureNew);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curTextureState.ff_glBindTexture(texture, target, gRetracer.mCurCallNo);
}

static void retrace_glBlendColor(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    float red; // literal
    _src = ReadFixed<float>(_src, red);
    float green; // literal
    _src = ReadFixed<float>(_src, green);
    float blue; // literal
    _src = ReadFixed<float>(_src, blue);
    float alpha; // literal
    _src = ReadFixed<float>(_src, alpha);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glBlendColor(red, green, blue, alpha);
    // ---------- register handles ------------
    gRetracer.curContextState->curGlobalState.ff_glBlendColor(gRetracer.mCurCallNo);
}

static void retrace_glBlendEquation(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int mode; // enum
    _src = ReadFixed(_src, mode);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glBlendEquation(mode);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curGlobalState.ff_glBlendEquation(mode, gRetracer.mCurCallNo);
}

static void retrace_glBlendEquationSeparate(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int modeRGB; // enum
    _src = ReadFixed(_src, modeRGB);
    int modeAlpha; // enum
    _src = ReadFixed(_src, modeAlpha);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    // glBlendEquationSeparate(modeRGB, modeAlpha);
    // ---------- register handles ------------

    gRetracer.curContextState->curGlobalState.ff_glBlendEquationSeparate(gRetracer.mCurCallNo);
}

static void retrace_glBlendFunc(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int sfactor; // enum
    _src = ReadFixed(_src, sfactor);
    int dfactor; // enum
    _src = ReadFixed(_src, dfactor);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glBlendFunc(sfactor, dfactor);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curGlobalState.ff_glBlendFunc(dfactor, gRetracer.mCurCallNo);
}

static void retrace_glBlendFuncSeparate(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int sfactorRGB; // enum
    _src = ReadFixed(_src, sfactorRGB);
    int dfactorRGB; // enum
    _src = ReadFixed(_src, dfactorRGB);
    int sfactorAlpha; // enum
    _src = ReadFixed(_src, sfactorAlpha);
    int dfactorAlpha; // enum
    _src = ReadFixed(_src, dfactorAlpha);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
    // ---------- register handles ------------
    gRetracer.curContextState->curGlobalState.ff_glBlendFuncSeparate(gRetracer.mCurCallNo);
}

static void retrace_glBufferData(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int size; // literal
    _src = ReadFixed<int>(_src, size);
    Array<char> data; // blob
    _src = Read1DArray(_src, data);
    int usage; // enum
    _src = ReadFixed(_src, usage);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    //#ifndef NDEBUG
    //    gRetracer.mVBODataSize += size;
    //#endif
    // ------------- retrace ------------------
    //   glBufferData(target, size, data, usage);
    // ---------- register handles ------------
    gRetracer.curContextState->curBufferState.ff_glBufferData(target, gRetracer.mCurCallNo);
}

static void retrace_glBufferSubData(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int offset; // literal
    _src = ReadFixed<int>(_src, offset);
    int size; // literal
    _src = ReadFixed<int>(_src, size);
    Array<char> data; // blob
    _src = Read1DArray(_src, data);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    //#ifndef NDEBUG
    //    gRetracer.mVBODataSize += size;
    //#endif
    // ------------- retrace ------------------
    //  glBufferSubData(target, offset, size, data);
    // ---------- register handles ------------
    gRetracer.curContextState->curBufferState.ff_glBufferSubData(target, offset, size, gRetracer.mCurCallNo);
}

static void retrace_glCheckFramebufferStatus(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);

    int old_ret; // enum
    _src = ReadFixed(_src, old_ret);
    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    GLenum ret = 0;
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  ret = glCheckFramebufferStatus(target);
    // ---------- register handles ------------
    (void)ret;  // Ignored return value
}

static void retrace_glClear(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int mask; // literal
    _src = ReadFixed<unsigned int>(_src, mask);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    //   gRetracer.IncCurDrawId();
    //    pre_glDraw();
    // ------------- retrace ------------------
    //  glClear(mask);
    // ---------- register handles ------------

    //for fastforwad
    //printf("mask ~~~~~~~~~~~~~~~~~~~~~~~~ %d\n",gRetracer.mOptions.clearMask);
    if(mask != 0)
    {
        if((gRetracer.preExecute == true && gRetracer.mCurCallNo > gRetracer.curPreExecuteState.endDrawCallNo) && gRetracer.mOptions.allDraws == false)
        {
            gRetracer.curPreExecuteState.newFinalPotr++;
            gRetracer.curPreExecuteState.beginDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)/FIRST_10_BIT;
            gRetracer.curPreExecuteState.endDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)%FIRST_10_BIT;
        }
        //std::vector<unsigned int> textureList;
        //if(gRetracer.preExecute == true && ((gRetracer.mCurCallNo>=gRetracer.curPreExecuteState.beginDrawCallNo&&gRetracer.mCurCallNo<=gRetracer.curPreExecuteState.endDrawCallNo)||gRetracer.mOptions.allDraws==true))
        //{
        //    if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer()==true && mask >= gRetracer.mOptions.clearMask/*17664&&gRetracer.mCurCallNo<=4473936mask>=16384 17664*/)
        //    {
        //    }
        //}

        gRetracer.curContextState->curGlobalState.ff_glClear(mask, gRetracer.mCurCallNo, gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer(), gRetracer.curContextState->curFrameBufferState.getDrawTextureNo());

        if(gRetracer.preExecute == true && ((gRetracer.mCurCallNo >= gRetracer.curPreExecuteState.beginDrawCallNo && gRetracer.mCurCallNo <= gRetracer.curPreExecuteState.endDrawCallNo) || gRetracer.mOptions.allDraws == true))
        {
            if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
            {
                if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true)
                {
                    std::vector<unsigned int> textureList;
                    //change this for save time
                    gRetracer.curContextState->curFrameBufferState.readCurFrameBufferState(textureList,gRetracer.callNoList);
                    gRetracer.curContextState->curGlobalState.readCurGlobalStateForFrameBufferClear(gRetracer.callNoList);
                    gRetracer.curFrameStoreState.setEndDrawClear(true);
                }
            }
        }//if(gRetracer.preExecute == true){
        else if(gRetracer.preExecute == false)
        {
            gRetracer.curPreExecuteState.drawCallNo++;
            bool clearOrNot = false;
            if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true && mask >= gRetracer.mOptions.clearMask)
            {
                int drawTextureIdx = gRetracer.curContextState->curFrameBufferState.getDrawTextureIdx();
                gRetracer.curPreExecuteState.clearTextureNoList(drawTextureIdx);
                clearOrNot = true;
            }
            if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
            {
                if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true)
                {
                    int drawTextureIdx = gRetracer.curContextState->curFrameBufferState.getDrawTextureIdx();
                    gRetracer.curPreExecuteState.newInsertCallIntoList(clearOrNot, drawTextureIdx);
                    gRetracer.curPreExecuteState.newInsertOneCallNo(clearOrNot, drawTextureIdx, gRetracer.mCurCallNo);
                }
            }
        }//if mask !=0
    }

    if(gRetracer.curPreExecuteState.endPreExecute == false)
    {
        if(gRetracer.mCurFrameNo > gRetracer.mOptions.fastforwadFrame1)
        {
            gRetracer.curPreExecuteState.newInsertCallIntoList(false, NO_IDX);
            gRetracer.curPreExecuteState.endPreExecute = true;
        }
    }
}

static void retrace_glClearColor(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    float red; // literal
    _src = ReadFixed<float>(_src, red);
    float green; // literal
    _src = ReadFixed<float>(_src, green);
    float blue; // literal
    _src = ReadFixed<float>(_src, blue);
    float alpha; // literal
    _src = ReadFixed<float>(_src, alpha);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glClearColor(red, green, blue, alpha);
    // ---------- register handles ------------
    //for fastforwad
    gRetracer.curContextState->curGlobalState.ff_glClearColor(gRetracer.mCurCallNo);
}

static void retrace_glClearDepthf(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    float d; // literal
    _src = ReadFixed<float>(_src, d);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glClearDepthf(d);
    // ---------- register handles ------------
    gRetracer.curContextState->curGlobalState.ff_glClearDepthf(gRetracer.mCurCallNo);
}

static void retrace_glClearStencil(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int s; // literal
    _src = ReadFixed<int>(_src, s);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glClearStencil(s);
    // ---------- register handles ------------
    gRetracer.curContextState->curGlobalState.ff_glClearStencil(gRetracer.mCurCallNo);
}

static void retrace_glColorMask(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned char red; // literal
    _src = ReadFixed<unsigned char>(_src, red);
    unsigned char green; // literal
    _src = ReadFixed<unsigned char>(_src, green);
    unsigned char blue; // literal
    _src = ReadFixed<unsigned char>(_src, blue);
    unsigned char alpha; // literal
    _src = ReadFixed<unsigned char>(_src, alpha);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glColorMask(red, green, blue, alpha);
    // ---------- register handles ------------
    gRetracer.curContextState->curGlobalState.ff_glColorMask(gRetracer.mCurCallNo);
}

static void retrace_glCompileShader(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int shader; // literal
    _src = ReadFixed<unsigned int>(_src, shader);

    // ----------- look up handles ------------
    //    Context& context = gRetracer.getCurrentContext();
    //    GLuint shaderNew = context.getShaderMap().RValue(shader);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glCompileShader(shaderNew);
    //   post_glCompileShader(shaderNew, shader);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curProgramState.curShaderState.ff_glCompileShader(shader, gRetracer.mCurCallNo);
}

static void retrace_glCompressedTexImage2D(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
#ifdef __APPLE__
    // Since we're only supporting Apple devices with GLES3
    // (which implies ETC2 support), we take advantage of ETC2's
    // backwards compatibility to get ETC1 support as well.
    // (This only makes sense for the internalformat, but is added to all parsed enums.)
    if (internalformat == GL_ETC1_RGB8_OES)
    {
        internalformat = GL_COMPRESSED_RGB8_ETC2;
    }
#endif
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);
    int border; // literal
    _src = ReadFixed<int>(_src, border);
    int imageSize; // literal
    _src = ReadFixed<int>(_src, imageSize);
    //   GLvoid* data = NULL;
    /*  Array<char> dataBlob;
    if (gRetracer.getFileFormatVersion() <= HEADER_VERSION_3)
    {
        _src = Read1DArray(_src, dataBlob);
        data = dataBlob.v;
    }
    else
    {
        unsigned int _opaque_type = 0;
        _src = ReadFixed(_src, _opaque_type);
        switch (_opaque_type)
        {
        case BlobType:
            _src = Read1DArray(_src, dataBlob);
            data = dataBlob.v;
            break;
        case BufferObjectReferenceType:
            unsigned int bufferOffset = 0;
            _src = ReadFixed<unsigned int>(_src, bufferOffset);
            data = (GLvoid *)static_cast<uintptr_t>(bufferOffset);
            break;
        }
    }
    */
    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    //#ifndef NDEBUG
    //  gRetracer.mCompressedTextureDataSize += imageSize;
    //#endif
    // ------------- retrace ------------------
     //   glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
    // ---------- register handles ------------

    //for fastforwad
    std::unordered_set<unsigned int> newList;
    unsigned int GL_PIXEL_UNPACK_BUFFERNo = gRetracer.curContextState->curBufferState.getCurGL_PIXEL_UNPACK_BUFFERNo();
    unsigned int GL_PIXEL_UNPACK_BUFFERIdx = gRetracer.curContextState->curBufferState.getCurGL_PIXEL_UNPACK_BUFFERIdx();
    if(GL_PIXEL_UNPACK_BUFFERIdx!=0)
    {
        gRetracer.curContextState->curBufferState.findABufferState(GL_PIXEL_UNPACK_BUFFERIdx, newList);
    }

    gRetracer.curContextState->curTextureState.ff_glTypes_TexImage2D(target, GL_PIXEL_UNPACK_BUFFERNo, gRetracer.mCurCallNo, newList);
}

static void retrace_glCompressedTexSubImage2D(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int xoffset; // literal
    _src = ReadFixed<int>(_src, xoffset);
    int yoffset; // literal
    _src = ReadFixed<int>(_src, yoffset);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);
    int format; // enum
    _src = ReadFixed(_src, format);
#ifdef __APPLE__
    // Since we're only supporting Apple devices with GLES3
    // (which implies ETC2 support), we take advantage of ETC2's
    // backwards compatibility to get ETC1 support as well.
    // (This only makes sense for the internalformat, but is added to all parsed enums.)
    if (format == GL_ETC1_RGB8_OES)
    {
        format = GL_COMPRESSED_RGB8_ETC2;
    }
#endif
    int imageSize; // literal
    _src = ReadFixed<int>(_src, imageSize);
    //   GLvoid* data = NULL;
    Array<char> dataBlob;
    if (gRetracer.getFileFormatVersion() <= HEADER_VERSION_3)
    {
        _src = Read1DArray(_src, dataBlob);
        //      data = dataBlob.v;
    }
    else
    {
        unsigned int _opaque_type = 0;
        _src = ReadFixed(_src, _opaque_type);
        switch (_opaque_type)
        {
        case BlobType:
            _src = Read1DArray(_src, dataBlob);
            //         data = dataBlob.v;
            break;
        case BufferObjectReferenceType:
            unsigned int bufferOffset = 0;
            _src = ReadFixed<unsigned int>(_src, bufferOffset);
            //        data = (GLvoid *)static_cast<uintptr_t>(bufferOffset);
            break;
        }
    }

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    //#ifndef NDEBUG
    //    gRetracer.mCompressedTextureDataSize += imageSize;
    //#endif
    // ------------- retrace ------------------
    //   glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
    // ---------- register handles ------------
    std::unordered_set<unsigned int> newList;
    unsigned int GL_PIXEL_UNPACK_BUFFERNo = gRetracer.curContextState->curBufferState.getCurGL_PIXEL_UNPACK_BUFFERNo();
    unsigned int GL_PIXEL_UNPACK_BUFFERIdx = gRetracer.curContextState->curBufferState.getCurGL_PIXEL_UNPACK_BUFFERIdx();
    if(GL_PIXEL_UNPACK_BUFFERIdx!=0)
    {
        gRetracer.curContextState->curBufferState.findABufferState(GL_PIXEL_UNPACK_BUFFERIdx, newList);
    }

    gRetracer.curContextState->curTextureState.ff_glTypes_TexSubImage2D(target,GL_PIXEL_UNPACK_BUFFERNo, gRetracer.mCurCallNo, newList);
}

static void retrace_glCopyTexImage2D(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    int x; // literal
    _src = ReadFixed<int>(_src, x);
    int y; // literal
    _src = ReadFixed<int>(_src, y);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);
    int border; // literal
    _src = ReadFixed<int>(_src, border);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
    // ---------- register handles ------------
}

static void retrace_glCopyTexSubImage2D(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int xoffset; // literal
    _src = ReadFixed<int>(_src, xoffset);
    int yoffset; // literal
    _src = ReadFixed<int>(_src, yoffset);
    int x; // literal
    _src = ReadFixed<int>(_src, x);
    int y; // literal
    _src = ReadFixed<int>(_src, y);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
    // ---------- register handles ------------
}

static void retrace_glCreateProgram(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------

    unsigned int old_ret; // literal
    _src = ReadFixed<unsigned int>(_src, old_ret);
    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    //   GLuint ret = 0;
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    ret = glCreateProgram();
    // ---------- register handles ------------
    //  Context& context = gRetracer.getCurrentContext();
    //   context.getProgramMap().LValue(old_ret) = ret;
    //    context.getProgramRevMap().LValue(ret) = old_ret;

    //for fastforwad
    gRetracer.curContextState->curProgramState.ff_glCreateProgram(old_ret, gRetracer.mCurCallNo);
}

static void retrace_glCreateShader(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int type; // enum
    _src = ReadFixed(_src, type);

    unsigned int old_ret; // literal
    _src = ReadFixed<unsigned int>(_src, old_ret);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    //   GLuint ret = 0;
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    ret = glCreateShader(type);
    // ---------- register handles ------------
    //    Context& context = gRetracer.getCurrentContext();
    //    context.getShaderMap().LValue(old_ret) = ret;
    //    context.getShaderRevMap().LValue(ret) = old_ret;

    //for fastforwad
    gRetracer.curContextState->curProgramState.curShaderState.ff_glCreatShader(old_ret, type, gRetracer.mCurCallNo);
}

static void retrace_glCullFace(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int mode; // enum
    _src = ReadFixed(_src, mode);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glCullFace(mode);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curGlobalState.ff_glCullFace(mode, gRetracer.mCurCallNo);
}

static void retrace_glDeleteBuffers(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> buffers; // array
    _src = Read1DArray(_src, buffers);

    // ----------- look up handles ------------
    // hardcode in retrace!
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    hardcode_glDeleteBuffers(n, buffers);
    // ---------- register handles ------------
    //gRetracer.callNoList.insert(gRetracer.mCurCallNo);
    gRetracer.curContextState->curBufferState.ff_glDeleteBuffers(n, buffers, gRetracer.mCurCallNo);
}

static void retrace_glDeleteFramebuffers(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> framebuffers; // array
    _src = Read1DArray(_src, framebuffers);

    // ----------- look up handles ------------
    // hardcode in retrace!
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    hardcode_glDeleteFramebuffers(n, framebuffers);
    // ---------- register handles ------------
    //gRetracer.callNoList.insert(gRetracer.mCurCallNo);
    if(gRetracer.preExecute == true)
    {
        gRetracer.curContextState->curFrameBufferState.ff_glDeleteFramebuffers(n, framebuffers, gRetracer.mCurCallNo, gRetracer.callNoList);
    }
}

static void retrace_glDeleteProgram(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);

    /*   // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    context.getProgramMap().LValue(program) = 0;
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glDeleteProgram(programNew);
    // ---------- register handles ------------
    */
    //gRetracer.callNoList.insert(gRetracer.mCurCallNo);
}

static void retrace_glDeleteRenderbuffers(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> renderbuffers; // array
    _src = Read1DArray(_src, renderbuffers);

    // ----------- look up handles ------------
    // hardcode in retrace!
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   hardcode_glDeleteRenderbuffers(n, renderbuffers);
    // ---------- register handles ------------
    //gRetracer.callNoList.insert(gRetracer.mCurCallNo);
}

static void retrace_glDeleteShader(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int shader; // literal
    _src = ReadFixed<unsigned int>(_src, shader);

    /*  // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint shaderNew = context.getShaderMap().RValue(shader);
    context.getShaderMap().LValue(shader) = 0;
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glDeleteShader(shaderNew);
    // ---------- register handles ------------
    */
    //gRetracer.callNoList.insert(gRetracer.mCurCallNo);
}

static void retrace_glDeleteTextures(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> textures; // array
    _src = Read1DArray(_src, textures);

    // ----------- look up handles ------------
    // hardcode in retrace!
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //fast    hardcode_glDeleteTextures(n, textures);
    // ---------- register handles ------------
    //gRetracer.callNoList.insert(gRetracer.mCurCallNo);
}

static void retrace_glDepthFunc(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int func; // enum
    _src = ReadFixed(_src, func);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glDepthFunc(func);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curGlobalState.ff_glDepthFunc(func, gRetracer.mCurCallNo);
}

static void retrace_glDepthMask(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned char flag; // literal
    _src = ReadFixed<unsigned char>(_src, flag);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glDepthMask(flag);
    // ---------- register handles ------------
    //for fastforwad
    gRetracer.curContextState->curGlobalState.ff_glDepthMask(flag, gRetracer.mCurCallNo);
}

static void retrace_glDepthRangef(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    float n; // literal
    _src = ReadFixed<float>(_src, n);
    float f; // literal
    _src = ReadFixed<float>(_src, f);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glDepthRangef(n, f);
    // ---------- register handles ------------
    gRetracer.curContextState->curGlobalState.ff_glDepthRangef(gRetracer.mCurCallNo);
}

static void retrace_glDetachShader(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    unsigned int shader; // literal
    _src = ReadFixed<unsigned int>(_src, shader);

    /*    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLuint shaderNew = context.getShaderMap().RValue(shader);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glDetachShader(programNew, shaderNew);
    // ---------- register handles ------------
    */
}

static void retrace_glDisable(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int cap; // enum
    _src = ReadFixed(_src, cap);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
     //   glDisable(cap);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curGlobalState.ff_glEnableDisable(cap, gRetracer.mCurCallNo);
}

static void retrace_glDisableVertexAttribArray(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glDisableVertexAttribArray(index);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curGlobalState.ff_glDisableVertexAttribArray(index, gRetracer.mCurCallNo);
}

static void retrace_glDrawArrays(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int mode; // enum
    _src = ReadFixed(_src, mode);
    int first; // literal
    _src = ReadFixed<int>(_src, first);
    int count; // literal
    _src = ReadFixed<int>(_src, count);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    /*  gRetracer.IncCurDrawId();
    if (unlikely(stateLoggingEnabled)) {
        gRetracer.getStateLogger().logFunction(gRetracer.getCurTid(), "glDrawArrays", gRetracer.GetCurCallId(), gRetracer.GetCurDrawId());
        gRetracer.getStateLogger().logState(gRetracer.getCurTid(), first, count, 0);
    }
    pre_glDraw();*/
    // ------------- retrace ------------------
    //    glDrawArrays(mode, first, count);
    // ---------- register handles ------------

    //for fastforwad

    if((gRetracer.preExecute == true && gRetracer.mCurCallNo > gRetracer.curPreExecuteState.endDrawCallNo) && gRetracer.mOptions.allDraws == false)
    {
        gRetracer.curPreExecuteState.newFinalPotr++;
        gRetracer.curPreExecuteState.beginDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)/FIRST_10_BIT;
        gRetracer.curPreExecuteState.endDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)%FIRST_10_BIT;
    }

    std::vector<unsigned int> textureList;

    //debug for fast
    if(gRetracer.preExecute == true && ((gRetracer.mCurCallNo>=gRetracer.curPreExecuteState.beginDrawCallNo&&gRetracer.mCurCallNo <= gRetracer.curPreExecuteState.endDrawCallNo) || gRetracer.curContextState->curGlobalState.TransformFeedbackSwitch == true || gRetracer.mOptions.allDraws == true))
    {
        if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
        {
            if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true)
            {
                gRetracer.curContextState->readAllState(0, 0, NO_IDX, gRetracer.callNoList, gRetracer.callNoListJudge);
                gRetracer.callNoList.insert(gRetracer.mCurCallNo);
                gRetracer.curFrameStoreState.setEndDraw(true);
            }
        }//for debug
        textureList.clear();
        if(((gRetracer.mCurFrameNo >= gRetracer.mOptions.fastforwadFrame0 && gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)&&gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer()==false)||gRetracer.curContextState->curGlobalState.TransformFeedbackSwitch == true)
        {
            gRetracer.curContextState->readAllState(0, 0, NO_IDX, gRetracer.callNoList, gRetracer.callNoListJudge);
            gRetracer.callNoList.insert(gRetracer.mCurCallNo);
        }
    }//if(gRetracer.preExecute == true)
    else if(gRetracer.preExecute == false)
    {
        gRetracer.curPreExecuteState.drawCallNo++;
        if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
        {
            if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer()==true)
            {
                int drawTextureIdx = gRetracer.curContextState->curFrameBufferState.getDrawTextureIdx();
                gRetracer.curContextState->curTextureState.readCurTextureIdx(textureList);
                gRetracer.curPreExecuteState.newInsertCallIntoList(false, drawTextureIdx);
                gRetracer.curPreExecuteState.newInsertTextureListNo(textureList,gRetracer.curPreExecuteState.newQueryTextureNoList(drawTextureIdx));
                gRetracer.curPreExecuteState.newInsertOneCallNo(false, drawTextureIdx, gRetracer.mCurCallNo);
            }
        }//for debug
        textureList.clear();
        if((gRetracer.mCurFrameNo >= gRetracer.mOptions.fastforwadFrame0 && gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1) && gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == false)
        {
            gRetracer.curContextState->curTextureState.readCurTextureIdx(textureList);
            gRetracer.curPreExecuteState.newInsertCallIntoList(false, 0);
            gRetracer.curPreExecuteState.newInsertTextureListNo(textureList, gRetracer.curPreExecuteState.newQueryTextureNoList(0));
            gRetracer.curPreExecuteState.newInsertOneCallNo(false, 0, gRetracer.mCurCallNo);
        }
    }//if(gRetracer.preExecute == true)

    if(gRetracer.curPreExecuteState.endPreExecute == false)
    {
        if(gRetracer.mCurFrameNo > gRetracer.mOptions.fastforwadFrame1)
        {
            gRetracer.curPreExecuteState.newInsertCallIntoList(false, NO_IDX);
            gRetracer.curPreExecuteState.endPreExecute=true;
        }
    }
}

static void retrace_glDrawElements(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int mode; // enum
    _src = ReadFixed(_src, mode);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    int type; // enum
    _src = ReadFixed(_src, type);
    unsigned int csbName = NO_IDX;//0; // ClientSideBuffer name
    //    GLvoid* indices = NULL;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    if (_opaque_type == BufferObjectReferenceType)
    { // simple memory offset
        unsigned int indices_raw; // raw ptr
        _src = ReadFixed<unsigned int>(_src, indices_raw);
    //        indices = (GLvoid *)static_cast<uintptr_t>(indices_raw);
    }
    else if (_opaque_type == BlobType)
    { // blob type
        Array<float> indices_blob;
        _src = Read1DArray(_src, indices_blob);
    //        indices = indices_blob.v;
    }
    else if (_opaque_type == ClientSideBufferObjectReferenceType)
    { // a memory reference to client-side buffer
        unsigned int offset = 0;
        _src = ReadFixed<unsigned int>(_src, csbName);
        _src = ReadFixed<unsigned int>(_src, offset);
        //        indices = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);
    }
    else if (_opaque_type == NoopType)
    { // Do nothing
    }

    /*    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    gRetracer.IncCurDrawId();
    if (unlikely(stateLoggingEnabled)) {
        gRetracer.getStateLogger().logFunction(gRetracer.getCurTid(), "glDrawElements", gRetracer.GetCurCallId(), gRetracer.GetCurDrawId());
        gRetracer.getStateLogger().logState(gRetracer.getCurTid(), count, type, indices, 0);
    }
    //    pre_glDraw();
    // ------------- retrace ------------------
    glDrawElements(mode, count, type, indices);
    // ---------- register handles ------------
    */

    if((gRetracer.preExecute == true && gRetracer.mCurCallNo > gRetracer.curPreExecuteState.endDrawCallNo) && gRetracer.mOptions.allDraws == false)
    {
        gRetracer.curPreExecuteState.newFinalPotr++;
        gRetracer.curPreExecuteState.beginDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)/FIRST_10_BIT;
        gRetracer.curPreExecuteState.endDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)%FIRST_10_BIT;
    }

    std::vector<unsigned int> textureList;

    if(gRetracer.preExecute == true && ((gRetracer.mCurCallNo >= gRetracer.curPreExecuteState.beginDrawCallNo && gRetracer.mCurCallNo <= gRetracer.curPreExecuteState.endDrawCallNo) || gRetracer.mOptions.allDraws == true))
    {
        if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
        {
            if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true)
            {
                gRetracer.curContextState->readAllState(0, 1, csbName, gRetracer.callNoList, gRetracer.callNoListJudge);
                gRetracer.callNoList.insert(gRetracer.mCurCallNo);
                gRetracer.curFrameStoreState.setEndDraw(true);
            }
        }//for debug
        textureList.clear();
        if((gRetracer.mCurFrameNo >= gRetracer.mOptions.fastforwadFrame0 && gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1) && gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == false)
        {
            gRetracer.curContextState->readAllState(0, 1, csbName, gRetracer.callNoList, gRetracer.callNoListJudge);
            gRetracer.callNoList.insert(gRetracer.mCurCallNo);
        }
    }//if(gRetracer.preExecute == true){
    else if(gRetracer.preExecute == false)
    {
        gRetracer.curPreExecuteState.drawCallNo++;
        if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
        {
            if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true)
            {
                int drawTextureIdx = gRetracer.curContextState->curFrameBufferState.getDrawTextureIdx();
                gRetracer.curContextState->curTextureState.readCurTextureIdx(textureList);
                gRetracer.curPreExecuteState.newInsertCallIntoList(false, drawTextureIdx);
                gRetracer.curPreExecuteState.newInsertTextureListNo(textureList, gRetracer.curPreExecuteState.newQueryTextureNoList(drawTextureIdx));
                gRetracer.curPreExecuteState.newInsertOneCallNo(false, drawTextureIdx, gRetracer.mCurCallNo);
            }
        }//for debug
        textureList.clear();
        if((gRetracer.mCurFrameNo >= gRetracer.mOptions.fastforwadFrame0 && gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1) && gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == false)
        {
            gRetracer.curContextState->curTextureState.readCurTextureIdx(textureList);
            gRetracer.curPreExecuteState.newInsertCallIntoList(false, 0);
            gRetracer.curPreExecuteState.newInsertTextureListNo(textureList, gRetracer.curPreExecuteState.newQueryTextureNoList(0));
            gRetracer.curPreExecuteState.newInsertOneCallNo(false, 0, gRetracer.mCurCallNo);
        }
    }//if(gRetracer.preExecute == true)

    if(gRetracer.curPreExecuteState.endPreExecute == false)
    {
        if(gRetracer.mCurFrameNo > gRetracer.mOptions.fastforwadFrame1)
        {
            gRetracer.curPreExecuteState.newInsertCallIntoList(false, NO_IDX);
            gRetracer.curPreExecuteState.endPreExecute = true;
        }
    }
}

static void retrace_glEnable(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int cap; // enum
    _src = ReadFixed(_src, cap);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glEnable(cap);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curGlobalState.ff_glEnableDisable(cap, gRetracer.mCurCallNo);
}

static void retrace_glEnableVertexAttribArray(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glEnableVertexAttribArray(index);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curGlobalState.ff_glEnableVertexAttribArray(index, gRetracer.mCurCallNo);
    gRetracer.curContextState->curBufferState.ff_glEnableVertexAttribArrayForVao(index, gRetracer.mCurCallNo);
}

static void retrace_glFinish(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glFinish();
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curGlobalState.ff_glFinish(gRetracer.mCurCallNo);
    gRetracer.callNoList.insert(gRetracer.mCurCallNo);
}

static void retrace_glFlush(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glFlush();
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curGlobalState.ff_glFlush(gRetracer.mCurCallNo);
}

static void retrace_glFramebufferRenderbuffer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int attachment; // enum
    _src = ReadFixed(_src, attachment);
    int renderbuffertarget; // enum
    _src = ReadFixed(_src, renderbuffertarget);
    unsigned int renderbuffer; // literal
    _src = ReadFixed<unsigned int>(_src, renderbuffer);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint renderbufferNew = context.getRenderbufferMap().RValue(renderbuffer);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbufferNew);
    // ---------- register handles ------------
    */
    gRetracer.curContextState->curFrameBufferState.ff_glFramebufferRenderbuffer(target, attachment, renderbuffer, gRetracer.mCurCallNo);
}

static void retrace_glFramebufferTexture2D(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int attachment; // enum
    _src = ReadFixed(_src, attachment);
    int textarget; // enum
    _src = ReadFixed(_src, textarget);
    unsigned int texture; // literal
    _src = ReadFixed<unsigned int>(_src, texture);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint textureNew = context.getTextureMap().RValue(texture);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glFramebufferTexture2D(target, attachment, textarget, textureNew, level);
    // ---------- register handles ------------
    */
    gRetracer.curContextState->curFrameBufferState.ff_glFramebufferTexture2D(target, attachment, textarget, texture, gRetracer.mCurCallNo);
}

static void retrace_glFrontFace(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int mode; // enum
    _src = ReadFixed(_src, mode);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glFrontFace(mode);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curGlobalState.ff_glFrontFace(mode, gRetracer.mCurCallNo);
}

static void retrace_glGenBuffers(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> old_buffer; // array
    _src = Read1DArray(_src, old_buffer);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    // ----------- out parameters  ------------
    std::vector<GLuint> _buffer(n);
    GLuint *buffer = _buffer.data();
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glGenBuffers(n, buffer);
    // ---------- register handles ------------
    for (unsigned int i = 0; i < old_buffer.cnt; ++i)
    {
        context.getBufferMap().LValue(old_buffer[i]) = buffer[i];
        context.getBufferRevMap().LValue(buffer[i]) = old_buffer[i];
    }
    */
    gRetracer.curContextState->curBufferState.ff_glGenBuffers(n, old_buffer, gRetracer.mCurCallNo);
}

static void retrace_glGenFramebuffers(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> old_framebuffers; // array
    _src = Read1DArray(_src, old_framebuffers);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    // ----------- out parameters  ------------
    std::vector<GLuint> _framebuffers(n);
    GLuint *framebuffers = _framebuffers.data();
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glGenFramebuffers(n, framebuffers);
    // ---------- register handles ------------
    for (unsigned int i = 0; i < old_framebuffers.cnt; ++i)
    {
        context._framebuffer_map.LValue(old_framebuffers[i]) = framebuffers[i];
    }
    */
    gRetracer.curContextState->curFrameBufferState.ff_glGenFramebuffers(n, old_framebuffers, gRetracer.mCurCallNo);
}

static void retrace_glGenRenderbuffers(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> old_renderbuffers; // array
    _src = Read1DArray(_src, old_renderbuffers);

    /*    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    // ----------- out parameters  ------------
    std::vector<GLuint> _renderbuffers(n);
    GLuint *renderbuffers = _renderbuffers.data();
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glGenRenderbuffers(n, renderbuffers);
    // ---------- register handles ------------
    for (unsigned int i = 0; i < old_renderbuffers.cnt; ++i)
    {
        context.getRenderbufferMap().LValue(old_renderbuffers[i]) = renderbuffers[i];
    }
    */
    gRetracer.curContextState->curFrameBufferState.curRenderBufferState.ff_glGenRenderbuffers(old_renderbuffers[0], gRetracer.mCurCallNo);
}

static void retrace_glGenTextures(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> old_textures; // array
    _src = Read1DArray(_src, old_textures);

    // ----------- look up handles ------------
    //    Context& context = gRetracer.getCurrentContext();
    // ----------- out parameters  ------------
    //    std::vector<GLuint> _textures(n);
    //    GLuint *textures = _textures.data();
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glGenTextures(n, textures);
    // ---------- register handles ------------
    /*    for (unsigned int i = 0; i < old_textures.cnt; ++i)
    {
        context.getTextureMap().LValue(old_textures[i]) = textures[i];
        context.getTextureRevMap().LValue(textures[i]) = old_textures[i];
    }*/
    //for fastforwad
    //printf("gentextur  %d   %d\n",old_textures[0], gRetracer.mCurCallNo);
    gRetracer.curContextState->curTextureState.ff_glGenTextures(n, old_textures, gRetracer.mCurCallNo);
}

static void retrace_glGenerateMipmap(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glGenerateMipmap(target);
    // ---------- register handles ------------
    gRetracer.curContextState->curTextureState.ff_glGenerateMipmap(gRetracer.mCurCallNo);
}

static void retrace_glGetAttribLocation(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    char* name; // string
    _src = ReadString(_src, name);

    unsigned int old_ret; // literal
    _src = ReadFixed<unsigned int>(_src, old_ret);
    // ----------- look up handles ------------
    //    Context& context = gRetracer.getCurrentContext();
    //    GLuint programNew = context.getProgramMap().RValue(program);
    // ----------- out parameters  ------------
     //   GLuint ret = 0;
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    ret = glGetAttribLocation(programNew, name);
    // ---------- register handles ------------
     //   (void)ret;  // Ignored return value

    //for fastforwad
    gRetracer.curContextState->curProgramState.ff_glGetAttribLocation(program, name, gRetracer.mCurCallNo);
}

static void retrace_glGetUniformLocation(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    char* name; // string
    _src = ReadString(_src, name);
    int old_ret; // literal
    _src = ReadFixed<int>(_src, old_ret);
    // ----------- look up handles ------------
    //    Context& context = gRetracer.getCurrentContext();
    //    GLuint programNew = context.getProgramMap().RValue(program);
    // ----------- out parameters  ------------
    //   GLint ret = 0;
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    ret = glGetUniformLocation(programNew, name);
    // ---------- register handles ------------
    /*    if (old_ret == -1)
        return;
    context._uniformLocation_map[programNew].LValue(old_ret) = ret;
    */
    //for fastforwad
    gRetracer.curContextState->curProgramState.ff_glGetUniformLocation(program, name, old_ret, gRetracer.mCurCallNo);
}

static void retrace_glHint(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int mode; // enum
    _src = ReadFixed(_src, mode);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glHint(target, mode);
    // ---------- register handles ------------
    //gRetracer.curContextState->curGlobalState.ff_glHint(target,gRetracer.mCurCallNo);
    //gRetracer.curContextState->curTextureState.ff_glHint_gl_generate_mipmap_hint(gRetracer.mCurCallNo);
}

static void retrace_glLineWidth(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    float width; // literal
    _src = ReadFixed<float>(_src, width);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glLineWidth(width);
    // ---------- register handles ------------
    gRetracer.curContextState->curGlobalState.ff_glLineWidth(gRetracer.mCurCallNo);
}

static void retrace_glLinkProgram(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);

    // ----------- look up handles ------------
    //    Context& context = gRetracer.getCurrentContext();
    //    GLuint programNew = context.getProgramMap().RValue(program);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    /*    glLinkProgram(programNew);
    if (unlikely(gRetracer.mOptions.mDumpStatic))
    {
        GLint linkStatus = 0;
        _glGetProgramiv(programNew, GL_LINK_STATUS, &linkStatus);
        if (linkStatus == GL_TRUE)
        {
            Json::Value entry = Json::arrayValue;
            GLchar tmp[128];
            GLint activeUniforms = 0;
            _glGetProgramiv(programNew, GL_ACTIVE_UNIFORMS, &activeUniforms);
            for (int i = 0; i < activeUniforms; ++i)
            {
                GLsizei length = 0;
                GLint size = 0;
                GLenum type = GL_NONE;
                Json::Value value = Json::arrayValue;
                memset(tmp, 0, sizeof(tmp));
                _glGetActiveUniform(programNew, i, sizeof(tmp) - 1, &length, &size, &type, tmp);
                if (type == GL_SAMPLER_2D || type == GL_SAMPLER_3D || type == GL_SAMPLER_CUBE || type == GL_SAMPLER_2D_SHADOW
                    || type == GL_SAMPLER_2D_ARRAY || type == GL_SAMPLER_2D_ARRAY_SHADOW || type == GL_SAMPLER_CUBE_SHADOW
                    || type == GL_INT_SAMPLER_2D || type == GL_INT_SAMPLER_3D || type == GL_INT_SAMPLER_2D_ARRAY
                    || type == GL_UNSIGNED_INT_SAMPLER_2D || type == GL_UNSIGNED_INT_SAMPLER_3D || type == GL_UNSIGNED_INT_SAMPLER_CUBE
                    || type == GL_UNSIGNED_INT_SAMPLER_2D_ARRAY)
                {
                    GLint location = _glGetUniformLocation(programNew, tmp);
                    for (int j = 0; location >= 0 && j < size; ++j)
                    {
                        GLint param = 0;
                        _glGetUniformiv(programNew, location + j, &param); // arrays are guaranteed to be in sequential location
                        value.append(param);
                    }
                    entry[i]["value"] = value;
                }
                entry[i]["name"] = tmp;
                entry[i]["size"] = size;
                entry[i]["type"] = type;
            }
            gRetracer.staticDump["uniforms"].append(entry);
        }
    }
    post_glLinkProgram(programNew, program);*/
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curProgramState.ff_glLinkProgram(program, gRetracer.mCurCallNo);
}

static void retrace_glPixelStorei(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int pname; // enum
    _src = ReadFixed(_src, pname);
    int param; // literal
    _src = ReadFixed<int>(_src, param);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glPixelStorei(pname, param);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curTextureState.ff_glPixelStorei(pname, param, gRetracer.mCurCallNo);
}

static void retrace_glPolygonOffset(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    float factor; // literal
    _src = ReadFixed<float>(_src, factor);
    float units; // literal
    _src = ReadFixed<float>(_src, units);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glPolygonOffset(factor, units);
    // ---------- register handles ------------
    gRetracer.curContextState->curGlobalState.ff_glPolygonOffset(gRetracer.mCurCallNo);
}

static void retrace_glReadPixels(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int x; // literal
    _src = ReadFixed<int>(_src, x);
    int y; // literal
    _src = ReadFixed<int>(_src, y);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);
    int format; // enum
    _src = ReadFixed(_src, format);
    int type; // enum
    _src = ReadFixed(_src, type);
    //GLvoid* old_pixels = NULL;
    //bool isPixelPackBufferBound = false;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    /*if (gRetracer.getFileFormatVersion() <= HEADER_VERSION_3)
    {
       unsigned int t = 0;
       _src = ReadFixed<unsigned int>(_src, t);
       // old_pixels will not be used
       old_pixels = (GLvoid *)static_cast<uintptr_t>(t);
    }
    else // if (gRetracer.getFileFormatVersion() > HEADER_VERSION_3)
    {
        if (_opaque_type == BufferObjectReferenceType)
        {
            isPixelPackBufferBound = true;
            unsigned int bufferOffset = 0;
            _src = ReadFixed<unsigned int>(_src, bufferOffset);
            old_pixels = (GLvoid *)static_cast<uintptr_t>(bufferOffset);
        }
        else if (_opaque_type == NoopType)
        {
        }
        else
        {
            DBG_LOG("ERROR: Unsupported opaque type\n");
        }
    }

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    GLvoid* pixels = 0;

    // If GLES3 or higher, the glReadPixels operation can
    // potentionally be meant to a PIXEL_PACK_BUFFER.
    if (!isPixelPackBufferBound)
    {
      //  size_t size = _gl_image_size(format, type, width, height, 1);
     //   pixels = new char[size];
    }
    else
    {
        // If a pixel pack buffer is bound, old_pixels is
        // treated as an offset
        pixels = old_pixels;
    }

    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glReadPixels(x, y, width, height, format, type, pixels);
    // ---------- register handles ------------
    if (!isPixelPackBufferBound)
    {
        delete[] static_cast<char*>(pixels);
    }
    */
}

static void retrace_glReleaseShaderCompiler(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glReleaseShaderCompiler();
    // ---------- register handles ------------
}

static void retrace_glRenderbufferStorage(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glRenderbufferStorage(target, internalformat, width, height);
    // ---------- register handles ------------
    gRetracer.curContextState->curFrameBufferState.curRenderBufferState.ff_glRenderbufferStorage(gRetracer.mCurCallNo);
}

static void retrace_glSampleCoverage(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    float value; // literal
    _src = ReadFixed<float>(_src, value);
    unsigned char invert; // literal
    _src = ReadFixed<unsigned char>(_src, invert);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glSampleCoverage(value, invert);
    // ---------- register handles ------------
    gRetracer.curContextState->curGlobalState.ff_glSampleCoverage(gRetracer.mCurCallNo);
}

static void retrace_glScissor(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int x; // literal
    _src = ReadFixed<int>(_src, x);
    int y; // literal
    _src = ReadFixed<int>(_src, y);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    /*   if (gRetracer.mOptions.mDoOverrideResolution)
    {
        gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppSR.x = x;
        gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppSR.y = y;
        gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppSR.w = width;
        gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppSR.h = height;
    }
    else
    {
    // ------------- retrace ------------------
    //       glScissor(x, y, width, height);
    }*/
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curGlobalState.ff_glScissor(gRetracer.mCurCallNo);
}

static void retrace_glShaderBinary(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<unsigned int> shaders; // array
    _src = Read1DArray(_src, shaders);
    int binaryformat; // enum
    _src = ReadFixed(_src, binaryformat);
    Array<char> binary; // blob
    _src = Read1DArray(_src, binary);
    int length; // literal
    _src = ReadFixed<int>(_src, length);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glShaderBinary(count, shaders, binaryformat, binary, length);
    // ---------- register handles ------------
}

static void retrace_glShaderSource(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int shader; // literal
    _src = ReadFixed<unsigned int>(_src, shader);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<const char*> string; // string array
    _src = ReadStringArray(_src, string);
    Array<int> length; // array
    _src = Read1DArray(_src, length);

    // ----------- look up handles ------------
    //    Context& context = gRetracer.getCurrentContext();
    //    GLuint shaderNew = context.getShaderMap().RValue(shader);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glShaderSource(shaderNew, count, string, length);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curProgramState.curShaderState.ff_glShaderSource(shader, gRetracer.mCurCallNo);
}

static void retrace_glStencilFunc(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int func; // enum
    _src = ReadFixed(_src, func);
    int ref; // literal
    _src = ReadFixed<int>(_src, ref);
    unsigned int mask; // literal
    _src = ReadFixed<unsigned int>(_src, mask);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glStencilFunc(func, ref, mask);
    // ---------- register handles ------------
    gRetracer.curContextState->curGlobalState.ff_glStencilFuncSeparate(GL_FRONT_AND_BACK, gRetracer.mCurCallNo);
}

static void retrace_glStencilFuncSeparate(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int face; // enum
    _src = ReadFixed(_src, face);
    int func; // enum
    _src = ReadFixed(_src, func);
    int ref; // literal
    _src = ReadFixed<int>(_src, ref);
    unsigned int mask; // literal
    _src = ReadFixed<unsigned int>(_src, mask);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glStencilFuncSeparate(face, func, ref, mask);
    // ---------- register handles ------------
    gRetracer.curContextState->curGlobalState.ff_glStencilFuncSeparate(face, gRetracer.mCurCallNo);
}

static void retrace_glStencilMask(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int mask; // literal
    _src = ReadFixed<unsigned int>(_src, mask);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glStencilMask(mask);
    // ---------- register handles ------------
    //gRetracer.curContextState->curGlobalState.ff_glStencilMask(gRetracer.mCurCallNo);
    gRetracer.curContextState->curGlobalState.ff_glStencilMaskSeparate(GL_FRONT_AND_BACK, gRetracer.mCurCallNo);
}

static void retrace_glStencilMaskSeparate(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int face; // enum
    _src = ReadFixed(_src, face);
    unsigned int mask; // literal
    _src = ReadFixed<unsigned int>(_src, mask);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glStencilMaskSeparate(face, mask);
    // ---------- register handles ------------
    gRetracer.curContextState->curGlobalState.ff_glStencilMaskSeparate(face, gRetracer.mCurCallNo);
}

static void retrace_glStencilOp(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int fail; // enum
    _src = ReadFixed(_src, fail);
    int zfail; // enum
    _src = ReadFixed(_src, zfail);
    int zpass; // enum
    _src = ReadFixed(_src, zpass);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glStencilOp(fail, zfail, zpass);
    // ---------- register handles ------------
    gRetracer.curContextState->curGlobalState.ff_glStencilOpSeparate(GL_FRONT_AND_BACK, gRetracer.mCurCallNo);
}

static void retrace_glStencilOpSeparate(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int face; // enum
    _src = ReadFixed(_src, face);
    int sfail; // enum
    _src = ReadFixed(_src, sfail);
    int dpfail; // enum
    _src = ReadFixed(_src, dpfail);
    int dppass; // enum
    _src = ReadFixed(_src, dppass);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glStencilOpSeparate(face, sfail, dpfail, dppass);
    // ---------- register handles ------------
    gRetracer.curContextState->curGlobalState.ff_glStencilOpSeparate(face, gRetracer.mCurCallNo);
}

static void retrace_glTexImage2D(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);
    int border; // literal
    _src = ReadFixed<int>(_src, border);
    int format; // enum
    _src = ReadFixed(_src, format);
    int type; // enum
    _src = ReadFixed(_src, type);
    /*   GLvoid* pixels = NULL;
    Array<char> pixelsBlob;
    //if (gRetracer.getFileFormatVersion() <= HEADER_VERSION_3)
    // {
    //     //  _src = Read1DArray(_src, pixelsBlob);
    //    pixels = pixelsBlob.v;
    //  }
    //    else
    {
        unsigned int _opaque_type = 0;
        _src = ReadFixed(_src, _opaque_type);
        switch (_opaque_type)
        {
        case BlobType:
            _src = Read1DArray(_src, pixelsBlob);
            pixels = pixelsBlob.v;
            break;
        case BufferObjectReferenceType:
            unsigned int bufferOffset = 0;
            _src = ReadFixed<unsigned int>(_src, bufferOffset);
            pixels = (GLvoid *)static_cast<uintptr_t>(bufferOffset);
            break;
        }
    }
    */
    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
#ifndef NDEBUG
    //   gRetracer.mTextureDataSize += _gl_image_size(format, type, width, height, 1);
#endif
    // ------------- retrace ------------------
    //    glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
    // ---------- register handles ------------

    //for fastforwad
    std::unordered_set<unsigned int> newList;
    unsigned int GL_PIXEL_UNPACK_BUFFERNo = gRetracer.curContextState->curBufferState.getCurGL_PIXEL_UNPACK_BUFFERNo();
    unsigned int GL_PIXEL_UNPACK_BUFFERIdx = gRetracer.curContextState->curBufferState.getCurGL_PIXEL_UNPACK_BUFFERIdx();
    if(GL_PIXEL_UNPACK_BUFFERIdx!=0)
        {
            gRetracer.curContextState->curBufferState.findABufferState(GL_PIXEL_UNPACK_BUFFERIdx, newList);
        }

    gRetracer.curContextState->curTextureState.ff_glTypes_TexImage2D(target, GL_PIXEL_UNPACK_BUFFERNo, gRetracer.mCurCallNo, newList);
}

static void retrace_glTexParameterf(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    float param; // literal
    _src = ReadFixed<float>(_src, param);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glTexParameterf(target, pname, param);
    // ---------- register handles ------------
    gRetracer.curContextState->curTextureState.ff_glTexParameter_Types(target, pname, gRetracer.mCurCallNo);
}

static void retrace_glTexParameterfv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<float> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glTexParameterfv(target, pname, params);
    // ---------- register handles ------------
}

static void retrace_glTexParameteri(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    int param; // literal
    _src = ReadFixed<int>(_src, param);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glTexParameteri(target, pname, param);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curTextureState.ff_glTexParameter_Types(target, pname, gRetracer.mCurCallNo);
    //gRetracer.curContextState->curGlobalState.debug(gRetracer.mCurCallNo);
}

static void retrace_glTexParameteriv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<int> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glTexParameteriv(target, pname, params);
    // ---------- register handles ------------
}

static void retrace_glTexSubImage2D(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int xoffset; // literal
    _src = ReadFixed<int>(_src, xoffset);
    int yoffset; // literal
    _src = ReadFixed<int>(_src, yoffset);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);
    int format; // enum
    _src = ReadFixed(_src, format);
    int type; // enum
    _src = ReadFixed(_src, type);
    /*   GLvoid* pixels = NULL;
    Array<char> pixelsBlob;
    //   if (gRetracer.getFileFormatVersion() <= HEADER_VERSION_3)
    //  {
    //     _src = Read1DArray(_src, pixelsBlob);
    //    pixels = pixelsBlob.v;
    // }
    //else
    {
        unsigned int _opaque_type = 0;
        _src = ReadFixed(_src, _opaque_type);
        switch (_opaque_type)
        {
        case BlobType:
            _src = Read1DArray(_src, pixelsBlob);
            pixels = pixelsBlob.v;
            break;
        case BufferObjectReferenceType:
            unsigned int bufferOffset = 0;
            _src = ReadFixed<unsigned int>(_src, bufferOffset);
            pixels = (GLvoid *)static_cast<uintptr_t>(bufferOffset);
            break;
        }
    }
    */
    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
#ifndef NDEBUG
    //   gRetracer.mTextureDataSize += _gl_image_size(format, type, width, height, 1);
#endif
    // ------------- retrace ------------------
    //    glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
    // ---------- register handles ------------


    //for fastforwad
    std::unordered_set<unsigned int> newList;
    unsigned int GL_PIXEL_UNPACK_BUFFERNo = gRetracer.curContextState->curBufferState.getCurGL_PIXEL_UNPACK_BUFFERNo();
    unsigned int GL_PIXEL_UNPACK_BUFFERIdx = gRetracer.curContextState->curBufferState.getCurGL_PIXEL_UNPACK_BUFFERIdx();
    if(GL_PIXEL_UNPACK_BUFFERIdx!=0)
    {
        gRetracer.curContextState->curBufferState.findABufferState(GL_PIXEL_UNPACK_BUFFERIdx, newList);
    }
    gRetracer.curContextState->curTextureState.ff_glTypes_TexSubImage2D(target, GL_PIXEL_UNPACK_BUFFERNo , gRetracer.mCurCallNo, newList);
}

static void retrace_glUniform1f(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    float v0; // literal
    _src = ReadFixed<float>(_src, v0);

    // --------- assistant parameters ---------
    //   GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    /*   if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }*/
    // ----------- look up handles ------------
    //  Context& context = gRetracer.getCurrentContext();
    //   GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glUniform1f(locationNew, v0);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curProgramState.ff_glUniform_Types(location, gRetracer.mCurCallNo);
}

static void retrace_glUniform2f(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    float v0; // literal
    _src = ReadFixed<float>(_src, v0);
    float v1; // literal
    _src = ReadFixed<float>(_src, v1);

    // --------- assistant parameters ---------
    /*    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform2f(locationNew, v0, v1);
    // ---------- register handles ------------
    */
    gRetracer.curContextState->curProgramState.ff_glUniform_Types(location, gRetracer.mCurCallNo);
}

static void retrace_glUniform3f(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    float v0; // literal
    _src = ReadFixed<float>(_src, v0);
    float v1; // literal
    _src = ReadFixed<float>(_src, v1);
    float v2; // literal
    _src = ReadFixed<float>(_src, v2);

    // --------- assistant parameters ---------
    /*    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform3f(locationNew, v0, v1, v2);
    // ---------- register handles ------------
    */
    gRetracer.curContextState->curProgramState.ff_glUniform_Types(location, gRetracer.mCurCallNo);
}

static void retrace_glUniform4f(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    float v0; // literal
    _src = ReadFixed<float>(_src, v0);
    float v1; // literal
    _src = ReadFixed<float>(_src, v1);
    float v2; // literal
    _src = ReadFixed<float>(_src, v2);
    float v3; // literal
    _src = ReadFixed<float>(_src, v3);

    // --------- assistant parameters ---------
    /*   GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform4f(locationNew, v0, v1, v2, v3);
    // ---------- register handles ------------
    */
    gRetracer.curContextState->curProgramState.ff_glUniform_Types(location, gRetracer.mCurCallNo);
}

static void retrace_glUniform1i(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int v0; // literal
    _src = ReadFixed<int>(_src, v0);

    // --------- assistant parameters ---------
    /*    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }*/
    // ----------- look up handles ------------
    //    Context& context = gRetracer.getCurrentContext();
    //   GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glUniform1i(locationNew, v0);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curProgramState.ff_glUniform_Types(location, gRetracer.mCurCallNo);
}

static void retrace_glUniform2i(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int v0; // literal
    _src = ReadFixed<int>(_src, v0);
    int v1; // literal
    _src = ReadFixed<int>(_src, v1);

    // --------- assistant parameters ---------
    /*    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform2i(locationNew, v0, v1);
    // ---------- register handles ------------
    */
    //for fastforwad
    gRetracer.curContextState->curProgramState.ff_glUniform_Types(location, gRetracer.mCurCallNo);
}

static void retrace_glUniform3i(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int v0; // literal
    _src = ReadFixed<int>(_src, v0);
    int v1; // literal
    _src = ReadFixed<int>(_src, v1);
    int v2; // literal
    _src = ReadFixed<int>(_src, v2);

    // --------- assistant parameters ---------
    /*   GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform3i(locationNew, v0, v1, v2);
    // ---------- register handles ------------
    */
}

static void retrace_glUniform4i(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int v0; // literal
    _src = ReadFixed<int>(_src, v0);
    int v1; // literal
    _src = ReadFixed<int>(_src, v1);
    int v2; // literal
    _src = ReadFixed<int>(_src, v2);
    int v3; // literal
    _src = ReadFixed<int>(_src, v3);

    /*   // --------- assistant parameters ---------
    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform4i(locationNew, v0, v1, v2, v3);
    // ---------- register handles ------------
    */
}

static void retrace_glUniform1fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<float> value; // array
    _src = Read1DArray(_src, value);

    // --------- assistant parameters ---------
    /*    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform1fv(locationNew, count, value);
    // ---------- register handles ------------
    */
    gRetracer.curContextState->curProgramState.ff_glUniform_Types(location+count*UNIFORM_COUNT_BASE, gRetracer.mCurCallNo);
}

static void retrace_glUniform2fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<float> value; // array
    _src = Read1DArray(_src, value);

    // --------- assistant parameters ---------
    /*   GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform2fv(locationNew, count, value);
    // ---------- register handles ------------
    */
    gRetracer.curContextState->curProgramState.ff_glUniform_Types(location+count*UNIFORM_COUNT_BASE, gRetracer.mCurCallNo);
}

static void retrace_glUniform3fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<float> value; // array
    _src = Read1DArray(_src, value);

    // --------- assistant parameters ---------
    /*     GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform3fv(locationNew, count, value);
    // ---------- register handles ------------
    */
    gRetracer.curContextState->curProgramState.ff_glUniform_Types(location+count*UNIFORM_COUNT_BASE, gRetracer.mCurCallNo);
}

static void retrace_glUniform4fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<float> value; // array
    _src = Read1DArray(_src, value);

    // --------- assistant parameters ---------
    /*    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform4fv(locationNew, count, value);
    // ---------- register handles ------------
    */
    if(value != NULL)
    {
        gRetracer.curContextState->curProgramState.ff_glUniform_Types(location+count*UNIFORM_COUNT_BASE, gRetracer.mCurCallNo);
    }
}

static void retrace_glUniform1iv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<int> value; // array
    _src = Read1DArray(_src, value);

    // --------- assistant parameters ---------
    /*    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform1iv(locationNew, count, value);
    // ---------- register handles ------------
    */
    gRetracer.curContextState->curProgramState.ff_glUniform_Types(location, gRetracer.mCurCallNo);
}

static void retrace_glUniform2iv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<int> value; // array
    _src = Read1DArray(_src, value);

    // --------- assistant parameters ---------
    /*    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform2iv(locationNew, count, value);
    // ---------- register handles ------------
    */
}

static void retrace_glUniform3iv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<int> value; // array
    _src = Read1DArray(_src, value);

    /*   // --------- assistant parameters ---------
    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform3iv(locationNew, count, value);
    // ---------- register handles ------------
    */
}

static void retrace_glUniform4iv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<int> value; // array
    _src = Read1DArray(_src, value);

    // --------- assistant parameters ---------
    /*   GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform4iv(locationNew, count, value);
    // ---------- register handles ------------
    */
    gRetracer.curContextState->curProgramState.ff_glUniform_Types(location, gRetracer.mCurCallNo);
}

static void retrace_glUniformMatrix2fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    unsigned char transpose; // literal
    _src = ReadFixed<unsigned char>(_src, transpose);
    Array<float> value; // array
    _src = Read1DArray(_src, value);

    // --------- assistant parameters ---------
    /*    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniformMatrix2fv(locationNew, count, transpose, value);
    // ---------- register handles ------------
    */
    gRetracer.curContextState->curProgramState.ff_glUniform_Types(location, gRetracer.mCurCallNo);
}

static void retrace_glUniformMatrix3fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    unsigned char transpose; // literal
    _src = ReadFixed<unsigned char>(_src, transpose);
    Array<float> value; // array
    _src = Read1DArray(_src, value);

    /*    // --------- assistant parameters ---------
    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniformMatrix3fv(locationNew, count, transpose, value);
    // ---------- register handles ------------
    */
    gRetracer.curContextState->curProgramState.ff_glUniform_Types(location, gRetracer.mCurCallNo);
}

static void retrace_glUniformMatrix4fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    unsigned char transpose; // literal
    _src = ReadFixed<unsigned char>(_src, transpose);
    Array<float> value; // array
    _src = Read1DArray(_src, value);

    // --------- assistant parameters ---------
    /*    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }*/
    // ----------- look up handles ------------
    //    Context& context = gRetracer.getCurrentContext();
    //    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glUniformMatrix4fv(locationNew, count, transpose, value);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curProgramState.ff_glUniform_Types(location+count*UNIFORM_COUNT_BASE, gRetracer.mCurCallNo);
}

static void retrace_glUseProgram(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);

    // ----------- look up handles ------------
    //    Context& context = gRetracer.getCurrentContext();
    //    GLuint programNew = context.getProgramMap().RValue(program);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    //   gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program = programNew;
    // ------------- retrace ------------------
    //    glUseProgram(programNew);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curProgramState.ff_glUseProgram(program, gRetracer.mCurCallNo);
}

static void retrace_glValidateProgram(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);

    // ----------- look up handles ------------
    //    Context& context = gRetracer.getCurrentContext();
    //    GLuint programNew = context.getProgramMap().RValue(program);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glValidateProgram(programNew);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curProgramState.ff_glValidateProgram(program, gRetracer.mCurCallNo);
}

static void retrace_glVertexAttrib1f(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    float x; // literal
    _src = ReadFixed<float>(_src, x);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glVertexAttrib1f(index, x);
    // ---------- register handles ------------
}

static void retrace_glVertexAttrib2f(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    float x; // literal
    _src = ReadFixed<float>(_src, x);
    float y; // literal
    _src = ReadFixed<float>(_src, y);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glVertexAttrib2f(index, x, y);
    // ---------- register handles ------------
}

static void retrace_glVertexAttrib3f(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    float x; // literal
    _src = ReadFixed<float>(_src, x);
    float y; // literal
    _src = ReadFixed<float>(_src, y);
    float z; // literal
    _src = ReadFixed<float>(_src, z);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glVertexAttrib3f(index, x, y, z);
    // ---------- register handles ------------
}

static void retrace_glVertexAttrib4f(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    float x; // literal
    _src = ReadFixed<float>(_src, x);
    float y; // literal
    _src = ReadFixed<float>(_src, y);
    float z; // literal
    _src = ReadFixed<float>(_src, z);
    float w; // literal
    _src = ReadFixed<float>(_src, w);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glVertexAttrib4f(index, x, y, z, w);
    // ---------- register handles ------------
    //gRetracer.curContextState->curGlobalState.ff_glVertexAttribPointer(index, gRetracer.mCurCallNo, NO_IDX,0,0);

    gRetracer.curContextState->curGlobalState.ff_glVertexAttrib_Types(index, gRetracer.mCurCallNo);
}

static void retrace_glVertexAttrib1fv(char* _src)
{
    //DBG_LOG("glVertexAttrib1fv is not supported\n");
}

static void retrace_glVertexAttrib2fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    Array<float> v; // array
    _src = Read1DArray(_src, v);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glVertexAttrib2fv(index, v);
    // ---------- register handles ------------
}

static void retrace_glVertexAttrib3fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    Array<float> v; // array
    _src = Read1DArray(_src, v);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glVertexAttrib3fv(index, v);
    // ---------- register handles ------------
}

static void retrace_glVertexAttrib4fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    Array<float> v; // array
    _src = Read1DArray(_src, v);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glVertexAttrib4fv(index, v);
    // ---------- register handles ------------
    //gRetracer.curContextState->curGlobalState.ff_glVertexAttribPointer(index, gRetracer.mCurCallNo, NO_IDX,0,0);

    gRetracer.curContextState->curGlobalState.ff_glVertexAttrib_Types(index, gRetracer.mCurCallNo);
}

static void retrace_glVertexAttribPointer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    int size; // literal
    _src = ReadFixed<int>(_src, size);
    int type; // enum
    _src = ReadFixed(_src, type);
    unsigned char normalized; // literal
    _src = ReadFixed<unsigned char>(_src, normalized);
    int stride; // literal
    _src = ReadFixed<int>(_src, stride);
    unsigned int csbName = NO_IDX; // ClientSideBuffer name
    //   GLvoid* pointer;// = NULL;
    //   pointer = NULL;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    if (_opaque_type == BufferObjectReferenceType)
    { // simple memory offset
        unsigned int pointer_raw; // raw ptr
        _src = ReadFixed<unsigned int>(_src, pointer_raw);
        //     pointer = (GLvoid *)static_cast<uintptr_t>(pointer_raw);
    }
    else if (_opaque_type == BlobType)
    { // blob type
        Array<float> pointer_blob;
        _src = Read1DArray(_src, pointer_blob);
        //      pointer = pointer_blob.v;
    }
    else if (_opaque_type == ClientSideBufferObjectReferenceType)
    { // a memory reference to client-side buffer
        unsigned int offset = 0;
        _src = ReadFixed<unsigned int>(_src, csbName);
        _src = ReadFixed<unsigned int>(_src, offset);
        //     pointer = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);
    }
    else if (_opaque_type == NoopType)
    { // Do nothing
    }
    //if(csbName == 37)printf("csb  %d  callNo  %d\n",csbName, gRetracer.mCurCallNo);
    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glVertexAttribPointer(index, size, type, normalized, stride, pointer);
    // ---------- register handles ------------

    //for fastforwad
    unsigned int bindbufferIdx = gRetracer.curContextState->curBufferState.getCurGL_ARRAY_BUFFERIdx();
    unsigned int bindbufferNo = gRetracer.curContextState->curBufferState.getCurGL_ARRAY_BUFFERNo();
    gRetracer.curContextState->curGlobalState.ff_glVertexAttribPointer(index, gRetracer.mCurCallNo, csbName, bindbufferIdx, bindbufferNo);
    gRetracer.curContextState->curBufferState.ff_glVertexAttribPointerForVao(index, gRetracer.mCurCallNo, csbName, bindbufferIdx, bindbufferNo);
}

static void retrace_glViewport(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int x; // literal
    _src = ReadFixed<int>(_src, x);
    int y; // literal
    _src = ReadFixed<int>(_src, y);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    /*    gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppVP.x = x;
    gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppVP.y = y;
    gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppVP.w = width;
    gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppVP.h = height;*/
    // ------------- retrace ------------------
    /*   if (!gRetracer.mOptions.mDoOverrideResolution)
    {
        glViewport(x, y, width, height);
    }*/
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curGlobalState.ff_glViewport(gRetracer.mCurCallNo);
}

static void retrace_glAlphaFunc(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int func; // enum
    _src = ReadFixed(_src, func);
    float ref; // literal
    _src = ReadFixed<float>(_src, ref);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glAlphaFunc(func, ref);
    // ---------- register handles ------------
}

static void retrace_glAlphaFuncx(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int func; // enum
    _src = ReadFixed(_src, func);
    int32_t ref; // literal
    _src = ReadFixed<int32_t>(_src, ref);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glAlphaFuncx(func, ref);
    // ---------- register handles ------------
}

static void retrace_glClearColorx(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int32_t red; // literal
    _src = ReadFixed<int32_t>(_src, red);
    int32_t green; // literal
    _src = ReadFixed<int32_t>(_src, green);
    int32_t blue; // literal
    _src = ReadFixed<int32_t>(_src, blue);
    int32_t alpha; // literal
    _src = ReadFixed<int32_t>(_src, alpha);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glClearColorx(red, green, blue, alpha);
    // ---------- register handles ------------
}

static void retrace_glClearDepthx(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int32_t depth; // literal
    _src = ReadFixed<int32_t>(_src, depth);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glClearDepthx(depth);
    // ---------- register handles ------------
}

static void retrace_glClientActiveTexture(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int texture; // enum
    _src = ReadFixed(_src, texture);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glClientActiveTexture(texture);
    // ---------- register handles ------------
}

static void retrace_glClipPlanef(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int plane; // enum
    _src = ReadFixed(_src, plane);
    Array<float> equation; // array
    _src = Read1DArray(_src, equation);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glClipPlanef(plane, equation);
    // ---------- register handles ------------
}

static void retrace_glClipPlanex(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int plane; // enum
    _src = ReadFixed(_src, plane);
    Array<int32_t> equation; // array
    _src = Read1DArray(_src, equation);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glClipPlanex(plane, equation);
    // ---------- register handles ------------
}

static void retrace_glColor4f(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    float red; // literal
    _src = ReadFixed<float>(_src, red);
    float green; // literal
    _src = ReadFixed<float>(_src, green);
    float blue; // literal
    _src = ReadFixed<float>(_src, blue);
    float alpha; // literal
    _src = ReadFixed<float>(_src, alpha);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glColor4f(red, green, blue, alpha);
    // ---------- register handles ------------
}

static void retrace_glColor4ub(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned char red; // literal
    _src = ReadFixed<unsigned char>(_src, red);
    unsigned char green; // literal
    _src = ReadFixed<unsigned char>(_src, green);
    unsigned char blue; // literal
    _src = ReadFixed<unsigned char>(_src, blue);
    unsigned char alpha; // literal
    _src = ReadFixed<unsigned char>(_src, alpha);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glColor4ub(red, green, blue, alpha);
    // ---------- register handles ------------
}

static void retrace_glColor4x(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int32_t red; // literal
    _src = ReadFixed<int32_t>(_src, red);
    int32_t green; // literal
    _src = ReadFixed<int32_t>(_src, green);
    int32_t blue; // literal
    _src = ReadFixed<int32_t>(_src, blue);
    int32_t alpha; // literal
    _src = ReadFixed<int32_t>(_src, alpha);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glColor4x(red, green, blue, alpha);
    // ---------- register handles ------------
}

static void retrace_glColorPointer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int size; // literal
    _src = ReadFixed<int>(_src, size);
    int type; // enum
    _src = ReadFixed(_src, type);
    int stride; // literal
    _src = ReadFixed<int>(_src, stride);
    unsigned int csbName = 0; // ClientSideBuffer name
    //  GLvoid* pointer = NULL;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    if (_opaque_type == BufferObjectReferenceType)
    { // simple memory offset
        unsigned int pointer_raw; // raw ptr
        _src = ReadFixed<unsigned int>(_src, pointer_raw);
        //       pointer = (GLvoid *)static_cast<uintptr_t>(pointer_raw);
    }
    else if (_opaque_type == BlobType)
    { // blob type
        Array<float> pointer_blob;
        _src = Read1DArray(_src, pointer_blob);
        //      pointer = pointer_blob.v;
    }
    else if (_opaque_type == ClientSideBufferObjectReferenceType)
    { // a memory reference to client-side buffer
        unsigned int offset = 0;
        _src = ReadFixed<unsigned int>(_src, csbName);
        _src = ReadFixed<unsigned int>(_src, offset);
        //     pointer = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);
    }
    else if (_opaque_type == NoopType)
    { // Do nothing
    }

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glColorPointer(size, type, stride, pointer);
    // ---------- register handles ------------
}

static void retrace_glDepthRangex(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int32_t zNear; // literal
    _src = ReadFixed<int32_t>(_src, zNear);
    int32_t zFar; // literal
    _src = ReadFixed<int32_t>(_src, zFar);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glDepthRangex(zNear, zFar);
    // ---------- register handles ------------
}

static void retrace_glDisableClientState(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int array; // enum
    _src = ReadFixed(_src, array);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glDisableClientState(array);
    // ---------- register handles ------------
}

static void retrace_glEnableClientState(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int array; // enum
    _src = ReadFixed(_src, array);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glEnableClientState(array);
    // ---------- register handles ------------
}

static void retrace_glFogf(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int pname; // enum
    _src = ReadFixed(_src, pname);
    float param; // literal
    _src = ReadFixed<float>(_src, param);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glFogf(pname, param);
    // ---------- register handles ------------
}

static void retrace_glFogx(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int pname; // enum
    _src = ReadFixed(_src, pname);
    int32_t param; // literal
    _src = ReadFixed<int32_t>(_src, param);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glFogx(pname, param);
    // ---------- register handles ------------
}

static void retrace_glFogfv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<float> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glFogfv(pname, params);
    // ---------- register handles ------------
}

static void retrace_glFogxv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<int32_t> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glFogxv(pname, params);
    // ---------- register handles ------------
}

static void retrace_glFrustumf(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    float left; // literal
    _src = ReadFixed<float>(_src, left);
    float right; // literal
    _src = ReadFixed<float>(_src, right);
    float bottom; // literal
    _src = ReadFixed<float>(_src, bottom);
    float top; // literal
    _src = ReadFixed<float>(_src, top);
    float zNear; // literal
    _src = ReadFixed<float>(_src, zNear);
    float zFar; // literal
    _src = ReadFixed<float>(_src, zFar);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glFrustumf(left, right, bottom, top, zNear, zFar);
    // ---------- register handles ------------
}

static void retrace_glFrustumx(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int32_t left; // literal
    _src = ReadFixed<int32_t>(_src, left);
    int32_t right; // literal
    _src = ReadFixed<int32_t>(_src, right);
    int32_t bottom; // literal
    _src = ReadFixed<int32_t>(_src, bottom);
    int32_t top; // literal
    _src = ReadFixed<int32_t>(_src, top);
    int32_t zNear; // literal
    _src = ReadFixed<int32_t>(_src, zNear);
    int32_t zFar; // literal
    _src = ReadFixed<int32_t>(_src, zFar);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glFrustumx(left, right, bottom, top, zNear, zFar);
    // ---------- register handles ------------
}

static void retrace_glLightf(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int light; // enum
    _src = ReadFixed(_src, light);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    float param; // literal
    _src = ReadFixed<float>(_src, param);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glLightf(light, pname, param);
    // ---------- register handles ------------
}

static void retrace_glLightfv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int light; // enum
    _src = ReadFixed(_src, light);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<float> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glLightfv(light, pname, params);
    // ---------- register handles ------------
}

static void retrace_glLightx(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int light; // enum
    _src = ReadFixed(_src, light);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    int32_t param; // literal
    _src = ReadFixed<int32_t>(_src, param);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glLightx(light, pname, param);
    // ---------- register handles ------------
}

static void retrace_glLightxv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int light; // enum
    _src = ReadFixed(_src, light);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<int32_t> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glLightxv(light, pname, params);
    // ---------- register handles ------------
}

static void retrace_glLightModelf(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int pname; // enum
    _src = ReadFixed(_src, pname);
    float param; // literal
    _src = ReadFixed<float>(_src, param);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glLightModelf(pname, param);
    // ---------- register handles ------------
}

static void retrace_glLightModelx(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int pname; // enum
    _src = ReadFixed(_src, pname);
    int32_t param; // literal
    _src = ReadFixed<int32_t>(_src, param);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glLightModelx(pname, param);
    // ---------- register handles ------------
}

static void retrace_glLightModelfv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<float> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glLightModelfv(pname, params);
    // ---------- register handles ------------
}

static void retrace_glLightModelxv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<int32_t> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glLightModelxv(pname, params);
    // ---------- register handles ------------
}

static void retrace_glLineWidthx(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int32_t width; // literal
    _src = ReadFixed<int32_t>(_src, width);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glLineWidthx(width);
    // ---------- register handles ------------
}

static void retrace_glLoadIdentity(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glLoadIdentity();
    // ---------- register handles ------------
}

static void retrace_glLoadMatrixf(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    Array<float> m; // array
    _src = Read1DArray(_src, m);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glLoadMatrixf(m);
    // ---------- register handles ------------
}

static void retrace_glLoadMatrixx(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    Array<int32_t> m; // array
    _src = Read1DArray(_src, m);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glLoadMatrixx(m);
    // ---------- register handles ------------
}

static void retrace_glLogicOp(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int opcode; // enum
    _src = ReadFixed(_src, opcode);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glLogicOp(opcode);
    // ---------- register handles ------------
}

static void retrace_glMaterialf(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int face; // enum
    _src = ReadFixed(_src, face);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    float param; // literal
    _src = ReadFixed<float>(_src, param);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glMaterialf(face, pname, param);
    // ---------- register handles ------------
}

static void retrace_glMaterialx(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int face; // enum
    _src = ReadFixed(_src, face);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    int32_t param; // literal
    _src = ReadFixed<int32_t>(_src, param);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glMaterialx(face, pname, param);
    // ---------- register handles ------------
}

static void retrace_glMaterialfv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int face; // enum
    _src = ReadFixed(_src, face);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<float> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glMaterialfv(face, pname, params);
    // ---------- register handles ------------
}

static void retrace_glMaterialxv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int face; // enum
    _src = ReadFixed(_src, face);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<int32_t> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glMaterialxv(face, pname, params);
    // ---------- register handles ------------
}

static void retrace_glMatrixMode(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int mode; // enum
    _src = ReadFixed(_src, mode);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glMatrixMode(mode);
    // ---------- register handles ------------
}

static void retrace_glMultMatrixf(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    Array<float> m; // array
    _src = Read1DArray(_src, m);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glMultMatrixf(m);
    // ---------- register handles ------------
}

static void retrace_glMultMatrixx(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    Array<int32_t> m; // array
    _src = Read1DArray(_src, m);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glMultMatrixx(m);
    // ---------- register handles ------------
}

static void retrace_glMultiTexCoord4f(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    float s; // literal
    _src = ReadFixed<float>(_src, s);
    float t; // literal
    _src = ReadFixed<float>(_src, t);
    float r; // literal
    _src = ReadFixed<float>(_src, r);
    float q; // literal
    _src = ReadFixed<float>(_src, q);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glMultiTexCoord4f(target, s, t, r, q);
    // ---------- register handles ------------
}

static void retrace_glMultiTexCoord4x(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int32_t s; // literal
    _src = ReadFixed<int32_t>(_src, s);
    int32_t t; // literal
    _src = ReadFixed<int32_t>(_src, t);
    int32_t r; // literal
    _src = ReadFixed<int32_t>(_src, r);
    int32_t q; // literal
    _src = ReadFixed<int32_t>(_src, q);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glMultiTexCoord4x(target, s, t, r, q);
    // ---------- register handles ------------
}

static void retrace_glNormal3f(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    float nx; // literal
    _src = ReadFixed<float>(_src, nx);
    float ny; // literal
    _src = ReadFixed<float>(_src, ny);
    float nz; // literal
    _src = ReadFixed<float>(_src, nz);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glNormal3f(nx, ny, nz);
    // ---------- register handles ------------
}

static void retrace_glNormal3x(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int32_t nx; // literal
    _src = ReadFixed<int32_t>(_src, nx);
    int32_t ny; // literal
    _src = ReadFixed<int32_t>(_src, ny);
    int32_t nz; // literal
    _src = ReadFixed<int32_t>(_src, nz);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glNormal3x(nx, ny, nz);
    // ---------- register handles ------------
}

static void retrace_glNormalPointer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int type; // enum
    _src = ReadFixed(_src, type);
    int stride; // literal
    _src = ReadFixed<int>(_src, stride);
    unsigned int csbName = 0; // ClientSideBuffer name
    //  GLvoid* pointer = NULL;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    if (_opaque_type == BufferObjectReferenceType)
    { // simple memory offset
        unsigned int pointer_raw; // raw ptr
        _src = ReadFixed<unsigned int>(_src, pointer_raw);
        //     pointer = (GLvoid *)static_cast<uintptr_t>(pointer_raw);
    }
    else if (_opaque_type == BlobType)
    { // blob type
        Array<float> pointer_blob;
        _src = Read1DArray(_src, pointer_blob);
  //      pointer = pointer_blob.v;
    }
    else if (_opaque_type == ClientSideBufferObjectReferenceType)
    { // a memory reference to client-side buffer
        unsigned int offset = 0;
        _src = ReadFixed<unsigned int>(_src, csbName);
        _src = ReadFixed<unsigned int>(_src, offset);
   //     pointer = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);
    }
    else if (_opaque_type == NoopType)
    { // Do nothing
    }

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glNormalPointer(type, stride, pointer);
    // ---------- register handles ------------
}

static void retrace_glOrthof(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    float left; // literal
    _src = ReadFixed<float>(_src, left);
    float right; // literal
    _src = ReadFixed<float>(_src, right);
    float bottom; // literal
    _src = ReadFixed<float>(_src, bottom);
    float top; // literal
    _src = ReadFixed<float>(_src, top);
    float zNear; // literal
    _src = ReadFixed<float>(_src, zNear);
    float zFar; // literal
    _src = ReadFixed<float>(_src, zFar);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glOrthof(left, right, bottom, top, zNear, zFar);
    // ---------- register handles ------------
}

static void retrace_glOrthox(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int32_t left; // literal
    _src = ReadFixed<int32_t>(_src, left);
    int32_t right; // literal
    _src = ReadFixed<int32_t>(_src, right);
    int32_t bottom; // literal
    _src = ReadFixed<int32_t>(_src, bottom);
    int32_t top; // literal
    _src = ReadFixed<int32_t>(_src, top);
    int32_t zNear; // literal
    _src = ReadFixed<int32_t>(_src, zNear);
    int32_t zFar; // literal
    _src = ReadFixed<int32_t>(_src, zFar);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glOrthox(left, right, bottom, top, zNear, zFar);
    // ---------- register handles ------------
}

static void retrace_glPointParameterf(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int pname; // enum
    _src = ReadFixed(_src, pname);
    float param; // literal
    _src = ReadFixed<float>(_src, param);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glPointParameterf(pname, param);
    // ---------- register handles ------------
}

static void retrace_glPointParameterx(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int pname; // enum
    _src = ReadFixed(_src, pname);
    int32_t param; // literal
    _src = ReadFixed<int32_t>(_src, param);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glPointParameterx(pname, param);
    // ---------- register handles ------------
}

static void retrace_glPointParameterfv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<float> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glPointParameterfv(pname, params);
    // ---------- register handles ------------
}

static void retrace_glPointParameterxv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<int32_t> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glPointParameterxv(pname, params);
    // ---------- register handles ------------
}

static void retrace_glPointSize(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    float size; // literal
    _src = ReadFixed<float>(_src, size);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glPointSize(size);
    // ---------- register handles ------------
}

static void retrace_glPointSizex(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int32_t size; // literal
    _src = ReadFixed<int32_t>(_src, size);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glPointSizex(size);
    // ---------- register handles ------------
}

static void retrace_glPointSizePointerOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int type; // enum
    _src = ReadFixed(_src, type);
    int stride; // literal
    _src = ReadFixed<int>(_src, stride);
    unsigned int csbName = 0; // ClientSideBuffer name
    //  GLvoid* ptr = NULL;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    if (_opaque_type == BufferObjectReferenceType)
    { // simple memory offset
        unsigned int ptr_raw; // raw ptr
        _src = ReadFixed<unsigned int>(_src, ptr_raw);
        //      ptr = (GLvoid *)static_cast<uintptr_t>(ptr_raw);
    }
    else if (_opaque_type == BlobType)
    { // blob type
        Array<float> ptr_blob;
        _src = Read1DArray(_src, ptr_blob);
        //     ptr = ptr_blob.v;
    }
    else if (_opaque_type == ClientSideBufferObjectReferenceType)
    { // a memory reference to client-side buffer
        unsigned int offset = 0;
        _src = ReadFixed<unsigned int>(_src, csbName);
        _src = ReadFixed<unsigned int>(_src, offset);
        //    ptr = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);
    }
    else if (_opaque_type == NoopType)
    { // Do nothing
    }

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glPointSizePointerOES(type, stride, ptr);
    // ---------- register handles ------------
}

static void retrace_glPolygonOffsetx(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int32_t factor; // literal
    _src = ReadFixed<int32_t>(_src, factor);
    int32_t units; // literal
    _src = ReadFixed<int32_t>(_src, units);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glPolygonOffsetx(factor, units);
    // ---------- register handles ------------
}

static void retrace_glPopMatrix(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glPopMatrix();
    // ---------- register handles ------------
}

static void retrace_glPushMatrix(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glPushMatrix();
    // ---------- register handles ------------
}

static void retrace_glRotatef(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    float angle; // literal
    _src = ReadFixed<float>(_src, angle);
    float x; // literal
    _src = ReadFixed<float>(_src, x);
    float y; // literal
    _src = ReadFixed<float>(_src, y);
    float z; // literal
    _src = ReadFixed<float>(_src, z);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glRotatef(angle, x, y, z);
    // ---------- register handles ------------
}

static void retrace_glRotatex(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int32_t angle; // literal
    _src = ReadFixed<int32_t>(_src, angle);
    int32_t x; // literal
    _src = ReadFixed<int32_t>(_src, x);
    int32_t y; // literal
    _src = ReadFixed<int32_t>(_src, y);
    int32_t z; // literal
    _src = ReadFixed<int32_t>(_src, z);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glRotatex(angle, x, y, z);
    // ---------- register handles ------------
}

static void retrace_glSampleCoveragex(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int32_t value; // literal
    _src = ReadFixed<int32_t>(_src, value);
    unsigned char invert; // literal
    _src = ReadFixed<unsigned char>(_src, invert);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glSampleCoveragex(value, invert);
    // ---------- register handles ------------
}

static void retrace_glScalef(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    float x; // literal
    _src = ReadFixed<float>(_src, x);
    float y; // literal
    _src = ReadFixed<float>(_src, y);
    float z; // literal
    _src = ReadFixed<float>(_src, z);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glScalef(x, y, z);
    // ---------- register handles ------------
}

static void retrace_glScalex(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int32_t x; // literal
    _src = ReadFixed<int32_t>(_src, x);
    int32_t y; // literal
    _src = ReadFixed<int32_t>(_src, y);
    int32_t z; // literal
    _src = ReadFixed<int32_t>(_src, z);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glScalex(x, y, z);
    // ---------- register handles ------------
}

static void retrace_glShadeModel(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int mode; // enum
    _src = ReadFixed(_src, mode);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glShadeModel(mode);
    // ---------- register handles ------------
}

static void retrace_glTexCoordPointer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int size; // literal
    _src = ReadFixed<int>(_src, size);
    int type; // enum
    _src = ReadFixed(_src, type);
    int stride; // literal
    _src = ReadFixed<int>(_src, stride);
    unsigned int csbName = 0; // ClientSideBuffer name
    //  GLvoid* pointer = NULL;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    if (_opaque_type == BufferObjectReferenceType)
    { // simple memory offset
        unsigned int pointer_raw; // raw ptr
        _src = ReadFixed<unsigned int>(_src, pointer_raw);
  //      pointer = (GLvoid *)static_cast<uintptr_t>(pointer_raw);
    }
    else if (_opaque_type == BlobType)
    { // blob type
        Array<float> pointer_blob;
        _src = Read1DArray(_src, pointer_blob);
   //     pointer = pointer_blob.v;
    }
    else if (_opaque_type == ClientSideBufferObjectReferenceType)
    { // a memory reference to client-side buffer
        unsigned int offset = 0;
        _src = ReadFixed<unsigned int>(_src, csbName);
        _src = ReadFixed<unsigned int>(_src, offset);
        //     pointer = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);
    }
    else if (_opaque_type == NoopType)
    { // Do nothing
    }

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glTexCoordPointer(size, type, stride, pointer);
    // ---------- register handles ------------
}

static void retrace_glTexEnvf(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    float param; // literal
    _src = ReadFixed<float>(_src, param);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glTexEnvf(target, pname, param);
    // ---------- register handles ------------
}

static void retrace_glTexEnvi(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    int param; // literal
    _src = ReadFixed<int>(_src, param);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glTexEnvi(target, pname, param);
    // ---------- register handles ------------
}

static void retrace_glTexEnvx(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    int32_t param; // literal
    _src = ReadFixed<int32_t>(_src, param);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glTexEnvx(target, pname, param);
    // ---------- register handles ------------
}

static void retrace_glTexEnvfv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<float> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glTexEnvfv(target, pname, params);
    // ---------- register handles ------------
}

static void retrace_glTexEnviv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<int> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glTexEnviv(target, pname, params);
    // ---------- register handles ------------
}

static void retrace_glTexEnvxv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<int32_t> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glTexEnvxv(target, pname, params);
    // ---------- register handles ------------
}

static void retrace_glTexParameterx(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    int32_t param; // literal
    _src = ReadFixed<int32_t>(_src, param);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glTexParameterx(target, pname, param);
    // ---------- register handles ------------
}

static void retrace_glTexParameterxv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<int32_t> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glTexParameterxv(target, pname, params);
    // ---------- register handles ------------
}

static void retrace_glTranslatef(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    float x; // literal
    _src = ReadFixed<float>(_src, x);
    float y; // literal
    _src = ReadFixed<float>(_src, y);
    float z; // literal
    _src = ReadFixed<float>(_src, z);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glTranslatef(x, y, z);
    // ---------- register handles ------------
}

static void retrace_glTranslatex(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int32_t x; // literal
    _src = ReadFixed<int32_t>(_src, x);
    int32_t y; // literal
    _src = ReadFixed<int32_t>(_src, y);
    int32_t z; // literal
    _src = ReadFixed<int32_t>(_src, z);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glTranslatex(x, y, z);
    // ---------- register handles ------------
}

static void retrace_glVertexPointer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int size; // literal
    _src = ReadFixed<int>(_src, size);
    int type; // enum
    _src = ReadFixed(_src, type);
    int stride; // literal
    _src = ReadFixed<int>(_src, stride);
    unsigned int csbName = 0; // ClientSideBuffer name
    //   GLvoid* pointer = NULL;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    if (_opaque_type == BufferObjectReferenceType)
    { // simple memory offset
        unsigned int pointer_raw; // raw ptr
        _src = ReadFixed<unsigned int>(_src, pointer_raw);
        //       pointer = (GLvoid *)static_cast<uintptr_t>(pointer_raw);
    }
    else if (_opaque_type == BlobType)
    { // blob type
        Array<float> pointer_blob;
        _src = Read1DArray(_src, pointer_blob);
        //       pointer = pointer_blob.v;
    }
    else if (_opaque_type == ClientSideBufferObjectReferenceType)
    { // a memory reference to client-side buffer
        unsigned int offset = 0;
        _src = ReadFixed<unsigned int>(_src, csbName);
        _src = ReadFixed<unsigned int>(_src, offset);
        //      pointer = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);
    }
    else if (_opaque_type == NoopType)
    { // Do nothing
    }

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glVertexPointer(size, type, stride, pointer);
    // ---------- register handles ------------
}

static void retrace_glBlendEquationSeparateOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int modeRGB; // enum
    _src = ReadFixed(_src, modeRGB);
    int modeAlpha; // enum
    _src = ReadFixed(_src, modeAlpha);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glBlendEquationSeparateOES(modeRGB, modeAlpha);
    // ---------- register handles ------------
}

static void retrace_glBlendFuncSeparateOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int sfactorRGB; // enum
    _src = ReadFixed(_src, sfactorRGB);
    int dfactorRGB; // enum
    _src = ReadFixed(_src, dfactorRGB);
    int sfactorAlpha; // enum
    _src = ReadFixed(_src, sfactorAlpha);
    int dfactorAlpha; // enum
    _src = ReadFixed(_src, dfactorAlpha);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glBlendFuncSeparateOES(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
    // ---------- register handles ------------
}

static void retrace_glBlendEquationOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int mode; // enum
    _src = ReadFixed(_src, mode);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glBlendEquationOES(mode);
    // ---------- register handles ------------
}

static void retrace_glBindRenderbufferOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    unsigned int renderbuffer; // literal
    _src = ReadFixed<unsigned int>(_src, renderbuffer);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint renderbufferNew = context.getRenderbufferMap().RValue(renderbuffer);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glBindRenderbufferOES(target, renderbufferNew);
    // ---------- register handles ------------
    */
}

static void retrace_glDeleteRenderbuffersOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> renderbuffers; // array
    _src = Read1DArray(_src, renderbuffers);

    /*   // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    unsigned int* renderbuffersNew = renderbuffers.v;
    for (unsigned int i = 0; i < renderbuffers.cnt; ++i) {
        renderbuffersNew[i] = context.getRenderbufferMap().RValue(renderbuffers[i]);
    }
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glDeleteRenderbuffersOES(n, renderbuffersNew);
    // ---------- register handles ------------
    */
}

static void retrace_glGenRenderbuffersOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> old_renderbuffers; // array
    _src = Read1DArray(_src, old_renderbuffers);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    // ----------- out parameters  ------------
    std::vector<GLuint> _renderbuffers(n);
    GLuint *renderbuffers = _renderbuffers.data();
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glGenRenderbuffersOES(n, renderbuffers);
    // ---------- register handles ------------
    for (unsigned int i = 0; i < old_renderbuffers.cnt; ++i)
    {
        context.getRenderbufferMap().LValue(old_renderbuffers[i]) = renderbuffers[i];
    }
    */
}

static void retrace_glRenderbufferStorageOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glRenderbufferStorageOES(target, internalformat, width, height);
    // ---------- register handles ------------
}

static void retrace_glBindFramebufferOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    unsigned int framebuffer; // literal
    _src = ReadFixed<unsigned int>(_src, framebuffer);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint framebufferNew = context._framebuffer_map.RValue(framebuffer);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glBindFramebufferOES(target, framebufferNew);
    // ---------- register handles ------------
    */
}

static void retrace_glDeleteFramebuffersOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> framebuffers; // array
    _src = Read1DArray(_src, framebuffers);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    unsigned int* framebuffersNew = framebuffers.v;
    for (unsigned int i = 0; i < framebuffers.cnt; ++i) {
        framebuffersNew[i] = context._framebuffer_map.RValue(framebuffers[i]);
    }
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glDeleteFramebuffersOES(n, framebuffersNew);
    // ---------- register handles ------------
    */
}

static void retrace_glGenFramebuffersOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> old_framebuffers; // array
    _src = Read1DArray(_src, old_framebuffers);

    /*   // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    // ----------- out parameters  ------------
    std::vector<GLuint> _framebuffers(n);
    GLuint *framebuffers = _framebuffers.data();
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glGenFramebuffersOES(n, framebuffers);
    // ---------- register handles ------------
    for (unsigned int i = 0; i < old_framebuffers.cnt; ++i)
    {
        context._framebuffer_map.LValue(old_framebuffers[i]) = framebuffers[i];
    }
    */
}

static void retrace_glCheckFramebufferStatusOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);

    int old_ret; // enum
    _src = ReadFixed(_src, old_ret);
    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    GLenum ret = 0;
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   ret = glCheckFramebufferStatusOES(target);
    // ---------- register handles ------------
    (void)ret;  // Ignored return value
}

static void retrace_glFramebufferTexture2DOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int attachment; // enum
    _src = ReadFixed(_src, attachment);
    int textarget; // enum
    _src = ReadFixed(_src, textarget);
    unsigned int texture; // literal
    _src = ReadFixed<unsigned int>(_src, texture);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint textureNew = context.getTextureMap().RValue(texture);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glFramebufferTexture2DOES(target, attachment, textarget, textureNew, level);
    // ---------- register handles ------------
    */
}

static void retrace_glFramebufferRenderbufferOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int attachment; // enum
    _src = ReadFixed(_src, attachment);
    int renderbuffertarget; // enum
    _src = ReadFixed(_src, renderbuffertarget);
    unsigned int renderbuffer; // literal
    _src = ReadFixed<unsigned int>(_src, renderbuffer);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint renderbufferNew = context.getRenderbufferMap().RValue(renderbuffer);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glFramebufferRenderbufferOES(target, attachment, renderbuffertarget, renderbufferNew);
    // ---------- register handles ------------
    */
}

static void retrace_glGenerateMipmapOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glGenerateMipmapOES(target);
    // ---------- register handles ------------
}

static void retrace_glCurrentPaletteMatrixOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glCurrentPaletteMatrixOES(index);
    // ---------- register handles ------------
}

static void retrace_glLoadPaletteFromModelViewMatrixOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glLoadPaletteFromModelViewMatrixOES();
    // ---------- register handles ------------
}

static void retrace_glMatrixIndexPointerOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int size; // literal
    _src = ReadFixed<int>(_src, size);
    int type; // enum
    _src = ReadFixed(_src, type);
    int stride; // literal
    _src = ReadFixed<int>(_src, stride);
    unsigned int csbName = 0; // ClientSideBuffer name
    //    GLvoid* pointer = NULL;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    if (_opaque_type == BufferObjectReferenceType)
    { // simple memory offset
        unsigned int pointer_raw; // raw ptr
        _src = ReadFixed<unsigned int>(_src, pointer_raw);
        //      pointer = (GLvoid *)static_cast<uintptr_t>(pointer_raw);
    }
    else if (_opaque_type == BlobType)
    { // blob type
        Array<float> pointer_blob;
        _src = Read1DArray(_src, pointer_blob);
        //      pointer = pointer_blob.v;
    }
    else if (_opaque_type == ClientSideBufferObjectReferenceType)
    { // a memory reference to client-side buffer
        unsigned int offset = 0;
        _src = ReadFixed<unsigned int>(_src, csbName);
        _src = ReadFixed<unsigned int>(_src, offset);
        //     pointer = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);
    }
    else if (_opaque_type == NoopType)
    { // Do nothing
    }

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glMatrixIndexPointerOES(size, type, stride, pointer);
    // ---------- register handles ------------
}

static void retrace_glWeightPointerOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int size; // literal
    _src = ReadFixed<int>(_src, size);
    int type; // enum
    _src = ReadFixed(_src, type);
    int stride; // literal
    _src = ReadFixed<int>(_src, stride);
    unsigned int csbName = 0; // ClientSideBuffer name
    //    GLvoid* pointer = NULL;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    if (_opaque_type == BufferObjectReferenceType)
    { // simple memory offset
        unsigned int pointer_raw; // raw ptr
        _src = ReadFixed<unsigned int>(_src, pointer_raw);
        //      pointer = (GLvoid *)static_cast<uintptr_t>(pointer_raw);
    }
    else if (_opaque_type == BlobType)
    { // blob type
        Array<float> pointer_blob;
        _src = Read1DArray(_src, pointer_blob);
        //       pointer = pointer_blob.v;
    }
    else if (_opaque_type == ClientSideBufferObjectReferenceType)
    { // a memory reference to client-side buffer
        unsigned int offset = 0;
        _src = ReadFixed<unsigned int>(_src, csbName);
        _src = ReadFixed<unsigned int>(_src, offset);
        //     pointer = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);
    }
    else if (_opaque_type == NoopType)
    { // Do nothing
    }

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glWeightPointerOES(size, type, stride, pointer);
    // ---------- register handles ------------
}

static void retrace_glQueryMatrixxOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    Array<int32_t> mantissa; // array
    _src = Read1DArray(_src, mantissa);
    Array<int> exponent; // array
    _src = Read1DArray(_src, exponent);

    unsigned int old_ret; // literal
    _src = ReadFixed<unsigned int>(_src, old_ret);
    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    GLbitfield ret = 0;
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   ret = glQueryMatrixxOES(mantissa, exponent);
    // ---------- register handles ------------
    (void)ret;  // Ignored return value
}

static void retrace_glTexGenfOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int coord; // enum
    _src = ReadFixed(_src, coord);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    float param; // literal
    _src = ReadFixed<float>(_src, param);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glTexGenfOES(coord, pname, param);
    // ---------- register handles ------------
}

static void retrace_glTexGenfvOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int coord; // enum
    _src = ReadFixed(_src, coord);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<float> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glTexGenfvOES(coord, pname, params);
    // ---------- register handles ------------
}

static void retrace_glTexGeniOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int coord; // enum
    _src = ReadFixed(_src, coord);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    int param; // literal
    _src = ReadFixed<int>(_src, param);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glTexGeniOES(coord, pname, param);
    // ---------- register handles ------------
}

static void retrace_glTexGenivOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int coord; // enum
    _src = ReadFixed(_src, coord);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<int> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glTexGenivOES(coord, pname, params);
    // ---------- register handles ------------
}

static void retrace_glTexGenxOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int coord; // enum
    _src = ReadFixed(_src, coord);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    int32_t param; // literal
    _src = ReadFixed<int32_t>(_src, param);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glTexGenxOES(coord, pname, param);
    // ---------- register handles ------------
}

static void retrace_glTexGenxvOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int coord; // enum
    _src = ReadFixed(_src, coord);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<int32_t> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glTexGenxvOES(coord, pname, params);
    // ---------- register handles ------------
}

static void retrace_glMapBufferOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    unsigned int access; // literal
    _src = ReadFixed<unsigned int>(_src, access);

    unsigned int csbName = 0; // ClientSideBuffer name
    //  GLvoid* old_ret = NULL;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    if (_opaque_type == BufferObjectReferenceType)
    { // simple memory offset
        unsigned int old_ret_raw; // raw ptr
        _src = ReadFixed<unsigned int>(_src, old_ret_raw);
    //    old_ret = (GLvoid *)static_cast<uintptr_t>(old_ret_raw);
    }
    else if (_opaque_type == BlobType)
    { // blob type
        Array<float> old_ret_blob;
        _src = Read1DArray(_src, old_ret_blob);
        //     old_ret = old_ret_blob.v;
    }
    else if (_opaque_type == ClientSideBufferObjectReferenceType)
    { // a memory reference to client-side buffer
        unsigned int offset = 0;
        _src = ReadFixed<unsigned int>(_src, csbName);
        _src = ReadFixed<unsigned int>(_src, offset);
        //      old_ret = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);
    }
    else if (_opaque_type == NoopType)
    { // Do nothing
    }
    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    //  GLvoid * ret = 0;
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  ret = glMapBufferOES(target, access);
    // ---------- register handles ------------
    //   (void)old_ret;  // Unused variable
    /*
    Context& context = gRetracer.getCurrentContext();
    GLuint buffer = getBoundBuffer(target);

    if (access == GL_WRITE_ONLY)
    {
        context._bufferToData_map[buffer] = ret;
    }
    else
    {
        context._bufferToData_map[buffer] = 0;
    }
    */

    gRetracer.curContextState->curBufferState.ff_glMapBufferRange(target, 0, NO_IDX, gRetracer.mCurCallNo);
}

static void retrace_glUnmapBufferOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);

    unsigned char old_ret; // literal
    _src = ReadFixed<unsigned char>(_src, old_ret);
    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    //    GLboolean ret = 0;
    // ------------- pre retrace ------------------
    //   GLuint bufferId = getBoundBuffer(target);
    //    gRetracer.getCurrentContext()._bufferToData_map.erase(bufferId);
    // ------------- retrace ------------------
    //    ret = glUnmapBufferOES(target);
    // ---------- register handles ------------
    //   (void)ret;  // Ignored return value
    //gRetracer.callNoList.insert(gRetracer.mCurCallNo);
    gRetracer.curContextState->curBufferState.ff_glUnmapBuffer(target, gRetracer.mCurCallNo);
}

static void retrace_glTexImage3DOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);
    int depth; // literal
    _src = ReadFixed<int>(_src, depth);
    int border; // literal
    _src = ReadFixed<int>(_src, border);
    int format; // enum
    _src = ReadFixed(_src, format);
    int type; // enum
    _src = ReadFixed(_src, type);
    //   GLvoid* pixels = NULL;
    Array<char> pixelsBlob;
    if (gRetracer.getFileFormatVersion() <= HEADER_VERSION_3)
    {
        _src = Read1DArray(_src, pixelsBlob);
        //     pixels = pixelsBlob.v;
    }
    else
    {
        unsigned int _opaque_type = 0;
        _src = ReadFixed(_src, _opaque_type);
        switch (_opaque_type)
        {
        case BlobType:
            _src = Read1DArray(_src, pixelsBlob);
            //       pixels = pixelsBlob.v;
            break;
        case BufferObjectReferenceType:
            unsigned int bufferOffset = 0;
            _src = ReadFixed<unsigned int>(_src, bufferOffset);
            //       pixels = (GLvoid *)static_cast<uintptr_t>(bufferOffset);
            break;
        }
    }

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
#ifndef NDEBUG
    // gRetracer.mTextureDataSize += _gl_image_size(format, type, width, height, depth);
#endif
    // ------------- retrace ------------------
    //   glTexImage3DOES(target, level, internalformat, width, height, depth, border, format, type, pixels);
    // ---------- register handles ------------

    std::unordered_set<unsigned int> newList;
    unsigned int GL_PIXEL_UNPACK_BUFFERNo = gRetracer.curContextState->curBufferState.getCurGL_PIXEL_UNPACK_BUFFERNo();
    unsigned int GL_PIXEL_UNPACK_BUFFERIdx = gRetracer.curContextState->curBufferState.getCurGL_PIXEL_UNPACK_BUFFERIdx();
    if(GL_PIXEL_UNPACK_BUFFERIdx!=0)
    {
        gRetracer.curContextState->curBufferState.findABufferState(GL_PIXEL_UNPACK_BUFFERIdx, newList);
    }
    gRetracer.curContextState->curTextureState.ff_glTypes_TexImage2D(target, GL_PIXEL_UNPACK_BUFFERNo, gRetracer.mCurCallNo, newList);
}

static void retrace_glTexSubImage3DOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int xoffset; // literal
    _src = ReadFixed<int>(_src, xoffset);
    int yoffset; // literal
    _src = ReadFixed<int>(_src, yoffset);
    int zoffset; // literal
    _src = ReadFixed<int>(_src, zoffset);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);
    int depth; // literal
    _src = ReadFixed<int>(_src, depth);
    int format; // enum
    _src = ReadFixed(_src, format);
    int type; // enum
    _src = ReadFixed(_src, type);
    //   GLvoid* pixels = NULL;
    Array<char> pixelsBlob;
    if (gRetracer.getFileFormatVersion() <= HEADER_VERSION_3)
    {
        _src = Read1DArray(_src, pixelsBlob);
        //       pixels = pixelsBlob.v;
    }
    else
    {
        unsigned int _opaque_type = 0;
        _src = ReadFixed(_src, _opaque_type);
        switch (_opaque_type)
        {
        case BlobType:
            _src = Read1DArray(_src, pixelsBlob);
            //          pixels = pixelsBlob.v;
            break;
        case BufferObjectReferenceType:
            unsigned int bufferOffset = 0;
            _src = ReadFixed<unsigned int>(_src, bufferOffset);
            //          pixels = (GLvoid *)static_cast<uintptr_t>(bufferOffset);
            break;
        }
    }

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
#ifndef NDEBUG
    //  gRetracer.mTextureDataSize += _gl_image_size(format, type, width, height, depth);
#endif
    // ------------- retrace ------------------
    //   glTexSubImage3DOES(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
    // ---------- register handles ------------

    std::unordered_set<unsigned int> newList;
    unsigned int GL_PIXEL_UNPACK_BUFFERNo = gRetracer.curContextState->curBufferState.getCurGL_PIXEL_UNPACK_BUFFERNo();
    unsigned int GL_PIXEL_UNPACK_BUFFERIdx = gRetracer.curContextState->curBufferState.getCurGL_PIXEL_UNPACK_BUFFERIdx();
    if(GL_PIXEL_UNPACK_BUFFERIdx!=0)
    {
        gRetracer.curContextState->curBufferState.findABufferState(GL_PIXEL_UNPACK_BUFFERIdx, newList);
    }
    gRetracer.curContextState->curTextureState.ff_glTypes_TexSubImage2D(target, GL_PIXEL_UNPACK_BUFFERNo, gRetracer.mCurCallNo, newList);
}

static void retrace_glCopyTexSubImage3DOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int xoffset; // literal
    _src = ReadFixed<int>(_src, xoffset);
    int yoffset; // literal
    _src = ReadFixed<int>(_src, yoffset);
    int zoffset; // literal
    _src = ReadFixed<int>(_src, zoffset);
    int x; // literal
    _src = ReadFixed<int>(_src, x);
    int y; // literal
    _src = ReadFixed<int>(_src, y);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glCopyTexSubImage3DOES(target, level, xoffset, yoffset, zoffset, x, y, width, height);
    // ---------- register handles ------------
}

static void retrace_glCompressedTexImage3DOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
#ifdef __APPLE__
    // Since we're only supporting Apple devices with GLES3
    // (which implies ETC2 support), we take advantage of ETC2's
    // backwards compatibility to get ETC1 support as well.
    // (This only makes sense for the internalformat, but is added to all parsed enums.)
    if (internalformat == GL_ETC1_RGB8_OES)
    {
        internalformat = GL_COMPRESSED_RGB8_ETC2;
    }
#endif
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);
    int depth; // literal
    _src = ReadFixed<int>(_src, depth);
    int border; // literal
    _src = ReadFixed<int>(_src, border);
    int imageSize; // literal
    _src = ReadFixed<int>(_src, imageSize);
    //   GLvoid* data = NULL;
    Array<char> dataBlob;
    if (gRetracer.getFileFormatVersion() <= HEADER_VERSION_3)
    {
        _src = Read1DArray(_src, dataBlob);
        //      data = dataBlob.v;
    }
    else
    {
        unsigned int _opaque_type = 0;
        _src = ReadFixed(_src, _opaque_type);
        switch (_opaque_type)
        {
        case BlobType:
            _src = Read1DArray(_src, dataBlob);
            //         data = dataBlob.v;
            break;
        case BufferObjectReferenceType:
            unsigned int bufferOffset = 0;
            _src = ReadFixed<unsigned int>(_src, bufferOffset);
            //        data = (GLvoid *)static_cast<uintptr_t>(bufferOffset);
            break;
        }
    }

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    //#ifndef NDEBUG
    //    gRetracer.mCompressedTextureDataSize += imageSize;
    //#endif
    // ------------- retrace ------------------
    //   glCompressedTexImage3DOES(target, level, internalformat, width, height, depth, border, imageSize, data);
    // ---------- register handles ------------

    std::unordered_set<unsigned int> newList;
    unsigned int GL_PIXEL_UNPACK_BUFFERNo = gRetracer.curContextState->curBufferState.getCurGL_PIXEL_UNPACK_BUFFERNo();
    unsigned int GL_PIXEL_UNPACK_BUFFERIdx = gRetracer.curContextState->curBufferState.getCurGL_PIXEL_UNPACK_BUFFERIdx();
    if(GL_PIXEL_UNPACK_BUFFERIdx!=0)
    {
        gRetracer.curContextState->curBufferState.findABufferState(GL_PIXEL_UNPACK_BUFFERIdx, newList);
    }
    gRetracer.curContextState->curTextureState.ff_glTypes_TexImage2D(target, GL_PIXEL_UNPACK_BUFFERNo, gRetracer.mCurCallNo, newList);
}

static void retrace_glCompressedTexSubImage3DOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int xoffset; // literal
    _src = ReadFixed<int>(_src, xoffset);
    int yoffset; // literal
    _src = ReadFixed<int>(_src, yoffset);
    int zoffset; // literal
    _src = ReadFixed<int>(_src, zoffset);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);
    int depth; // literal
    _src = ReadFixed<int>(_src, depth);
    int format; // enum
    _src = ReadFixed(_src, format);
#ifdef __APPLE__
    // Since we're only supporting Apple devices with GLES3
    // (which implies ETC2 support), we take advantage of ETC2's
    // backwards compatibility to get ETC1 support as well.
    // (This only makes sense for the internalformat, but is added to all parsed enums.)
    if (format == GL_ETC1_RGB8_OES)
    {
        format = GL_COMPRESSED_RGB8_ETC2;
    }
#endif
    int imageSize; // literal
    _src = ReadFixed<int>(_src, imageSize);
    //   GLvoid* data = NULL;
    Array<char> dataBlob;
    if (gRetracer.getFileFormatVersion() <= HEADER_VERSION_3)
    {
        _src = Read1DArray(_src, dataBlob);
        //       data = dataBlob.v;
    }
    else
    {
        unsigned int _opaque_type = 0;
        _src = ReadFixed(_src, _opaque_type);
        switch (_opaque_type)
        {
        case BlobType:
            _src = Read1DArray(_src, dataBlob);
            //         data = dataBlob.v;
            break;
        case BufferObjectReferenceType:
            unsigned int bufferOffset = 0;
            _src = ReadFixed<unsigned int>(_src, bufferOffset);
            //       data = (GLvoid *)static_cast<uintptr_t>(bufferOffset);
            break;
        }
    }

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    //#ifndef NDEBUG
    //    gRetracer.mCompressedTextureDataSize += imageSize;
    //#endif
    // ------------- retrace ------------------
    //   glCompressedTexSubImage3DOES(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
    // ---------- register handles ------------
}

static void retrace_glFramebufferTexture3DOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int attachment; // enum
    _src = ReadFixed(_src, attachment);
    int textarget; // enum
    _src = ReadFixed(_src, textarget);
    unsigned int texture; // literal
    _src = ReadFixed<unsigned int>(_src, texture);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int zoffset; // literal
    _src = ReadFixed<int>(_src, zoffset);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint textureNew = context.getTextureMap().RValue(texture);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glFramebufferTexture3DOES(target, attachment, textarget, textureNew, level, zoffset);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramBinaryOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int binaryFormat; // enum
    _src = ReadFixed(_src, binaryFormat);
    Array<char> binary; // blob
    _src = Read1DArray(_src, binary);
    int length; // literal
    _src = ReadFixed<int>(_src, length);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramBinaryOES(programNew, binaryFormat, binary, length);
    // ---------- register handles ------------
    */
}

static void retrace_glDiscardFramebufferEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int numAttachments; // literal
    _src = ReadFixed<int>(_src, numAttachments);
    Array<unsigned int> attachments; // array
    _src = Read1DArray(_src, attachments);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    RetraceOptions& opt = gRetracer.mOptions;
    /*   if(opt.mForceOffscreen)
    {
        for (int i = 0; i < numAttachments; i++)
        {
            unsigned int attachment = attachments[i];
            if (attachment == GL_COLOR)
                attachments[i] = GL_COLOR_ATTACHMENT0;
            else if (attachment == GL_DEPTH)
                attachments[i] = GL_DEPTH_ATTACHMENT;
            else if (attachment == GL_STENCIL)
                attachments[i] = GL_STENCIL_ATTACHMENT;
        }
    }*/
    //  glDiscardFramebufferEXT(target, numAttachments, attachments);
    // ---------- register handles ------------
}

static void retrace_glBindVertexArrayOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int array; // literal
    _src = ReadFixed<unsigned int>(_src, array);

    /*    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint arrayNew = context._array_map.RValue(array);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glBindVertexArrayOES(arrayNew);
    // ---------- register handles ------------
    */
    //gRetracer.callNoList.insert(gRetracer.mCurCallNo);
    if(array == 0 && gRetracer.preExecute == true)
    {
        gRetracer.callNoList.insert(gRetracer.mCurCallNo);
    }
    if(array == 0)
    {
        array = 65535;//default vao index
    }
    gRetracer.curContextState->curBufferState.ff_glBindVertexArray(array, gRetracer.mCurCallNo);
}

static void retrace_glDeleteVertexArraysOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> arrays; // array
    _src = Read1DArray(_src, arrays);

    /*   // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    unsigned int* arraysNew = arrays.v;
    for (unsigned int i = 0; i < arrays.cnt; ++i) {
        arraysNew[i] = context._array_map.RValue(arrays[i]);
    }
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glDeleteVertexArraysOES(n, arraysNew);
    // ---------- register handles ------------
    */
}

static void retrace_glGenVertexArraysOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> old_arrays; // array
    _src = Read1DArray(_src, old_arrays);

    /*    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    // ----------- out parameters  ------------
    std::vector<GLuint> _arrays(n);
    GLuint *arrays = _arrays.data();
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glGenVertexArraysOES(n, arrays);
    // ---------- register handles ------------
    for (unsigned int i = 0; i < old_arrays.cnt; ++i)
    {
        context._array_map.LValue(old_arrays[i]) = arrays[i];
    }
    */
    //gRetracer.callNoList.insert(gRetracer.mCurCallNo);
    gRetracer.curContextState->curBufferState.ff_glGenVertexArrays(n, old_arrays, gRetracer.mCurCallNo);
}

static void retrace_glCoverageMaskNV(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned char mask; // literal
    _src = ReadFixed<unsigned char>(_src, mask);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glCoverageMaskNV(mask);
    // ---------- register handles ------------
}

static void retrace_glCoverageOperationNV(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int operation; // enum
    _src = ReadFixed(_src, operation);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glCoverageOperationNV(operation);
    // ---------- register handles ------------
}

static void retrace_glRenderbufferStorageMultisampleEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int samples; // literal
    _src = ReadFixed<int>(_src, samples);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    // glRenderbufferStorageMultisampleEXT(target, samples, internalformat, width, height);
    // ---------- register handles ------------
}

static void retrace_glFramebufferTexture2DMultisampleEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int attachment; // enum
    _src = ReadFixed(_src, attachment);
    int textarget; // enum
    _src = ReadFixed(_src, textarget);
    unsigned int texture; // literal
    _src = ReadFixed<unsigned int>(_src, texture);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int samples; // literal
    _src = ReadFixed<int>(_src, samples);

    /*   // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint textureNew = context.getTextureMap().RValue(texture);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glFramebufferTexture2DMultisampleEXT(target, attachment, textarget, textureNew, level, samples);
    // ---------- register handles ------------
    */
}

static void retrace_glRenderbufferStorageMultisampleIMG(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int samples; // literal
    _src = ReadFixed<int>(_src, samples);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glRenderbufferStorageMultisampleIMG(target, samples, internalformat, width, height);
    // ---------- register handles ------------
}

static void retrace_glFramebufferTexture2DMultisampleIMG(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int attachment; // enum
    _src = ReadFixed(_src, attachment);
    int textarget; // enum
    _src = ReadFixed(_src, textarget);
    unsigned int texture; // literal
    _src = ReadFixed<unsigned int>(_src, texture);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int samples; // literal
    _src = ReadFixed<int>(_src, samples);

    /*    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint textureNew = context.getTextureMap().RValue(texture);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glFramebufferTexture2DMultisampleIMG(target, attachment, textarget, textureNew, level, samples);
    // ---------- register handles ------------
    */
}

static void retrace_glRenderbufferStorageMultisampleAPPLE(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int samples; // literal
    _src = ReadFixed<int>(_src, samples);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    // glRenderbufferStorageMultisampleAPPLE(target, samples, internalformat, width, height);
    // ---------- register handles ------------
}

static void retrace_glResolveMultisampleFramebufferAPPLE(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
   //  glResolveMultisampleFramebufferAPPLE();
    // ---------- register handles ------------
}

static void retrace_glBlitFramebufferANGLE(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int srcX0; // literal
    _src = ReadFixed<int>(_src, srcX0);
    int srcY0; // literal
    _src = ReadFixed<int>(_src, srcY0);
    int srcX1; // literal
    _src = ReadFixed<int>(_src, srcX1);
    int srcY1; // literal
    _src = ReadFixed<int>(_src, srcY1);
    int dstX0; // literal
    _src = ReadFixed<int>(_src, dstX0);
    int dstY0; // literal
    _src = ReadFixed<int>(_src, dstY0);
    int dstX1; // literal
    _src = ReadFixed<int>(_src, dstX1);
    int dstY1; // literal
    _src = ReadFixed<int>(_src, dstY1);
    unsigned int mask; // literal
    _src = ReadFixed<unsigned int>(_src, mask);
    int filter; // enum
    _src = ReadFixed(_src, filter);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glBlitFramebufferANGLE(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    // ---------- register handles ------------
}

static void retrace_glRenderbufferStorageMultisampleANGLE(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int samples; // literal
    _src = ReadFixed<int>(_src, samples);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glRenderbufferStorageMultisampleANGLE(target, samples, internalformat, width, height);
    // ---------- register handles ------------
}

static void retrace_glDrawBuffersNV(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> bufs; // array
    _src = Read1DArray(_src, bufs);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glDrawBuffersNV(n, bufs);
    // ---------- register handles ------------
}

static void retrace_glReadBufferNV(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int mode; // enum
    _src = ReadFixed(_src, mode);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glReadBufferNV(mode);
    // ---------- register handles ------------
    //gRetracer.callNoList.insert(gRetracer.mCurCallNo);
}

static void retrace_glLabelObjectEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int type; // enum
    _src = ReadFixed(_src, type);
    unsigned int object; // literal
    _src = ReadFixed<unsigned int>(_src, object);
    int length; // literal
    _src = ReadFixed<int>(_src, length);
    char* label; // string
    _src = ReadString(_src, label);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glLabelObjectEXT(type, object, length, label);
    // ---------- register handles ------------
}

static void retrace_glInsertEventMarkerEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int length; // literal
    _src = ReadFixed<int>(_src, length);
    char* marker; // string
    _src = ReadString(_src, marker);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glInsertEventMarkerEXT(length, marker);
    // ---------- register handles ------------
}

static void retrace_glPushGroupMarkerEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int length; // literal
    _src = ReadFixed<int>(_src, length);
    char* marker; // string
    _src = ReadString(_src, marker);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glPushGroupMarkerEXT(length, marker);
    // ---------- register handles ------------
}

static void retrace_glPopGroupMarkerEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glPopGroupMarkerEXT();
    // ---------- register handles ------------
}

static void retrace_glGenQueriesEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> old_ids; // array
    _src = Read1DArray(_src, old_ids);

    /*   // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    // ----------- out parameters  ------------
    std::vector<GLuint> _ids(n);
    GLuint *ids = _ids.data();
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glGenQueriesEXT(n, ids);
    // ---------- register handles ------------
    for (unsigned int i = 0; i < old_ids.cnt; ++i)
    {
        context._query_map.LValue(old_ids[i]) = ids[i];
    }
    */
}

static void retrace_glDeleteQueriesEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> ids; // array
    _src = Read1DArray(_src, ids);

    /*   // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    unsigned int* idsNew = ids.v;
    for (unsigned int i = 0; i < ids.cnt; ++i) {
        idsNew[i] = context._query_map.RValue(ids[i]);
    }
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glDeleteQueriesEXT(n, idsNew);
    // ---------- register handles ------------
    */
}

static void retrace_glBeginQueryEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    unsigned int id; // literal
    _src = ReadFixed<unsigned int>(_src, id);

    /*   // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint idNew = context._query_map.RValue(id);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glBeginQueryEXT(target, idNew);
    // ---------- register handles ------------
    */
}

static void retrace_glEndQueryEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glEndQueryEXT(target);
    // ---------- register handles ------------
}

static void retrace_glUseProgramStagesEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int pipeline; // literal
    _src = ReadFixed<unsigned int>(_src, pipeline);
    unsigned int stages; // literal
    _src = ReadFixed<unsigned int>(_src, stages);
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);

     /*   // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint pipelineNew = context._pipeline_map.RValue(pipeline);
    GLuint programNew = context.getProgramMap().RValue(program);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUseProgramStagesEXT(pipelineNew, stages, programNew);
    // ---------- register handles ------------
    */
}

static void retrace_glActiveShaderProgramEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int pipeline; // literal
    _src = ReadFixed<unsigned int>(_src, pipeline);
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);

    /*   // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint pipelineNew = context._pipeline_map.RValue(pipeline);
    GLuint programNew = context.getProgramMap().RValue(program);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glActiveShaderProgramEXT(pipelineNew, programNew);
    // ---------- register handles ------------
    */
}

static void retrace_glCreateShaderProgramvEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int type; // enum
    _src = ReadFixed(_src, type);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<const char*> strings; // string array
    _src = ReadStringArray(_src, strings);

    unsigned int old_ret; // literal
    _src = ReadFixed<unsigned int>(_src, old_ret);
    // ----------- look up handles ------------
    /*    // ----------- out parameters  ------------
    GLuint ret = 0;
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    ret = glCreateShaderProgramvEXT(type, count, strings);
    // ---------- register handles ------------
    Context& context = gRetracer.getCurrentContext();
    context.getProgramMap().LValue(old_ret) = ret;
    context.getProgramRevMap().LValue(ret) = old_ret;
    */
}

static void retrace_glBindProgramPipelineEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int pipeline; // literal
    _src = ReadFixed<unsigned int>(_src, pipeline);

    /*    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint pipelineNew = context._pipeline_map.RValue(pipeline);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glBindProgramPipelineEXT(pipelineNew);
    // ---------- register handles ------------
    */
}

static void retrace_glDeleteProgramPipelinesEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> pipelines; // array
    _src = Read1DArray(_src, pipelines);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glDeleteProgramPipelinesEXT(n, pipelines);
    // ---------- register handles ------------
}

static void retrace_glGenProgramPipelinesEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> old_pipelines; // array
    _src = Read1DArray(_src, old_pipelines);

    /*   // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    // ----------- out parameters  ------------
    std::vector<GLuint> _pipelines(n);
    GLuint *pipelines = _pipelines.data();
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glGenProgramPipelinesEXT(n, pipelines);
    // ---------- register handles ------------
    for (unsigned int i = 0; i < old_pipelines.cnt; ++i)
    {
        context._pipeline_map.LValue(old_pipelines[i]) = pipelines[i];
        context._pipeline_rev_map.LValue(pipelines[i]) = old_pipelines[i];
    }
    */
}

static void retrace_glValidateProgramPipelineEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int pipeline; // literal
    _src = ReadFixed<unsigned int>(_src, pipeline);

    /*  // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint pipelineNew = context._pipeline_map.RValue(pipeline);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glValidateProgramPipelineEXT(pipelineNew);
    // ---------- register handles ------------
    */
}

static void retrace_glDrawTexiOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int x; // literal
    _src = ReadFixed<int>(_src, x);
    int y; // literal
    _src = ReadFixed<int>(_src, y);
    int z; // literal
    _src = ReadFixed<int>(_src, z);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glDrawTexiOES(x, y, z, width, height);
    // ---------- register handles ------------
}

static void retrace_glBlendBarrierKHR(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glBlendBarrierKHR();
    // ---------- register handles ------------
}

static void retrace_glBlendBarrierNV(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glBlendBarrierNV();
    // ---------- register handles ------------
}

static void retrace_glCreateClientSideBuffer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------

    unsigned int old_ret; // literal
    _src = ReadFixed<unsigned int>(_src, old_ret);
    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    //  GLuint ret = 0;
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //ret = glCreateClientSideBuffer();
    // ---------- register handles ------------
    //  (void)ret;  // Ignored return value
    //gRetracer.callNoList.insert(gRetracer.mCurCallNo);
}

static void retrace_glDeleteClientSideBuffer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int name; // literal
    _src = ReadFixed<unsigned int>(_src, name);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glDeleteClientSideBuffer(name);
    // ---------- register handles ------------
}

static void retrace_glClientSideBufferData(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int name; // literal
    _src = ReadFixed<unsigned int>(_src, name);
    int size; // literal
    _src = ReadFixed<int>(_src, size);
    Array<char> data; // blob
    _src = Read1DArray(_src, data);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glClientSideBufferData(name, size, data);
    // ---------- register handles ------------

    //for fastforwad
    gRetracer.curContextState->curClientSideBufferState.ff_glClientSideBufferData(name, gRetracer.mCurCallNo);

    //gRetracer.callNoList.insert(gRetracer.mCurCallNo);
}

static void retrace_glClientSideBufferSubData(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int name; // literal
    _src = ReadFixed<unsigned int>(_src, name);
    int offset; // literal
    _src = ReadFixed<int>(_src, offset);
    int size; // literal
    _src = ReadFixed<int>(_src, size);
    Array<char> data; // blob
    _src = Read1DArray(_src, data);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glClientSideBufferSubData(name, offset, size, data);
    // ---------- register handles ------------
}

static void retrace_glCopyClientSideBuffer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    unsigned int name; // literal
    _src = ReadFixed<unsigned int>(_src, name);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glCopyClientSideBuffer(target, name);
    // ---------- register handles ------------
    /*
    std::unordered_set<unsigned int> newList;
    unsigned int binderNo = 0;
    unsigned int binderIdx = 0;

    if(target == GL_ARRAY_BUFFER)
    {
        binderNo = gRetracer.curContextState->curBufferState.getCurGL_ARRAY_BUFFERNo();
        binderIdx = gRetracer.curContextState->curBufferState.getCurGL_ARRAY_BUFFERIdx();

    }
    else if(target == GL_ELEMENT_ARRAY_BUFFER)
    {
        binderNo = gRetracer.curContextState->curBufferState.getCurGL_ELEMENT_ARRAY_BUFFERNo();
        binderIdx = gRetracer.curContextState->curBufferState.getCurGL_ELEMENT_ARRAY_BUFFERIdx();
    }
    else if(target == GL_PIXEL_UNPACK_BUFFER)
    {
        binderNo = gRetracer.curContextState->curBufferState.getCurGL_PIXEL_UNPACK_BUFFERNo();
        binderIdx = gRetracer.curContextState->curBufferState.getCurGL_PIXEL_UNPACK_BUFFERIdx();

    }
    else
    {
        printf("error: not support parameter of glCopyClientSideBuffer\n");
    }


    gRetracer.curContextState->curBufferState.findABufferState(binderIdx, newList);

    gRetracer.curContextState->curClientSideBufferState.ff_glCopyClientSideBuffer(target, name, binderNo, gRetracer.mCurCallNo, newList);

    */
    unsigned long long nameNo= 0;

    nameNo = gRetracer.curContextState->curClientSideBufferState.findAClientSideBufferState(name);
    gRetracer.curContextState->curBufferState.ff_glCopyClientSideBuffer(target, nameNo, gRetracer.mCurCallNo);
}

static void retrace_glPatchClientSideBuffer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int size; // literal
    _src = ReadFixed<int>(_src, size);
    Array<char> data; // blob
    _src = Read1DArray(_src, data);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glPatchClientSideBuffer(target, size, data);
    // ---------- register handles ------------
    gRetracer.curContextState->curBufferState.ff_glCopyClientSideBuffer(target, 0, gRetracer.mCurCallNo);
}

static void retrace_paMandatoryExtensions(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<const char*> stringArr; // string array
    _src = ReadStringArray(_src, stringArr);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    paMandatoryExtensions(count, stringArr);
    // ---------- register handles ------------
}

static void retrace_glAssertBuffer_ARM(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int offset; // literal
    _src = ReadFixed<int>(_src, offset);
    int size; // literal
    _src = ReadFixed<int>(_src, size);
    char* md5; // string
    _src = ReadString(_src, md5);

    /*    const char *ptr = (const char *)glMapBufferRange(target, offset, size, GL_MAP_READ_BIT);
    MD5Digest md5_bound_calc(ptr, size);
    std::string md5_bound = md5_bound_calc.text();
    if (md5_bound != md5) {
        DBG_LOG("glAssertBuffer_ARM: MD5 sums differ!\n");
        abort();
    }*/
    //  glUnmapBuffer(target);
    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ---------- register handles ------------
}

static void retrace_glStateDump_ARM(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    //    gRetracer.getStateLogger().logState(gRetracer.getCurTid());
    // ---------- register handles ------------
}

static void retrace_glDebugMessageControl(char* _src)
{
}

static void retrace_glDebugMessageInsert(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int source; // enum
    _src = ReadFixed(_src, source);
    int type; // enum
    _src = ReadFixed(_src, type);
    unsigned int id; // literal
    _src = ReadFixed<unsigned int>(_src, id);
    int severity; // enum
    _src = ReadFixed(_src, severity);
    int length; // literal
    _src = ReadFixed<int>(_src, length);
    char* buf; // string
    _src = ReadString(_src, buf);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glDebugMessageInsert(source, type, id, severity, length, buf);
    // ---------- register handles ------------
}

static void retrace_glPushDebugGroup(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int source; // enum
    _src = ReadFixed(_src, source);
    unsigned int id; // literal
    _src = ReadFixed<unsigned int>(_src, id);
    int length; // literal
    _src = ReadFixed<int>(_src, length);
    char* message; // string
    _src = ReadString(_src, message);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glPushDebugGroup(source, id, length, message);
    // ---------- register handles ------------
}

static void retrace_glPopDebugGroup(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glPopDebugGroup();
    // ---------- register handles ------------
}

static void retrace_glObjectLabel(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int identifier; // enum
    _src = ReadFixed(_src, identifier);
    unsigned int name; // literal
    _src = ReadFixed<unsigned int>(_src, name);
    int length; // literal
    _src = ReadFixed<int>(_src, length);
    char* label; // string
    _src = ReadString(_src, label);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glObjectLabel(identifier, name, length, label);
    // ---------- register handles ------------
}

static void retrace_glObjectPtrLabel(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int csbName = 0; // ClientSideBuffer name
    //  GLvoid* ptr = NULL;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    if (_opaque_type == BufferObjectReferenceType)
    { // simple memory offset
        unsigned int ptr_raw; // raw ptr
        _src = ReadFixed<unsigned int>(_src, ptr_raw);
        //     ptr = (GLvoid *)static_cast<uintptr_t>(ptr_raw);
    }
    else if (_opaque_type == BlobType)
    { // blob type
        Array<float> ptr_blob;
        _src = Read1DArray(_src, ptr_blob);
        //    ptr = ptr_blob.v;
    }
    else if (_opaque_type == ClientSideBufferObjectReferenceType)
    { // a memory reference to client-side buffer
        unsigned int offset = 0;
        _src = ReadFixed<unsigned int>(_src, csbName);
        _src = ReadFixed<unsigned int>(_src, offset);
        //   ptr = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);
    }
    else if (_opaque_type == NoopType)
    { // Do nothing
    }
    int length; // literal
    _src = ReadFixed<int>(_src, length);
    char* label; // string
    _src = ReadString(_src, label);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glObjectPtrLabel(ptr, length, label);
    // ---------- register handles ------------
}

static void retrace_glDebugMessageControlKHR(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int source; // enum
    _src = ReadFixed(_src, source);
    int type; // enum
    _src = ReadFixed(_src, type);
    int severity; // enum
    _src = ReadFixed(_src, severity);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<unsigned int> ids; // array
    _src = Read1DArray(_src, ids);
    unsigned char enabled; // literal
    _src = ReadFixed<unsigned char>(_src, enabled);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glDebugMessageControlKHR(source, type, severity, count, ids, enabled);
    // ---------- register handles ------------
}

static void retrace_glDebugMessageInsertKHR(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int source; // enum
    _src = ReadFixed(_src, source);
    int type; // enum
    _src = ReadFixed(_src, type);
    unsigned int id; // literal
    _src = ReadFixed<unsigned int>(_src, id);
    int severity; // enum
    _src = ReadFixed(_src, severity);
    int length; // literal
    _src = ReadFixed<int>(_src, length);
    char* buf; // string
    _src = ReadString(_src, buf);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glDebugMessageInsertKHR(source, type, id, severity, length, buf);
    // ---------- register handles ------------
}

static void retrace_glPushDebugGroupKHR(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int source; // enum
    _src = ReadFixed(_src, source);
    unsigned int id; // literal
    _src = ReadFixed<unsigned int>(_src, id);
    int length; // literal
    _src = ReadFixed<int>(_src, length);
    char* message; // string
    _src = ReadString(_src, message);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glPushDebugGroupKHR(source, id, length, message);
    // ---------- register handles ------------
}

static void retrace_glPopDebugGroupKHR(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glPopDebugGroupKHR();
    // ---------- register handles ------------
}

static void retrace_glObjectLabelKHR(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int identifier; // enum
    _src = ReadFixed(_src, identifier);
    unsigned int name; // literal
    _src = ReadFixed<unsigned int>(_src, name);
    int length; // literal
    _src = ReadFixed<int>(_src, length);
    char* label; // string
    _src = ReadString(_src, label);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glObjectLabelKHR(identifier, name, length, label);
    // ---------- register handles ------------
}

static void retrace_glObjectPtrLabelKHR(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int csbName = 0; // ClientSideBuffer name
    //  GLvoid* ptr = NULL;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    if (_opaque_type == BufferObjectReferenceType)
    { // simple memory offset
        unsigned int ptr_raw; // raw ptr
        _src = ReadFixed<unsigned int>(_src, ptr_raw);
        //      ptr = (GLvoid *)static_cast<uintptr_t>(ptr_raw);
    }
    else if (_opaque_type == BlobType)
    { // blob type
        Array<float> ptr_blob;
        _src = Read1DArray(_src, ptr_blob);
        //     ptr = ptr_blob.v;
    }
    else if (_opaque_type == ClientSideBufferObjectReferenceType)
    { // a memory reference to client-side buffer
        unsigned int offset = 0;
        _src = ReadFixed<unsigned int>(_src, csbName);
        _src = ReadFixed<unsigned int>(_src, offset);
   //     ptr = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);
    }
    else if (_opaque_type == NoopType)
    { // Do nothing
    }
    int length; // literal
    _src = ReadFixed<int>(_src, length);
    char* label; // string
    _src = ReadString(_src, label);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glObjectPtrLabelKHR(ptr, length, label);
    // ---------- register handles ------------
}

static void retrace_glMinSampleShadingOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    float value; // literal
    _src = ReadFixed<float>(_src, value);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glMinSampleShadingOES(value);
    // ---------- register handles ------------
}

static void retrace_glTexStorage3DMultisampleOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int samples; // literal
    _src = ReadFixed<int>(_src, samples);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);
    int depth; // literal
    _src = ReadFixed<int>(_src, depth);
    unsigned char fixedsamplelocations; // literal
    _src = ReadFixed<unsigned char>(_src, fixedsamplelocations);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glTexStorage3DMultisampleOES(target, samples, internalformat, width, height, depth, fixedsamplelocations);
    // ---------- register handles ------------
}

static void retrace_glEnableiEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glEnableiEXT(target, index);
    // ---------- register handles ------------
}

static void retrace_glDisableiEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glDisableiEXT(target, index);
    // ---------- register handles ------------
}

static void retrace_glBlendEquationiEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int buf; // literal
    _src = ReadFixed<unsigned int>(_src, buf);
    int mode; // enum
    _src = ReadFixed(_src, mode);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glBlendEquationiEXT(buf, mode);
    // ---------- register handles ------------
}

static void retrace_glBlendEquationSeparateiEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int buf; // literal
    _src = ReadFixed<unsigned int>(_src, buf);
    int modeRGB; // enum
    _src = ReadFixed(_src, modeRGB);
    int modeAlpha; // enum
    _src = ReadFixed(_src, modeAlpha);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glBlendEquationSeparateiEXT(buf, modeRGB, modeAlpha);
    // ---------- register handles ------------
}

static void retrace_glBlendFunciEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int buf; // literal
    _src = ReadFixed<unsigned int>(_src, buf);
    int src; // enum
    _src = ReadFixed(_src, src);
    int dst; // enum
    _src = ReadFixed(_src, dst);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glBlendFunciEXT(buf, src, dst);
    // ---------- register handles ------------
}

static void retrace_glBlendFuncSeparateiEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int buf; // literal
    _src = ReadFixed<unsigned int>(_src, buf);
    int srcRGB; // enum
    _src = ReadFixed(_src, srcRGB);
    int dstRGB; // enum
    _src = ReadFixed(_src, dstRGB);
    int srcAlpha; // enum
    _src = ReadFixed(_src, srcAlpha);
    int dstAlpha; // enum
    _src = ReadFixed(_src, dstAlpha);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glBlendFuncSeparateiEXT(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
    // ---------- register handles ------------
}

static void retrace_glColorMaskiEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    unsigned char r; // literal
    _src = ReadFixed<unsigned char>(_src, r);
    unsigned char g; // literal
    _src = ReadFixed<unsigned char>(_src, g);
    unsigned char b; // literal
    _src = ReadFixed<unsigned char>(_src, b);
    unsigned char a; // literal
    _src = ReadFixed<unsigned char>(_src, a);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glColorMaskiEXT(index, r, g, b, a)
    // ---------- register handles ------------
}

static void retrace_glEnableiOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glEnableiOES(target, index);
    // ---------- register handles ------------
}

static void retrace_glDisableiOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glDisableiOES(target, index);
    // ---------- register handles ------------
}

static void retrace_glBlendEquationiOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int buf; // literal
    _src = ReadFixed<unsigned int>(_src, buf);
    int mode; // enum
    _src = ReadFixed(_src, mode);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glBlendEquationiOES(buf, mode);
    // ---------- register handles ------------
}

static void retrace_glBlendEquationSeparateiOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int buf; // literal
    _src = ReadFixed<unsigned int>(_src, buf);
    int modeRGB; // enum
    _src = ReadFixed(_src, modeRGB);
    int modeAlpha; // enum
    _src = ReadFixed(_src, modeAlpha);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glBlendEquationSeparateiOES(buf, modeRGB, modeAlpha);
    // ---------- register handles ------------
}

static void retrace_glBlendFunciOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int buf; // literal
    _src = ReadFixed<unsigned int>(_src, buf);
    int src; // enum
    _src = ReadFixed(_src, src);
    int dst; // enum
    _src = ReadFixed(_src, dst);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glBlendFunciOES(buf, src, dst);
    // ---------- register handles ------------
}

static void retrace_glBlendFuncSeparateiOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int buf; // literal
    _src = ReadFixed<unsigned int>(_src, buf);
    int srcRGB; // enum
    _src = ReadFixed(_src, srcRGB);
    int dstRGB; // enum
    _src = ReadFixed(_src, dstRGB);
    int srcAlpha; // enum
    _src = ReadFixed(_src, srcAlpha);
    int dstAlpha; // enum
    _src = ReadFixed(_src, dstAlpha);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glBlendFuncSeparateiOES(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
    // ---------- register handles ------------
}

static void retrace_glColorMaskiOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    unsigned char r; // literal
    _src = ReadFixed<unsigned char>(_src, r);
    unsigned char g; // literal
    _src = ReadFixed<unsigned char>(_src, g);
    unsigned char b; // literal
    _src = ReadFixed<unsigned char>(_src, b);
    unsigned char a; // literal
    _src = ReadFixed<unsigned char>(_src, a);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glColorMaskiOES(index, r, g, b, a);
    // ---------- register handles ------------
}

static void retrace_glViewportArrayvOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int first; // literal
    _src = ReadFixed<unsigned int>(_src, first);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<float> v; // array
    _src = Read1DArray(_src, v);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glViewportArrayvOES(first, count, v);
    // ---------- register handles ------------
}

static void retrace_glViewportIndexedfOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    float x; // literal
    _src = ReadFixed<float>(_src, x);
    float y; // literal
    _src = ReadFixed<float>(_src, y);
    float w; // literal
    _src = ReadFixed<float>(_src, w);
    float h; // literal
    _src = ReadFixed<float>(_src, h);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //    glViewportIndexedfOES(index, x, y, w, h);
    // ---------- register handles ------------
}

static void retrace_glViewportIndexedfvOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    Array<float> v; // array
    _src = Read1DArray(_src, v);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glViewportIndexedfvOES(index, v);
    // ---------- register handles ------------
}

static void retrace_glScissorArrayvOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int first; // literal
    _src = ReadFixed<unsigned int>(_src, first);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<int> v; // array
    _src = Read1DArray(_src, v);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glScissorArrayvOES(first, count, v);
    // ---------- register handles ------------
}

static void retrace_glScissorIndexedOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    int left; // literal
    _src = ReadFixed<int>(_src, left);
    int bottom; // literal
    _src = ReadFixed<int>(_src, bottom);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glScissorIndexedOES(index, left, bottom, width, height);
    // ---------- register handles ------------
}

static void retrace_glScissorIndexedvOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    Array<int> v; // array
    _src = Read1DArray(_src, v);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glScissorIndexedvOES(index, v);
    // ---------- register handles ------------
}

static void retrace_glDepthRangeArrayfvOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int first; // literal
    _src = ReadFixed<unsigned int>(_src, first);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<float> v; // array
    _src = Read1DArray(_src, v);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glDepthRangeArrayfvOES(first, count, v);
    // ---------- register handles ------------
}

static void retrace_glDepthRangeIndexedfOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    float n; // literal
    _src = ReadFixed<float>(_src, n);
    float f; // literal
    _src = ReadFixed<float>(_src, f);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glDepthRangeIndexedfOES(index, n, f);
    // ---------- register handles ------------
}

static void retrace_glFramebufferTextureEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int attachment; // enum
    _src = ReadFixed(_src, attachment);
    unsigned int texture; // literal
    _src = ReadFixed<unsigned int>(_src, texture);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint textureNew = context.getTextureMap().RValue(texture);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glFramebufferTextureEXT(target, attachment, textureNew, level);
    // ---------- register handles ------------
    */
}

static void retrace_glPrimitiveBoundingBoxEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    float minX; // literal
    _src = ReadFixed<float>(_src, minX);
    float minY; // literal
    _src = ReadFixed<float>(_src, minY);
    float minZ; // literal
    _src = ReadFixed<float>(_src, minZ);
    float minW; // literal
    _src = ReadFixed<float>(_src, minW);
    float maxX; // literal
    _src = ReadFixed<float>(_src, maxX);
    float maxY; // literal
    _src = ReadFixed<float>(_src, maxY);
    float maxZ; // literal
    _src = ReadFixed<float>(_src, maxZ);
    float maxW; // literal
    _src = ReadFixed<float>(_src, maxW);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glPrimitiveBoundingBoxEXT(minX, minY, minZ, minW, maxX, maxY, maxZ, maxW);
    // ---------- register handles ------------
}

static void retrace_glPatchParameteriEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int pname; // enum
    _src = ReadFixed(_src, pname);
    int value; // literal
    _src = ReadFixed<int>(_src, value);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glPatchParameteriEXT(pname, value);
    // ---------- register handles ------------
    gRetracer.curContextState->curGlobalState.ff_glPatchParameteri(gRetracer.mCurCallNo);
}

static void retrace_glTexParameterIivEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<int> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glTexParameterIivEXT(target, pname, params);
    // ---------- register handles ------------
}

static void retrace_glTexParameterIuivEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<unsigned int> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glTexParameterIuivEXT(target, pname, params);
    // ---------- register handles ------------
}

static void retrace_glSamplerParameterIivEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int sampler; // literal
    _src = ReadFixed<unsigned int>(_src, sampler);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<int> params; // array
    _src = Read1DArray(_src, params);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint samplerNew = context.getSamplerMap().RValue(sampler);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glSamplerParameterIivEXT(samplerNew, pname, params);
    // ---------- register handles ------------
    */
}

static void retrace_glSamplerParameterIuivEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int sampler; // literal
    _src = ReadFixed<unsigned int>(_src, sampler);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<unsigned int> params; // array
    _src = Read1DArray(_src, params);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint samplerNew = context.getSamplerMap().RValue(sampler);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glSamplerParameterIuivEXT(samplerNew, pname, params);
    // ---------- register handles ------------
    */
}

static void retrace_glTexBufferEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    unsigned int buffer; // literal
    _src = ReadFixed<unsigned int>(_src, buffer);

    /*   // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint bufferNew = context.getBufferMap().RValue(buffer);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glTexBufferEXT(target, internalformat, bufferNew);
    // ---------- register handles ------------
    */
}

static void retrace_glTexBufferRangeEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    unsigned int buffer; // literal
    _src = ReadFixed<unsigned int>(_src, buffer);
    int offset; // literal
    _src = ReadFixed<int>(_src, offset);
    int size; // literal
    _src = ReadFixed<int>(_src, size);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint bufferNew = context.getBufferMap().RValue(buffer);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glTexBufferRangeEXT(target, internalformat, bufferNew, offset, size);
    // ---------- register handles ------------
    */
}

static void retrace_glCopyImageSubDataEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int srcName; // literal
    _src = ReadFixed<unsigned int>(_src, srcName);
    int srcTarget; // enum
    _src = ReadFixed(_src, srcTarget);
    int srcLevel; // literal
    _src = ReadFixed<int>(_src, srcLevel);
    int srcX; // literal
    _src = ReadFixed<int>(_src, srcX);
    int srcY; // literal
    _src = ReadFixed<int>(_src, srcY);
    int srcZ; // literal
    _src = ReadFixed<int>(_src, srcZ);
    unsigned int dstName; // literal
    _src = ReadFixed<unsigned int>(_src, dstName);
    int dstTarget; // enum
    _src = ReadFixed(_src, dstTarget);
    int dstLevel; // literal
    _src = ReadFixed<int>(_src, dstLevel);
    int dstX; // literal
    _src = ReadFixed<int>(_src, dstX);
    int dstY; // literal
    _src = ReadFixed<int>(_src, dstY);
    int dstZ; // literal
    _src = ReadFixed<int>(_src, dstZ);
    int srcWidth; // literal
    _src = ReadFixed<int>(_src, srcWidth);
    int srcHeight; // literal
    _src = ReadFixed<int>(_src, srcHeight);
    int srcDepth; // literal
    _src = ReadFixed<int>(_src, srcDepth);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    //  srcName = lookUpPolymorphic(srcName, srcTarget);
    //  dstName = lookUpPolymorphic(dstName, dstTarget);
    // ------------- retrace ------------------
    //  glCopyImageSubDataEXT(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth);
    // ---------- register handles ------------
}

static void retrace_glAlphaFuncQCOM(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int func; // enum
    _src = ReadFixed(_src, func);
    float ref; // literal
    _src = ReadFixed<float>(_src, ref);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glAlphaFuncQCOM(func, ref);
    // ---------- register handles ------------
}

static void retrace_glQueryCounterEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int id; // literal
    _src = ReadFixed<unsigned int>(_src, id);
    int target; // enum
    _src = ReadFixed(_src, target);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glQueryCounterEXT(id, target);
    // ---------- register handles ------------
}

static void retrace_glGenPerfMonitorsAMD(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> old_monitors; // array
    _src = Read1DArray(_src, old_monitors);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    //  std::vector<GLuint> _monitors(n);
    //  GLuint *monitors = _monitors.data();
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glGenPerfMonitorsAMD(n, monitors);
    // ---------- register handles ------------
    for (unsigned int i = 0; i < old_monitors.cnt; ++i)
    {
    }
}

static void retrace_glDeletePerfMonitorsAMD(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> monitors; // array
    _src = Read1DArray(_src, monitors);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glDeletePerfMonitorsAMD(n, monitors);
    // ---------- register handles ------------
}

static void retrace_glSelectPerfMonitorCountersAMD(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int monitor; // literal
    _src = ReadFixed<unsigned int>(_src, monitor);
    unsigned char enable; // literal
    _src = ReadFixed<unsigned char>(_src, enable);
    unsigned int group; // literal
    _src = ReadFixed<unsigned int>(_src, group);
    int numCounters; // literal
    _src = ReadFixed<int>(_src, numCounters);
    Array<unsigned int> counterList; // array
    _src = Read1DArray(_src, counterList);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glSelectPerfMonitorCountersAMD(monitor, enable, group, numCounters, counterList);
    // ---------- register handles ------------
}

static void retrace_glBeginPerfMonitorAMD(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int monitor; // literal
    _src = ReadFixed<unsigned int>(_src, monitor);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    // glBeginPerfMonitorAMD(monitor);
    // ---------- register handles ------------
}

static void retrace_glEndPerfMonitorAMD(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int monitor; // literal
    _src = ReadFixed<unsigned int>(_src, monitor);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    // glEndPerfMonitorAMD(monitor);
    // ---------- register handles ------------
}

static void retrace_glReadnPixels(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int x; // literal
    _src = ReadFixed<int>(_src, x);
    int y; // literal
    _src = ReadFixed<int>(_src, y);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);
    int format; // enum
    _src = ReadFixed(_src, format);
    int type; // enum
    _src = ReadFixed(_src, type);
    int bufSize; // literal
    _src = ReadFixed<int>(_src, bufSize);
    //GLvoid* old_pixels = NULL;
    //bool isPixelPackBufferBound = false;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    /*if (gRetracer.getFileFormatVersion() <= HEADER_VERSION_3)
    {
       unsigned int t = 0;
       _src = ReadFixed<unsigned int>(_src, t);
       // old_pixels will not be used
       old_pixels = (GLvoid *)static_cast<uintptr_t>(t);
    }
    else // if (gRetracer.getFileFormatVersion() > HEADER_VERSION_3)
    {
        if (_opaque_type == BufferObjectReferenceType)
        {
            isPixelPackBufferBound = true;
            unsigned int bufferOffset = 0;
            _src = ReadFixed<unsigned int>(_src, bufferOffset);
            old_pixels = (GLvoid *)static_cast<uintptr_t>(bufferOffset);
        }
        else if (_opaque_type == NoopType)
        {
        }
        else
        {
            DBG_LOG("ERROR: Unsupported opaque type\n");
        }
    }

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    GLvoid* pixels = 0;

    // If GLES3 or higher, the glReadPixels operation can
    // potentionally be meant to a PIXEL_PACK_BUFFER.
    if (!isPixelPackBufferBound)
    {
      //  size_t size = _gl_image_size(format, type, width, height, 1);
     //   pixels = new char[size];
    }
    else
    {
        // If a pixel pack buffer is bound, old_pixels is
        // treated as an offset
        pixels = old_pixels;
    }

    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glReadnPixels(x, y, width, height, format, type, bufSize, pixels);
    // ---------- register handles ------------
    if (!isPixelPackBufferBound)
    {
        delete[] static_cast<char*>(pixels);
    }
    */
}

static void retrace_glTexStorage3DMultisample(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int samples; // literal
    _src = ReadFixed<int>(_src, samples);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);
    int depth; // literal
    _src = ReadFixed<int>(_src, depth);
    unsigned char fixedsamplelocations; // literal
    _src = ReadFixed<unsigned char>(_src, fixedsamplelocations);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glTexStorage3DMultisample(target, samples, internalformat, width, height, depth, fixedsamplelocations);
    // ---------- register handles ------------
}

static void retrace_glEnablei(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glEnablei(target, index);
    // ---------- register handles ------------
}

static void retrace_glDisablei(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glDisablei(target, index);
    // ---------- register handles ------------
}

static void retrace_glBlendEquationi(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int buf; // literal
    _src = ReadFixed<unsigned int>(_src, buf);
    int mode; // enum
    _src = ReadFixed(_src, mode);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glBlendEquationi(buf, mode);
    // ---------- register handles ------------
}

static void retrace_glBlendEquationSeparatei(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int buf; // literal
    _src = ReadFixed<unsigned int>(_src, buf);
    int modeRGB; // enum
    _src = ReadFixed(_src, modeRGB);
    int modeAlpha; // enum
    _src = ReadFixed(_src, modeAlpha);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glBlendEquationSeparatei(buf, modeRGB, modeAlpha);
    // ---------- register handles ------------
}

static void retrace_glBlendFunci(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int buf; // literal
    _src = ReadFixed<unsigned int>(_src, buf);
    int src; // enum
    _src = ReadFixed(_src, src);
    int dst; // enum
    _src = ReadFixed(_src, dst);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glBlendFunci(buf, src, dst);
    // ---------- register handles ------------
}

static void retrace_glBlendFuncSeparatei(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int buf; // literal
    _src = ReadFixed<unsigned int>(_src, buf);
    int srcRGB; // enum
    _src = ReadFixed(_src, srcRGB);
    int dstRGB; // enum
    _src = ReadFixed(_src, dstRGB);
    int srcAlpha; // enum
    _src = ReadFixed(_src, srcAlpha);
    int dstAlpha; // enum
    _src = ReadFixed(_src, dstAlpha);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glBlendFuncSeparatei(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
    // ---------- register handles ------------
}

static void retrace_glColorMaski(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    unsigned char r; // literal
    _src = ReadFixed<unsigned char>(_src, r);
    unsigned char g; // literal
    _src = ReadFixed<unsigned char>(_src, g);
    unsigned char b; // literal
    _src = ReadFixed<unsigned char>(_src, b);
    unsigned char a; // literal
    _src = ReadFixed<unsigned char>(_src, a);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glColorMaski(index, r, g, b, a);
    // ---------- register handles ------------
}

static void retrace_glFramebufferTexture(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int attachment; // enum
    _src = ReadFixed(_src, attachment);
    unsigned int texture; // literal
    _src = ReadFixed<unsigned int>(_src, texture);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint textureNew = context.getTextureMap().RValue(texture);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glFramebufferTexture(target, attachment, textureNew, level);
    // ---------- register handles ------------
    */
}

static void retrace_glPrimitiveBoundingBox(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    float minX; // literal
    _src = ReadFixed<float>(_src, minX);
    float minY; // literal
    _src = ReadFixed<float>(_src, minY);
    float minZ; // literal
    _src = ReadFixed<float>(_src, minZ);
    float minW; // literal
    _src = ReadFixed<float>(_src, minW);
    float maxX; // literal
    _src = ReadFixed<float>(_src, maxX);
    float maxY; // literal
    _src = ReadFixed<float>(_src, maxY);
    float maxZ; // literal
    _src = ReadFixed<float>(_src, maxZ);
    float maxW; // literal
    _src = ReadFixed<float>(_src, maxW);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glPrimitiveBoundingBox(minX, minY, minZ, minW, maxX, maxY, maxZ, maxW);
    // ---------- register handles ------------
}

static void retrace_glPatchParameteri(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int pname; // enum
    _src = ReadFixed(_src, pname);
    int value; // literal
    _src = ReadFixed<int>(_src, value);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glPatchParameteri(pname, value);
    // ---------- register handles ------------
    gRetracer.curContextState->curGlobalState.ff_glPatchParameteri(gRetracer.mCurCallNo);
}

static void retrace_glTexParameterIiv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<int> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glTexParameterIiv(target, pname, params);
    // ---------- register handles ------------
}

static void retrace_glTexParameterIuiv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<unsigned int> params; // array
    _src = Read1DArray(_src, params);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glTexParameterIuiv(target, pname, params);
    // ---------- register handles ------------
}

static void retrace_glSamplerParameterIiv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int sampler; // literal
    _src = ReadFixed<unsigned int>(_src, sampler);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<int> params; // array
    _src = Read1DArray(_src, params);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint samplerNew = context.getSamplerMap().RValue(sampler);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glSamplerParameterIiv(samplerNew, pname, params);
    // ---------- register handles ------------
    */
}

static void retrace_glSamplerParameterIuiv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int sampler; // literal
    _src = ReadFixed<unsigned int>(_src, sampler);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<unsigned int> params; // array
    _src = Read1DArray(_src, params);

    /*    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint samplerNew = context.getSamplerMap().RValue(sampler);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glSamplerParameterIuiv(samplerNew, pname, params);
    // ---------- register handles ------------
    */
}

static void retrace_glTexBuffer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    unsigned int buffer; // literal
    _src = ReadFixed<unsigned int>(_src, buffer);

    /*   // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint bufferNew = context.getBufferMap().RValue(buffer);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glTexBuffer(target, internalformat, bufferNew);
    // ---------- register handles ------------
    */
}

static void retrace_glTexBufferRange(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    unsigned int buffer; // literal
    _src = ReadFixed<unsigned int>(_src, buffer);
    int offset; // literal
    _src = ReadFixed<int>(_src, offset);
    int size; // literal
    _src = ReadFixed<int>(_src, size);

    /*   // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint bufferNew = context.getBufferMap().RValue(buffer);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glTexBufferRange(target, internalformat, bufferNew, offset, size);
    // ---------- register handles ------------
    */
}

static void retrace_glCopyImageSubData(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int srcName; // literal
    _src = ReadFixed<unsigned int>(_src, srcName);
    int srcTarget; // enum
    _src = ReadFixed(_src, srcTarget);
    int srcLevel; // literal
    _src = ReadFixed<int>(_src, srcLevel);
    int srcX; // literal
    _src = ReadFixed<int>(_src, srcX);
    int srcY; // literal
    _src = ReadFixed<int>(_src, srcY);
    int srcZ; // literal
    _src = ReadFixed<int>(_src, srcZ);
    unsigned int dstName; // literal
    _src = ReadFixed<unsigned int>(_src, dstName);
    int dstTarget; // enum
    _src = ReadFixed(_src, dstTarget);
    int dstLevel; // literal
    _src = ReadFixed<int>(_src, dstLevel);
    int dstX; // literal
    _src = ReadFixed<int>(_src, dstX);
    int dstY; // literal
    _src = ReadFixed<int>(_src, dstY);
    int dstZ; // literal
    _src = ReadFixed<int>(_src, dstZ);
    int srcWidth; // literal
    _src = ReadFixed<int>(_src, srcWidth);
    int srcHeight; // literal
    _src = ReadFixed<int>(_src, srcHeight);
    int srcDepth; // literal
    _src = ReadFixed<int>(_src, srcDepth);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glCopyImageSubData(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth);
    // ---------- register handles ------------

    //gRetracer.callNoList.insert(gRetracer.mCurCallNo);
    gRetracer.curContextState->curTextureState.ff_glCopyImageSubData(srcName, dstName, gRetracer.mCurCallNo);


    if(gRetracer.preExecute == false)
    {
        std::vector<unsigned int> textureList;
        textureList.push_back(srcName);
        gRetracer.curPreExecuteState.newInsertTextureListNo(textureList,gRetracer.curPreExecuteState.newQueryTextureNoList(dstName));
    }//if(gRetracer.preExecute == false)

}

static void retrace_glDrawElementsBaseVertex(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int mode; // enum
    _src = ReadFixed(_src, mode);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    int type; // enum
    _src = ReadFixed(_src, type);
    unsigned int csbName = 0; // ClientSideBuffer name
    //  GLvoid* indices = NULL;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    if (_opaque_type == BufferObjectReferenceType)
    {
        // simple memory offset
        unsigned int indices_raw; // raw ptr
        _src = ReadFixed<unsigned int>(_src, indices_raw);
        //        indices = (GLvoid *)static_cast<uintptr_t>(indices_raw);
    }
    else if (_opaque_type == BlobType)
    {
        // blob type
        Array<float> indices_blob;
        _src = Read1DArray(_src, indices_blob);
        //       indices = indices_blob.v;
    }
    else if (_opaque_type == ClientSideBufferObjectReferenceType)
    {
        // a memory reference to client-side buffer
        unsigned int offset = 0;
        _src = ReadFixed<unsigned int>(_src, csbName);
        _src = ReadFixed<unsigned int>(_src, offset);
        //        indices = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);
    }
    else if (_opaque_type == NoopType)
    { // Do nothing
    }
    int basevertex; // literal
    _src = ReadFixed<int>(_src, basevertex);
    /*
    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    gRetracer.IncCurDrawId();
    if (unlikely(stateLoggingEnabled)) {
        gRetracer.getStateLogger().logFunction(gRetracer.getCurTid(), "glDrawElementsBaseVertex", gRetracer.GetCurCallId(), gRetracer.GetCurDrawId());
        gRetracer.getStateLogger().logState(gRetracer.getCurTid(), count, type, indices, 0);
    }
    //    pre_glDraw();
    // ------------- retrace ------------------
    glDrawElementsBaseVertex(mode, count, type, indices, basevertex);
    // ---------- register handles ------------
    */
}

static void retrace_glDrawRangeElementsBaseVertex(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int mode; // enum
    _src = ReadFixed(_src, mode);
    unsigned int start; // literal
    _src = ReadFixed<unsigned int>(_src, start);
    unsigned int end; // literal
    _src = ReadFixed<unsigned int>(_src, end);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    int type; // enum
    _src = ReadFixed(_src, type);
    unsigned int csbName = 0; // ClientSideBuffer name
    //   GLvoid* indices = NULL;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    if (_opaque_type == BufferObjectReferenceType)
    {
        // simple memory offset
        unsigned int indices_raw; // raw ptr
        _src = ReadFixed<unsigned int>(_src, indices_raw);
        //       indices = (GLvoid *)static_cast<uintptr_t>(indices_raw);
    }
    else if (_opaque_type == BlobType)
    {
      // blob type
        Array<float> indices_blob;
        _src = Read1DArray(_src, indices_blob);
        //      indices = indices_blob.v;
    }
    else if (_opaque_type == ClientSideBufferObjectReferenceType)
    { // a memory reference to client-side buffer
        unsigned int offset = 0;
        _src = ReadFixed<unsigned int>(_src, csbName);
        _src = ReadFixed<unsigned int>(_src, offset);
        //       indices = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);
    }
    else if (_opaque_type == NoopType)
    { // Do nothing
    }
    int basevertex; // literal
    _src = ReadFixed<int>(_src, basevertex);

    /*    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    gRetracer.IncCurDrawId();
    if (unlikely(stateLoggingEnabled)) {
        gRetracer.getStateLogger().logFunction(gRetracer.getCurTid(), "glDrawRangeElementsBaseVertex", gRetracer.GetCurCallId(), gRetracer.GetCurDrawId());
        gRetracer.getStateLogger().logState(gRetracer.getCurTid(), count, type, indices, 0);
    }
    //    pre_glDraw();
    // ------------- retrace ------------------
    glDrawRangeElementsBaseVertex(mode, start, end, count, type, indices, basevertex);
    // ---------- register handles ------------
    */
}

static void retrace_glDrawElementsInstancedBaseVertex(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int mode; // enum
    _src = ReadFixed(_src, mode);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    int type; // enum
    _src = ReadFixed(_src, type);
    unsigned int csbName = 0; // ClientSideBuffer name
    //    GLvoid* indices = NULL;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    if (_opaque_type == BufferObjectReferenceType) { // simple memory offset
        unsigned int indices_raw; // raw ptr
        _src = ReadFixed<unsigned int>(_src, indices_raw);
        //        indices = (GLvoid *)static_cast<uintptr_t>(indices_raw);
    } else if (_opaque_type == BlobType)
    { // blob type
        Array<float> indices_blob;
        _src = Read1DArray(_src, indices_blob);
        //        indices = indices_blob.v;
    } else if (_opaque_type == ClientSideBufferObjectReferenceType)
    { // a memory reference to client-side buffer
        unsigned int offset = 0;
        _src = ReadFixed<unsigned int>(_src, csbName);
        _src = ReadFixed<unsigned int>(_src, offset);
        //        indices = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);
    } else if (_opaque_type == NoopType)
    { // Do nothing
    }
    int instancecount; // literal
    _src = ReadFixed<int>(_src, instancecount);
    int basevertex; // literal
    _src = ReadFixed<int>(_src, basevertex);

    /*    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    gRetracer.IncCurDrawId();
    if (unlikely(stateLoggingEnabled)) {
        gRetracer.getStateLogger().logFunction(gRetracer.getCurTid(), "glDrawElementsInstancedBaseVertex", gRetracer.GetCurCallId(), gRetracer.GetCurDrawId());
        gRetracer.getStateLogger().logState(gRetracer.getCurTid(), count, type, indices, instancecount);
    }
    //    pre_glDraw();
    // ------------- retrace ------------------
    glDrawElementsInstancedBaseVertex(mode, count, type, indices, instancecount, basevertex);
    // ---------- register handles ------------
    */
}

static void retrace_glBlendBarrier(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glBlendBarrier();
    // ---------- register handles ------------
}

static void retrace_glMinSampleShading(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    float value; // literal
    _src = ReadFixed<float>(_src, value);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glMinSampleShading(value);
    // ---------- register handles ------------
}

static void retrace_glTexStorage1DEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int levels; // literal
    _src = ReadFixed<int>(_src, levels);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    int width; // literal
    _src = ReadFixed<int>(_src, width);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glTexStorage1DEXT(target, levels, internalformat, width);
    // ---------- register handles ------------
}

static void retrace_glTexStorage2DEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int levels; // literal
    _src = ReadFixed<int>(_src, levels);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glTexStorage2DEXT(target, levels, internalformat, width, height);
    // ---------- register handles ------------
    std::unordered_set<unsigned int> newList;

    gRetracer.curContextState->curTextureState.ff_glTypes_TexImage2D(target, 0, gRetracer.mCurCallNo, newList);
}

static void retrace_glTexStorage3DEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int levels; // literal
    _src = ReadFixed<int>(_src, levels);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);
    int depth; // literal
    _src = ReadFixed<int>(_src, depth);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glTexStorage3DEXT(target, levels, internalformat, width, height, depth);
    // ---------- register handles ------------

    std::unordered_set<unsigned int> newList;
    gRetracer.curContextState->curTextureState.ff_glTypes_TexImage2D(target, 0, gRetracer.mCurCallNo, newList);
}

static void retrace_glPatchParameteriOES(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int pname; // enum
    _src = ReadFixed(_src, pname);
    int value; // literal
    _src = ReadFixed<int>(_src, value);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glPatchParameteriOES(pname, value);
    // ---------- register handles ------------
}

static void retrace_glFramebufferTextureMultiviewOVR(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int attachment; // enum
    _src = ReadFixed(_src, attachment);
    unsigned int texture; // literal
    _src = ReadFixed<unsigned int>(_src, texture);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int baseViewIndex; // literal
    _src = ReadFixed<int>(_src, baseViewIndex);
    int numViews; // literal
    _src = ReadFixed<int>(_src, numViews);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glFramebufferTextureMultiviewOVR(target, attachment, texture, level, baseViewIndex, numViews);
    // ---------- register handles ------------
}

static void retrace_glFramebufferTextureMultisampleMultiviewOVR(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int attachment; // enum
    _src = ReadFixed(_src, attachment);
    unsigned int texture; // literal
    _src = ReadFixed<unsigned int>(_src, texture);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int samples; // literal
    _src = ReadFixed<int>(_src, samples);
    int baseViewIndex; // literal
    _src = ReadFixed<int>(_src, baseViewIndex);
    int numViews; // literal
    _src = ReadFixed<int>(_src, numViews);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glFramebufferTextureMultisampleMultiviewOVR(target, attachment, texture, level, samples, baseViewIndex, numViews);
    // ---------- register handles ------------
}

static void retrace_glClearTexImageEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int texture; // literal
    _src = ReadFixed<unsigned int>(_src, texture);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int format; // enum
    _src = ReadFixed(_src, format);
    int type; // enum
    _src = ReadFixed(_src, type);
    Array<char> data; // blob
    _src = Read1DArray(_src, data);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint textureNew = context.getTextureMap().RValue(texture);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glClearTexImageEXT(textureNew, level, format, type, data);
    // ---------- register handles ------------
    */
}

static void retrace_glClearTexSubImageEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int texture; // literal
    _src = ReadFixed<unsigned int>(_src, texture);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int xoffset; // literal
    _src = ReadFixed<int>(_src, xoffset);
    int yoffset; // literal
    _src = ReadFixed<int>(_src, yoffset);
    int zoffset; // literal
    _src = ReadFixed<int>(_src, zoffset);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);
    int depth; // literal
    _src = ReadFixed<int>(_src, depth);
    int format; // enum
    _src = ReadFixed(_src, format);
    int type; // enum
    _src = ReadFixed(_src, type);
    Array<char> data; // blob
    _src = Read1DArray(_src, data);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint textureNew = context.getTextureMap().RValue(texture);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glClearTexSubImageEXT(textureNew, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data);
    // ---------- register handles ------------
    */
}

static void retrace_glBufferStorageEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int size; // literal
    _src = ReadFixed<int>(_src, size);
    Array<char> data; // blob
    _src = Read1DArray(_src, data);
    unsigned int flags; // literal
    _src = ReadFixed<unsigned int>(_src, flags);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glBufferStorageEXT(target, size, data, flags);
    // ---------- register handles ------------
}

static void retrace_glDrawTransformFeedbackEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int mode; // enum
    _src = ReadFixed(_src, mode);
    unsigned int id; // literal
    _src = ReadFixed<unsigned int>(_src, id);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glDrawTransformFeedbackEXT(mode, id);
    // ---------- register handles ------------
}

static void retrace_glDrawTransformFeedbackInstancedEXT(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int mode; // enum
    _src = ReadFixed(_src, mode);
    unsigned int id; // literal
    _src = ReadFixed<unsigned int>(_src, id);
    int instancecount; // literal
    _src = ReadFixed<int>(_src, instancecount);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glDrawTransformFeedbackInstancedEXT(mode, id, instancecount);
    // ---------- register handles ------------
}

static void retrace_glActiveShaderProgram(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int pipeline; // literal
    _src = ReadFixed<unsigned int>(_src, pipeline);
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint pipelineNew = context._pipeline_map.RValue(pipeline);
    GLuint programNew = context.getProgramMap().RValue(program);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glActiveShaderProgram(pipelineNew, programNew);
    // ---------- register handles ------------
    */
}

static void retrace_glBeginQuery(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    unsigned int id; // literal
    _src = ReadFixed<unsigned int>(_src, id);

    /*   // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint idNew = context._query_map.RValue(id);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glBeginQuery(target, idNew);
    // ---------- register handles ------------
    */
}

static void retrace_glBeginTransformFeedback(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int primitiveMode; // enum
    _src = ReadFixed(_src, primitiveMode);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glBeginTransformFeedback(primitiveMode);
    // ---------- register handles ------------
    gRetracer.callNoList.insert(gRetracer.mCurCallNo);

    gRetracer.curContextState->curGlobalState.TransformFeedbackSwitch = true;
}

static void retrace_glBindBufferBase(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    unsigned int buffer; // literal
    _src = ReadFixed<unsigned int>(_src, buffer);

    /*   // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint bufferNew = context.getBufferMap().RValue(buffer);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glBindBufferBase(target, index, bufferNew);
    // ---------- register handles ------------
    */
    if(target == GL_TRANSFORM_FEEDBACK_BUFFER)
    {
        gRetracer.callNoList.insert(gRetracer.mCurCallNo);
        if(buffer != 0)
        {
            gRetracer.curContextState->curBufferState.findABufferState(buffer, gRetracer.callNoList);
        }
    }
    if(target == GL_SHADER_STORAGE_BUFFER || target == GL_UNIFORM_BUFFER)
    {
        if(index == 0)
        {
            gRetracer.curContextState->curBufferState.ff_glBindBuffer(target+31, buffer, gRetracer.mCurCallNo);
        }
        else if(index >=1 && index < 32)//max GL_SHADER_STORAGE_BUFFER num is 32
        {
            gRetracer.curContextState->curBufferState.ff_glBindBuffer(target+index, buffer, gRetracer.mCurCallNo);
        }
        else
        {
            DBG_LOG("bindbufferbase error: no support index %d\n", index);
        }//for chineseghoststory
    }

    if(target == GL_ATOMIC_COUNTER_BUFFER)
    {
        gRetracer.curContextState->curBufferState.ff_glBindBuffer(target, buffer, gRetracer.mCurCallNo);
    }
}

static void retrace_glBindBufferRange(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    unsigned int buffer; // literal
    _src = ReadFixed<unsigned int>(_src, buffer);
    int offset; // literal
    _src = ReadFixed<int>(_src, offset);
    int size; // literal
    _src = ReadFixed<int>(_src, size);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint bufferNew = context.getBufferMap().RValue(buffer);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glBindBufferRange(target, index, bufferNew, offset, size);
    // ---------- register handles ------------
    */
    if(target == GL_TRANSFORM_FEEDBACK_BUFFER)
    {
        gRetracer.callNoList.insert(gRetracer.mCurCallNo);
        if(buffer != 0)
        {
            gRetracer.curContextState->curBufferState.findABufferState(buffer, gRetracer.callNoList);
        }
    }
    if(target == GL_SHADER_STORAGE_BUFFER || target == GL_UNIFORM_BUFFER)
    {
        if(index == 0)
        {
            gRetracer.curContextState->curBufferState.ff_glBindBuffer(target+31, buffer, gRetracer.mCurCallNo);
        }
        else if(index >= 1 && index < 32)//max GL_SHADER_STORAGE_BUFFER num is 32
        {
            gRetracer.curContextState->curBufferState.ff_glBindBuffer(target+index, buffer, gRetracer.mCurCallNo);
        }
        else
        {
            DBG_LOG("bindbufferbase error: no support index %d\n", index);
        }
    }
}

static void retrace_glBindImageTexture(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int unit; // literal
    _src = ReadFixed<unsigned int>(_src, unit);
    unsigned int texture; // literal
    _src = ReadFixed<unsigned int>(_src, texture);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    unsigned char layered; // literal
    _src = ReadFixed<unsigned char>(_src, layered);
    int layer; // literal
    _src = ReadFixed<int>(_src, layer);
    int access; // enum
    _src = ReadFixed(_src, access);
    int format; // enum
    _src = ReadFixed(_src, format);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint textureNew = context.getTextureMap().RValue(texture);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glBindImageTexture(unit, textureNew, level, layered, layer, access, format);
    // ---------- register handles ------------
    */
    gRetracer.curContextState->curTextureState.ff_glBindImageTexture(unit,texture, access, gRetracer.mCurCallNo);
}

static void retrace_glBindProgramPipeline(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int pipeline; // literal
    _src = ReadFixed<unsigned int>(_src, pipeline);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint pipelineNew = context._pipeline_map.RValue(pipeline);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glBindProgramPipeline(pipelineNew);
    // ---------- register handles ------------
    */
}

static void retrace_glBindSampler(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int unit; // literal
    _src = ReadFixed<unsigned int>(_src, unit);
    unsigned int sampler; // literal
    _src = ReadFixed<unsigned int>(_src, sampler);

    /*   // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    unsigned int samplerNew = context.getSamplerMap().RValue(sampler);
    if (samplerNew == 0 && sampler != 0)
    {
        glGenSamplers(1, &samplerNew);
        context.getSamplerMap().LValue(sampler) = samplerNew;
    }
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glBindSampler(unit, samplerNew);
    // ---------- register handles ------------
    */
    gRetracer.curContextState->curTextureState.ff_glBindSampler(unit, sampler, gRetracer.mCurCallNo);
}

static void retrace_glBindTransformFeedback(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    unsigned int id; // literal
    _src = ReadFixed<unsigned int>(_src, id);

    /*   // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    unsigned int idNew = context._feedback_map.RValue(id);
    if (idNew == 0 && id != 0)
    {
        glGenTransformFeedbacks(1, &idNew);
        context._feedback_map.LValue(id) = idNew;
    }
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glBindTransformFeedback(target, idNew);
    // ---------- register handles ------------
    */
}

static void retrace_glBindVertexArray(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int array; // literal
    _src = ReadFixed<unsigned int>(_src, array);

    /*   // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    unsigned int arrayNew = context._array_map.RValue(array);
    if (arrayNew == 0 && array != 0)
    {
        glGenVertexArrays(1, &arrayNew);
        context._array_map.LValue(array) = arrayNew;
    }
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glBindVertexArray(arrayNew);
    // ---------- register handles ------------
    */
    if(array == 0 && gRetracer.preExecute == true)
    {
        gRetracer.callNoList.insert(gRetracer.mCurCallNo);
    }
    if(array == 0)
    {
        array = 65535;//default vao index is 65535
    }
    gRetracer.curContextState->curBufferState.ff_glBindVertexArray(array, gRetracer.mCurCallNo);
}

static void retrace_glBindVertexBuffer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int bindingindex; // literal
    _src = ReadFixed<unsigned int>(_src, bindingindex);
    unsigned int buffer; // literal
    _src = ReadFixed<unsigned int>(_src, buffer);
    int offset; // literal
    _src = ReadFixed<int>(_src, offset);
    int stride; // literal
    _src = ReadFixed<int>(_src, stride);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint bufferNew = context.getBufferMap().RValue(buffer);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glBindVertexBuffer(bindingindex, bufferNew, offset, stride);
    // ---------- register handles ------------
    */
}

static void retrace_glBlitFramebuffer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int srcX0; // literal
    _src = ReadFixed<int>(_src, srcX0);
    int srcY0; // literal
    _src = ReadFixed<int>(_src, srcY0);
    int srcX1; // literal
    _src = ReadFixed<int>(_src, srcX1);
    int srcY1; // literal
    _src = ReadFixed<int>(_src, srcY1);
    int dstX0; // literal
    _src = ReadFixed<int>(_src, dstX0);
    int dstY0; // literal
    _src = ReadFixed<int>(_src, dstY0);
    int dstX1; // literal
    _src = ReadFixed<int>(_src, dstX1);
    int dstY1; // literal
    _src = ReadFixed<int>(_src, dstY1);
    unsigned int mask; // literal
    _src = ReadFixed<unsigned int>(_src, mask);
    int filter; // enum
    _src = ReadFixed(_src, filter);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    gRetracer.IncCurDrawId();
    // ------------- retrace ------------------
    //   glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    // ---------- register handles ------------
    gRetracer.curContextState->curFrameBufferState.ff_glBlitFramebuffer(gRetracer.mCurCallNo);

    //...from draw
    if((gRetracer.preExecute == true && gRetracer.mCurCallNo > gRetracer.curPreExecuteState.endDrawCallNo) && gRetracer.mOptions.allDraws == false)
    {
        gRetracer.curPreExecuteState.newFinalPotr++;
        gRetracer.curPreExecuteState.beginDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)/FIRST_10_BIT;
        gRetracer.curPreExecuteState.endDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)%FIRST_10_BIT;
    }
    std::vector<unsigned int> textureList;

    //debug for fast
    if(gRetracer.preExecute == true && ((gRetracer.mCurCallNo >= gRetracer.curPreExecuteState.beginDrawCallNo &&
gRetracer.mCurCallNo <= gRetracer.curPreExecuteState.endDrawCallNo) || gRetracer.mOptions.allDraws == true))//*(gRetracer.curPreExecuteState.finalPotr) == gRetracer.mCurCallNo)
    {
        if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
        {
            if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true)
            {
                gRetracer.curContextState->curFrameBufferState.readCurFrameBufferState(textureList, gRetracer.callNoList);
                gRetracer.curContextState->curTextureState.findTextureContext(textureList, gRetracer.callNoList);
                unsigned int curDrawBindNo = gRetracer.curContextState->curFrameBufferState.getDrawTextureNo();
                unsigned int curReadBindNo = gRetracer.curContextState->curFrameBufferState.getReadTextureNo();
                gRetracer.callNoList.insert(curDrawBindNo);
                gRetracer.callNoList.insert(curReadBindNo);
                gRetracer.callNoList.insert(gRetracer.mCurCallNo);
                gRetracer.curFrameStoreState.setEndDraw(true);
            }
        }//for debug
        textureList.clear();
    }//if(gRetracer.preExecute == true)
    else if(gRetracer.preExecute == false)
    {
        gRetracer.curPreExecuteState.drawCallNo++;
        if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
        {
            if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true)
            {
                int drawTextureIdx = gRetracer.curContextState->curFrameBufferState.getDrawTextureIdx();
                int readTextureIdx = gRetracer.curContextState->curFrameBufferState.getReadTextureIdx();
                textureList.push_back(readTextureIdx);
                gRetracer.curPreExecuteState.newInsertCallIntoList(false, drawTextureIdx);
                gRetracer.curPreExecuteState.newInsertTextureListNo(textureList, gRetracer.curPreExecuteState.newQueryTextureNoList(drawTextureIdx));
                gRetracer.curPreExecuteState.newInsertOneCallNo(false, drawTextureIdx, gRetracer.mCurCallNo);
            }
        }//for debug
        textureList.clear();
    }//if(gRetracer.preExecute == true)

    if(gRetracer.curPreExecuteState.endPreExecute == false)
    {
        if(gRetracer.mCurFrameNo > gRetracer.mOptions.fastforwadFrame1)
        {
            gRetracer.curPreExecuteState.newInsertCallIntoList(false, NO_IDX);
            gRetracer.curPreExecuteState.endPreExecute = true;
        }
    }
}

static void retrace_glClearBufferfi(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int buffer; // enum
    _src = ReadFixed(_src, buffer);
    int drawbuffer; // literal
    _src = ReadFixed<int>(_src, drawbuffer);
    float depth; // literal
    _src = ReadFixed<float>(_src, depth);
    int stencil; // literal
    _src = ReadFixed<int>(_src, stencil);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    gRetracer.IncCurDrawId();
    // ------------- retrace ------------------
    //    glClearBufferfi(buffer, drawbuffer, depth, stencil);
    // ---------- register handles ------------
    unsigned int mask = 0;
    if(buffer == GL_COLOR)
    {
        mask = 16384;
    }
    else if(buffer == GL_DEPTH)
    {
        mask = 256;
    }
    else if(buffer == GL_STENCIL)
    {
        mask = 1024;
    }
    else if(buffer == GL_DEPTH_STENCIL)
    {
        mask = 1280;
    }

    if((gRetracer.preExecute == true && gRetracer.mCurCallNo > gRetracer.curPreExecuteState.endDrawCallNo) && gRetracer.mOptions.allDraws == false)
    {
        gRetracer.curPreExecuteState.newFinalPotr++;
        gRetracer.curPreExecuteState.beginDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)/FIRST_10_BIT;
        gRetracer.curPreExecuteState.endDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)%FIRST_10_BIT;
    }

    //std::vector<unsigned int> textureList;
    if(gRetracer.preExecute == true && (gRetracer.mCurCallNo >= gRetracer.curPreExecuteState.beginDrawCallNo && gRetracer.mCurCallNo <= gRetracer.curPreExecuteState.endDrawCallNo))
    {
    }

    if(gRetracer.preExecute == true && ((gRetracer.mCurCallNo >= gRetracer.curPreExecuteState.beginDrawCallNo && gRetracer.mCurCallNo <= gRetracer.curPreExecuteState.endDrawCallNo) || gRetracer.mOptions.allDraws == true))
    {
        if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
        {
            if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true)
            {
                std::vector<unsigned int> textureList;
                gRetracer.curContextState->curFrameBufferState.readCurFrameBufferState(textureList, gRetracer.callNoList);
                gRetracer.callNoList.insert(gRetracer.mCurCallNo);
                gRetracer.curFrameStoreState.setEndDrawClear(true);
            }
        }
    }
    else if(gRetracer.preExecute == false)
    {
        gRetracer.curPreExecuteState.drawCallNo++;
        bool clearOrNot = false;
        if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true && mask >= gRetracer.mOptions.clearMask)
        {
            int drawTextureIdx = gRetracer.curContextState->curFrameBufferState.getDrawTextureIdx();
            gRetracer.curPreExecuteState.clearTextureNoList(drawTextureIdx);
            clearOrNot = true;
        }
        if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
        {
            if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true)
            {
                int drawTextureIdx = gRetracer.curContextState->curFrameBufferState.getDrawTextureIdx();
                gRetracer.curPreExecuteState.newInsertCallIntoList(clearOrNot, drawTextureIdx);
                gRetracer.curPreExecuteState.newInsertOneCallNo(clearOrNot, drawTextureIdx, gRetracer.mCurCallNo);
            }
        }
    }//else if
}

static void retrace_glClearBufferfv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int buffer; // enum
    _src = ReadFixed(_src, buffer);
    int drawbuffer; // literal
    _src = ReadFixed<int>(_src, drawbuffer);
    Array<float> value; // array
    _src = Read1DArray(_src, value);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    gRetracer.IncCurDrawId();
    // ------------- retrace ------------------
    // glClearBufferfv(buffer, drawbuffer, value);
    // ---------- register handles ------------
    unsigned int mask = 0;
    if(buffer == GL_COLOR)
    {
        mask = 16384;
    }
    else if(buffer == GL_DEPTH)
    {
        mask = 256;
    }
    else if(buffer == GL_STENCIL)
    {
        mask = 1024;
    }
    else if(buffer == GL_DEPTH_STENCIL)
    {
        mask = 1280;
    }

    if((gRetracer.preExecute == true && gRetracer.mCurCallNo > gRetracer.curPreExecuteState.endDrawCallNo) && gRetracer.mOptions.allDraws == false)
    {
        gRetracer.curPreExecuteState.newFinalPotr++;
        gRetracer.curPreExecuteState.beginDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)/FIRST_10_BIT;
        gRetracer.curPreExecuteState.endDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)%FIRST_10_BIT;
    }

    //std::vector<unsigned int> textureList;
    if(gRetracer.preExecute == true && (gRetracer.mCurCallNo >= gRetracer.curPreExecuteState.beginDrawCallNo && gRetracer.mCurCallNo <= gRetracer.curPreExecuteState.endDrawCallNo))
    {
    }

    if(gRetracer.preExecute == true && ((gRetracer.mCurCallNo >= gRetracer.curPreExecuteState.beginDrawCallNo && gRetracer.mCurCallNo <= gRetracer.curPreExecuteState.endDrawCallNo) || gRetracer.mOptions.allDraws == true))
    {
        if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
        {
            if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true)
            {
                std::vector<unsigned int> textureList;
                gRetracer.curContextState->curFrameBufferState.readCurFrameBufferState(textureList, gRetracer.callNoList);
                gRetracer.callNoList.insert(gRetracer.mCurCallNo);
                gRetracer.curFrameStoreState.setEndDrawClear(true);
            }
        }
    }
    else if(gRetracer.preExecute == false)
    {
        gRetracer.curPreExecuteState.drawCallNo++;
        bool clearOrNot = false;
        if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true && mask >= gRetracer.mOptions.clearMask)
        {
            int drawTextureIdx = gRetracer.curContextState->curFrameBufferState.getDrawTextureIdx();
            gRetracer.curPreExecuteState.clearTextureNoList(drawTextureIdx);
            clearOrNot = true;
        }
        if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
        {
            if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true)
            {
                int drawTextureIdx = gRetracer.curContextState->curFrameBufferState.getDrawTextureIdx();
                gRetracer.curPreExecuteState.newInsertCallIntoList(clearOrNot, drawTextureIdx);
                gRetracer.curPreExecuteState.newInsertOneCallNo(clearOrNot, drawTextureIdx, gRetracer.mCurCallNo);
            }
        }
    }//else if
}

static void retrace_glClearBufferiv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int buffer; // enum
    _src = ReadFixed(_src, buffer);
    int drawbuffer; // literal
    _src = ReadFixed<int>(_src, drawbuffer);
    Array<int> value; // array
    _src = Read1DArray(_src, value);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    gRetracer.IncCurDrawId();
    // ------------- retrace ------------------
    //  glClearBufferiv(buffer, drawbuffer, value);
    // ---------- register handles ------------
}

static void retrace_glClearBufferuiv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int buffer; // enum
    _src = ReadFixed(_src, buffer);
    int drawbuffer; // literal
    _src = ReadFixed<int>(_src, drawbuffer);
    Array<unsigned int> value; // array
    _src = Read1DArray(_src, value);

    /*    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    gRetracer.IncCurDrawId();
    // ------------- retrace ------------------
    glClearBufferuiv(buffer, drawbuffer, value);
    // ---------- register handles ------------
    */
}

static void retrace_glClientWaitSync(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    uint64_t sync; // literal
    _src = ReadFixed<uint64_t>(_src, sync);
    unsigned int flags; // literal
    _src = ReadFixed<unsigned int>(_src, flags);
    uint64_t timeout; // literal
    _src = ReadFixed<uint64_t>(_src, timeout);

    int old_ret; // enum
    _src = ReadFixed(_src, old_ret);
    /*    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLsync syncNew = context.getSyncMap().RValue(sync);
    // ----------- out parameters  ------------
    GLenum ret = 0;
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    ret = glClientWaitSync(syncNew, flags, timeout);
    // ---------- register handles ------------
    (void)ret;  // Ignored return value
    */
}

static void retrace_glCompressedTexImage3D(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
#ifdef __APPLE__
    // Since we're only supporting Apple devices with GLES3
    // (which implies ETC2 support), we take advantage of ETC2's
    // backwards compatibility to get ETC1 support as well.
    // (This only makes sense for the internalformat, but is added to all parsed enums.)
    if (internalformat == GL_ETC1_RGB8_OES)
    {
        internalformat = GL_COMPRESSED_RGB8_ETC2;
    }
#endif
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);
    int depth; // literal
    _src = ReadFixed<int>(_src, depth);
    int border; // literal
    _src = ReadFixed<int>(_src, border);
    int imageSize; // literal
    _src = ReadFixed<int>(_src, imageSize);
    //  GLvoid* data = NULL;
    Array<char> dataBlob;
    if (gRetracer.getFileFormatVersion() <= HEADER_VERSION_3)
    {
        _src = Read1DArray(_src, dataBlob);
        //       data = dataBlob.v;
    }
    else
    {
        unsigned int _opaque_type = 0;
        _src = ReadFixed(_src, _opaque_type);
        switch (_opaque_type)
        {
        case BlobType:
            _src = Read1DArray(_src, dataBlob);
            //          data = dataBlob.v;
            break;
        case BufferObjectReferenceType:
            unsigned int bufferOffset = 0;
            _src = ReadFixed<unsigned int>(_src, bufferOffset);
            //         data = (GLvoid *)static_cast<uintptr_t>(bufferOffset);
            break;
        }
    }

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    //#ifndef NDEBUG
    //    gRetracer.mCompressedTextureDataSize += imageSize;
    //#endif
    // ------------- retrace ------------------
    //    glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data);
    // ---------- register handles ------------
}

static void retrace_glCompressedTexSubImage3D(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int xoffset; // literal
    _src = ReadFixed<int>(_src, xoffset);
    int yoffset; // literal
    _src = ReadFixed<int>(_src, yoffset);
    int zoffset; // literal
    _src = ReadFixed<int>(_src, zoffset);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);
    int depth; // literal
    _src = ReadFixed<int>(_src, depth);
    int format; // enum
    _src = ReadFixed(_src, format);
#ifdef __APPLE__
    // Since we're only supporting Apple devices with GLES3
    // (which implies ETC2 support), we take advantage of ETC2's
    // backwards compatibility to get ETC1 support as well.
    // (This only makes sense for the internalformat, but is added to all parsed enums.)
    if (format == GL_ETC1_RGB8_OES)
    {
        format = GL_COMPRESSED_RGB8_ETC2;
    }
#endif
    int imageSize; // literal
    _src = ReadFixed<int>(_src, imageSize);
    //   GLvoid* data = NULL;
    Array<char> dataBlob;
    if (gRetracer.getFileFormatVersion() <= HEADER_VERSION_3)
    {
        _src = Read1DArray(_src, dataBlob);
        //      data = dataBlob.v;
    }
    else
    {
        unsigned int _opaque_type = 0;
        _src = ReadFixed(_src, _opaque_type);
        switch (_opaque_type)
        {
        case BlobType:
            _src = Read1DArray(_src, dataBlob);
            //        data = dataBlob.v;
            break;
        case BufferObjectReferenceType:
            unsigned int bufferOffset = 0;
            _src = ReadFixed<unsigned int>(_src, bufferOffset);
            //        data = (GLvoid *)static_cast<uintptr_t>(bufferOffset);
            break;
        }
    }

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    //#ifndef NDEBUG
    //    gRetracer.mCompressedTextureDataSize += imageSize;
    //#endif
    // ------------- retrace ------------------
    //   glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
    // ---------- register handles ------------

    std::unordered_set<unsigned int> newList;
    unsigned int GL_PIXEL_UNPACK_BUFFERNo = gRetracer.curContextState->curBufferState.getCurGL_PIXEL_UNPACK_BUFFERNo();
    unsigned int GL_PIXEL_UNPACK_BUFFERIdx = gRetracer.curContextState->curBufferState.getCurGL_PIXEL_UNPACK_BUFFERIdx();
    if(GL_PIXEL_UNPACK_BUFFERIdx != 0)
    {
        gRetracer.curContextState->curBufferState.findABufferState(GL_PIXEL_UNPACK_BUFFERIdx, newList);
    }
    gRetracer.curContextState->curTextureState.ff_glTypes_TexSubImage2D(target, GL_PIXEL_UNPACK_BUFFERNo, gRetracer.mCurCallNo, newList);
}

static void retrace_glCopyBufferSubData(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int readTarget; // enum
    _src = ReadFixed(_src, readTarget);
    int writeTarget; // enum
    _src = ReadFixed(_src, writeTarget);
    int readOffset; // literal
    _src = ReadFixed<int>(_src, readOffset);
    int writeOffset; // literal
    _src = ReadFixed<int>(_src, writeOffset);
    int size; // literal
    _src = ReadFixed<int>(_src, size);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);
    // ---------- register handles ------------
}

static void retrace_glCopyTexSubImage3D(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int xoffset; // literal
    _src = ReadFixed<int>(_src, xoffset);
    int yoffset; // literal
    _src = ReadFixed<int>(_src, yoffset);
    int zoffset; // literal
    _src = ReadFixed<int>(_src, zoffset);
    int x; // literal
    _src = ReadFixed<int>(_src, x);
    int y; // literal
    _src = ReadFixed<int>(_src, y);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
    // ---------- register handles ------------
}

static void retrace_glCreateShaderProgramv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int type; // enum
    _src = ReadFixed(_src, type);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<const char*> strings; // string array
    _src = ReadStringArray(_src, strings);

    unsigned int old_ret; // literal
    _src = ReadFixed<unsigned int>(_src, old_ret);
    // ----------- look up handles ------------
    /*    // ----------- out parameters  ------------
    GLuint ret = 0;
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    ret = glCreateShaderProgramv(type, count, strings);
    // ---------- register handles ------------
    Context& context = gRetracer.getCurrentContext();
    context.getProgramMap().LValue(old_ret) = ret;
    context.getProgramRevMap().LValue(ret) = old_ret;
    */
}

static void retrace_glDeleteProgramPipelines(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> pipelines; // array
    _src = Read1DArray(_src, pipelines);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glDeleteProgramPipelines(n, pipelines);
    // ---------- register handles ------------
}

static void retrace_glDeleteQueries(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> ids; // array
    _src = Read1DArray(_src, ids);

    // ----------- look up handles ------------
    // hardcode in retrace!
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   hardcode_glDeleteQueries(n, ids);
    // ---------- register handles ------------
}

static void retrace_glDeleteSamplers(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<unsigned int> samplers; // array
    _src = Read1DArray(_src, samplers);

    // ----------- look up handles ------------
    // hardcode in retrace!
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   hardcode_glDeleteSamplers(count, samplers);
    // ---------- register handles ------------
}

static void retrace_glDeleteSync(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    uint64_t sync; // literal
    _src = ReadFixed<uint64_t>(_src, sync);

    /*    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLsync syncNew = context.getSyncMap().RValue(sync);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glDeleteSync(syncNew);
    // ---------- register handles ------------
    */
}

static void retrace_glDeleteTransformFeedbacks(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> ids; // array
    _src = Read1DArray(_src, ids);

    // ----------- look up handles ------------
    // hardcode in retrace!
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   hardcode_glDeleteTransformFeedbacks(n, ids);
    // ---------- register handles ------------
}

static void retrace_glDeleteVertexArrays(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> arrays; // array
    _src = Read1DArray(_src, arrays);

    // ----------- look up handles ------------
    // hardcode in retrace!
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   hardcode_glDeleteVertexArrays(n, arrays);
    // ---------- register handles ------------
}

static void retrace_glDispatchCompute(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int num_groups_x; // literal
    _src = ReadFixed<unsigned int>(_src, num_groups_x);
    unsigned int num_groups_y; // literal
    _src = ReadFixed<unsigned int>(_src, num_groups_y);
    unsigned int num_groups_z; // literal
    _src = ReadFixed<unsigned int>(_src, num_groups_z);

    /*    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    gRetracer.IncCurDrawId();
    if (unlikely(stateLoggingEnabled)) {
        gRetracer.getStateLogger().logFunction(gRetracer.getCurTid(), "glDispatchCompute", gRetracer.GetCurCallId(), gRetracer.GetCurDrawId());
        gRetracer.getStateLogger().logCompute(gRetracer.getCurTid(), num_groups_x, num_groups_y, num_groups_z);
    }
    // ------------- retrace ------------------
    glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
    // ---------- register handles ------------
    */

    if((gRetracer.preExecute == true && gRetracer.mCurCallNo > gRetracer.curPreExecuteState.endDrawCallNo) && gRetracer.mOptions.allDraws == false)
    {
        gRetracer.curPreExecuteState.newFinalPotr++;
        gRetracer.curPreExecuteState.beginDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)/FIRST_10_BIT;
        gRetracer.curPreExecuteState.endDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)%FIRST_10_BIT;
    }

    std::vector<unsigned int> textureList;

    //debug for fast
    if(gRetracer.preExecute == true && ((gRetracer.mCurCallNo >= gRetracer.curPreExecuteState.beginDrawCallNo &&
 gRetracer.mCurCallNo <= gRetracer.curPreExecuteState.endDrawCallNo) || gRetracer.mOptions.allDraws == true))//*(gRetracer.curPreExecuteState.finalPotr) == gRetracer.mCurCallNo)
    {
        if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
        {
            gRetracer.curContextState->readStateForDispatchCompute(gRetracer.callNoList, gRetracer.callNoListJudge);
            gRetracer.callNoList.insert(gRetracer.mCurCallNo);
            gRetracer.curContextState->curGlobalState.computeMemoryBarrier = true;
        }//for debug
    }//if(gRetracer.preExecute == true)
    else if(gRetracer.preExecute == false)
    {
        gRetracer.curPreExecuteState.drawCallNo++;
        if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
        {
    //        for(unsigned int i=0; i<5; i++)
      //      {
                int drawTextureIdx = gRetracer.curContextState->curTextureState.readCurTextureNIdx(191);//save for dispatchcompute
              //  if(drawTextureIdx != 0)
            //    {
                    gRetracer.curContextState->curTextureState.readCurTextureIdx(textureList);
                    gRetracer.curPreExecuteState.newInsertCallIntoList(false, drawTextureIdx);
                    gRetracer.curPreExecuteState.newInsertTextureListNo(textureList, gRetracer.curPreExecuteState.newQueryTextureNoList(drawTextureIdx));
                    gRetracer.curPreExecuteState.newInsertOneCallNo(false, drawTextureIdx, gRetracer.mCurCallNo);
          //      }
        //    }
        }//for debug
    }//if(gRetracer.preExecute == true)
}

static void retrace_glDispatchComputeIndirect(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int indirect; // literal
    _src = ReadFixed<int>(_src, indirect);

    // ----------- look up handles ------------
    /*   // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    gRetracer.IncCurDrawId();
    if (unlikely(gRetracer.mOptions.mDumpStatic))
    {
        static int idcount = 0;
        GLint bufferId = 0;
        const GLuint *ptr;
        const GLsizeiptr size = sizeof(IndirectCompute);
        const GLenum target = GL_DISPATCH_INDIRECT_BUFFER;
        _glGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, &bufferId);
        if (bufferId == 0)
        {
            ptr = reinterpret_cast<GLuint*>(indirect);
        }
        else
        {
            _glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
            const GLintptr offset = (GLintptr)indirect; // no c++ cast will accept this...
            ptr = (const GLuint *)_glMapBufferRange(target, offset, size, GL_MAP_READ_BIT);
        }
        Json::Value entry;
        entry["index"] = idcount;
        entry["function"] = "glDispatchComputeIndirect";
        entry["num_groups_x"] = ptr[0];
        entry["num_groups_y"] = ptr[1];
        entry["num_groups_z"] = ptr[2];
        gRetracer.staticDump["indirect"].append(entry);
        glUnmapBuffer(target);
        idcount++;
    }
    if (unlikely(stateLoggingEnabled)) {
        gRetracer.getStateLogger().logFunction(gRetracer.getCurTid(), "glDispatchComputeIndirect", gRetracer.GetCurCallId(), gRetracer.GetCurDrawId());
        gRetracer.getStateLogger().logComputeIndirect(gRetracer.getCurTid(), indirect);
    }
    // ------------- retrace ------------------
    glDispatchComputeIndirect(indirect);
    // ---------- register handles ------------
    */
}

static void retrace_glDrawArraysIndirect(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int mode; // enum
    _src = ReadFixed(_src, mode);
    unsigned int csbName = 0; // ClientSideBuffer name
    //  GLvoid* indirect = NULL;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    if (_opaque_type == BufferObjectReferenceType)
    { // simple memory offset
        unsigned int indirect_raw; // raw ptr
        _src = ReadFixed<unsigned int>(_src, indirect_raw);
        //   indirect = (GLvoid *)static_cast<uintptr_t>(indirect_raw);
    }
    else if (_opaque_type == BlobType)
    { // blob type
        Array<float> indirect_blob;
        _src = Read1DArray(_src, indirect_blob);
        //    indirect = indirect_blob.v;
    }
    else if (_opaque_type == ClientSideBufferObjectReferenceType)
    { // a memory reference to client-side buffer
        unsigned int offset = 0;
        _src = ReadFixed<unsigned int>(_src, csbName);
        _src = ReadFixed<unsigned int>(_src, offset);
        //     indirect = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);
    }
    else if (_opaque_type == NoopType)
    { // Do nothing
    }

    /*   // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    gRetracer.IncCurDrawId();
    if (unlikely(gRetracer.mOptions.mDumpStatic))
    {
        static int idcount = 0;
        GLint bufferId = 0;
        const GLuint *ptr;
        const GLsizeiptr size = sizeof(IndirectDrawArrays);
        const GLenum target = GL_DRAW_INDIRECT_BUFFER;
        _glGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, &bufferId);
        if (bufferId == 0)
        {
            ptr = reinterpret_cast<GLuint*>(indirect);
        }
        else
        {
            _glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
            const GLintptr offset = (GLintptr)indirect; // no c++ cast will accept this...
            ptr = (const GLuint *)_glMapBufferRange(target, offset, size, GL_MAP_READ_BIT);
        }
        Json::Value entry;
        entry["index"] = idcount;
        entry["function"] = "glDrawArraysIndirect";
        entry["count"] = ptr[0];
        entry["primCount"] = ptr[1];
        entry["first"] = ptr[2];
        gRetracer.staticDump["indirect"].append(entry);
        glUnmapBuffer(target);
        idcount++;
    }
    if (unlikely(stateLoggingEnabled)) {
        gRetracer.getStateLogger().logFunction(gRetracer.getCurTid(), "glDrawArraysIndirect", gRetracer.GetCurCallId(), gRetracer.GetCurDrawId());
        gRetracer.getStateLogger().logDrawArraysIndirect(gRetracer.getCurTid(), indirect);
    }
    //   pre_glDraw();
    // ------------- retrace ------------------
    glDrawArraysIndirect(mode, indirect);
    // ---------- register handles ------------
    */


    if((gRetracer.preExecute == true &&gRetracer.mCurCallNo > gRetracer.curPreExecuteState.endDrawCallNo) && gRetracer.mOptions.allDraws == false)
    {
        gRetracer.curPreExecuteState.newFinalPotr++;
        gRetracer.curPreExecuteState.beginDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)/FIRST_10_BIT;
        gRetracer.curPreExecuteState.endDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)%FIRST_10_BIT;
    }
    std::vector<unsigned int> textureList;

    //debug for fast
    if(gRetracer.preExecute == true && ((gRetracer.mCurCallNo >= gRetracer.curPreExecuteState.beginDrawCallNo &&
 gRetracer.mCurCallNo <= gRetracer.curPreExecuteState.endDrawCallNo) || gRetracer.curContextState->curGlobalState.TransformFeedbackSwitch == true||gRetracer.mOptions.allDraws==true))//*(gRetracer.curPreExecuteState.finalPotr) == gRetracer.mCurCallNo)
    {
        if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
        {
            if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true)
            {
                gRetracer.curContextState->readAllState(0, 0, NO_IDX, gRetracer.callNoList, gRetracer.callNoListJudge);
                gRetracer.callNoList.insert(gRetracer.mCurCallNo);
                gRetracer.curFrameStoreState.setEndDraw(true);
            }
        }//for debug
        textureList.clear();
        if(((gRetracer.mCurFrameNo >= gRetracer.mOptions.fastforwadFrame0 && gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1) &&
 gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == false) || gRetracer.curContextState->curGlobalState.TransformFeedbackSwitch == true)
        {
            gRetracer.curContextState->readAllState(0, 0, NO_IDX, gRetracer.callNoList, gRetracer.callNoListJudge);
            gRetracer.callNoList.insert(gRetracer.mCurCallNo);
        }
    }//if(gRetracer.preExecute == true)
    else if(gRetracer.preExecute == false)
    {
        gRetracer.curPreExecuteState.drawCallNo++;
        if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
        {
            if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true)
            {
                int drawTextureIdx = gRetracer.curContextState->curFrameBufferState.getDrawTextureIdx();
                gRetracer.curContextState->curTextureState.readCurTextureIdx(textureList);
                gRetracer.curPreExecuteState.newInsertCallIntoList(false, drawTextureIdx);
                gRetracer.curPreExecuteState.newInsertTextureListNo(textureList, gRetracer.curPreExecuteState.newQueryTextureNoList(drawTextureIdx));
                gRetracer.curPreExecuteState.newInsertOneCallNo(false, drawTextureIdx, gRetracer.mCurCallNo);
            }
        }//for debug
        textureList.clear();
        if((gRetracer.mCurFrameNo >= gRetracer.mOptions.fastforwadFrame0 && gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1) && gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == false)
        {
            gRetracer.curContextState->curTextureState.readCurTextureIdx(textureList);
            gRetracer.curPreExecuteState.newInsertCallIntoList(false, 0);
            gRetracer.curPreExecuteState.newInsertTextureListNo(textureList, gRetracer.curPreExecuteState.newQueryTextureNoList(0));
            gRetracer.curPreExecuteState.newInsertOneCallNo(false, 0, gRetracer.mCurCallNo);
        }
    }//if(gRetracer.preExecute == true)

    if(gRetracer.curPreExecuteState.endPreExecute == false)
    {
        if(gRetracer.mCurFrameNo > gRetracer.mOptions.fastforwadFrame1)
        {
            gRetracer.curPreExecuteState.newInsertCallIntoList(false, NO_IDX);
            gRetracer.curPreExecuteState.endPreExecute=true;
        }
    }
}

static void retrace_glDrawArraysInstanced(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int mode; // enum
    _src = ReadFixed(_src, mode);
    int first; // literal
    _src = ReadFixed<int>(_src, first);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    int instancecount; // literal
    _src = ReadFixed<int>(_src, instancecount);
    /*
    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    gRetracer.IncCurDrawId();
    if (unlikely(stateLoggingEnabled)) {
        gRetracer.getStateLogger().logFunction(gRetracer.getCurTid(), "glDrawArraysInstanced", gRetracer.GetCurCallId(), gRetracer.GetCurDrawId());
        gRetracer.getStateLogger().logState(gRetracer.getCurTid(), first, count, instancecount);
    }
    //   pre_glDraw();
    // ------------- retrace ------------------
    glDrawArraysInstanced(mode, first, count, instancecount);
    // ---------- register handles ------------
    */


    if((gRetracer.preExecute == true && gRetracer.mCurCallNo > gRetracer.curPreExecuteState.endDrawCallNo) && gRetracer.mOptions.allDraws == false)
    {
        gRetracer.curPreExecuteState.newFinalPotr++;
        gRetracer.curPreExecuteState.beginDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)/FIRST_10_BIT;
        gRetracer.curPreExecuteState.endDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)%FIRST_10_BIT;
    }

    std::vector<unsigned int> textureList;

    //debug for fast
    if(gRetracer.preExecute == true && ((gRetracer.mCurCallNo >= gRetracer.curPreExecuteState.beginDrawCallNo && gRetracer.mCurCallNo <= gRetracer.curPreExecuteState.endDrawCallNo) ||
 gRetracer.curContextState->curGlobalState.TransformFeedbackSwitch == true || gRetracer.mOptions.allDraws == true))//*(gRetracer.curPreExecuteState.finalPotr) == gRetracer.mCurCallNo)
    {
        if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
        {
            if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true)
            {
                gRetracer.curContextState->readAllState(0, 0, NO_IDX, gRetracer.callNoList, gRetracer.callNoListJudge);
                gRetracer.callNoList.insert(gRetracer.mCurCallNo);
                gRetracer.curFrameStoreState.setEndDraw(true);
            }
        }//for debug
        textureList.clear();
        if(((gRetracer.mCurFrameNo >= gRetracer.mOptions.fastforwadFrame0 && gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1) &&
 gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer()==false) || gRetracer.curContextState->curGlobalState.TransformFeedbackSwitch == true)
        {
            gRetracer.curContextState->readAllState(0, 0, NO_IDX, gRetracer.callNoList, gRetracer.callNoListJudge);
            gRetracer.callNoList.insert(gRetracer.mCurCallNo);
        }
    }//if(gRetracer.preExecute == true)
    else if(gRetracer.preExecute == false)
    {
        gRetracer.curPreExecuteState.drawCallNo++;
        if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
        {
            if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true)
            {
                int drawTextureIdx = gRetracer.curContextState->curFrameBufferState.getDrawTextureIdx();
                gRetracer.curContextState->curTextureState.readCurTextureIdx(textureList);
                gRetracer.curPreExecuteState.newInsertCallIntoList(false, drawTextureIdx);
                gRetracer.curPreExecuteState.newInsertTextureListNo(textureList, gRetracer.curPreExecuteState.newQueryTextureNoList(drawTextureIdx));
                gRetracer.curPreExecuteState.newInsertOneCallNo(false, drawTextureIdx, gRetracer.mCurCallNo);
            }
        }//for debug
        textureList.clear();
        if((gRetracer.mCurFrameNo >= gRetracer.mOptions.fastforwadFrame0 && gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1) && gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == false)
        {
            gRetracer.curContextState->curTextureState.readCurTextureIdx(textureList);
            gRetracer.curPreExecuteState.newInsertCallIntoList(false, 0);
            gRetracer.curPreExecuteState.newInsertTextureListNo(textureList, gRetracer.curPreExecuteState.newQueryTextureNoList(0));
            gRetracer.curPreExecuteState.newInsertOneCallNo(false, 0, gRetracer.mCurCallNo);
        }
    }//if(gRetracer.preExecute == true)
    if(gRetracer.curPreExecuteState.endPreExecute == false)
    {
        if(gRetracer.mCurFrameNo > gRetracer.mOptions.fastforwadFrame1)
        {
            gRetracer.curPreExecuteState.newInsertCallIntoList(false, NO_IDX);
            gRetracer.curPreExecuteState.endPreExecute=true;
        }
    }
}

static void retrace_glDrawBuffers(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> bufs; // array
    _src = Read1DArray(_src, bufs);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glDrawBuffers(n, bufs);
    // ---------- register handles ------------
    //gRetracer.curContextState->curGlobalState.ff_glDrawBuffers(gRetracer.mCurCallNo);

    gRetracer.curContextState->curFrameBufferState.ff_glDrawBuffers(gRetracer.mCurCallNo);
}

static void retrace_glDrawElementsIndirect(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int mode; // enum
    _src = ReadFixed(_src, mode);
    int type; // enum
    _src = ReadFixed(_src, type);
    unsigned int csbName = 0; // ClientSideBuffer name
    // GLvoid* indirect = NULL;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    if (_opaque_type == BufferObjectReferenceType)
    { // simple memory offset
        unsigned int indirect_raw; // raw ptr
        _src = ReadFixed<unsigned int>(_src, indirect_raw);
        //     indirect = (GLvoid *)static_cast<uintptr_t>(indirect_raw);
    }
    else if (_opaque_type == BlobType)
    { // blob type
        Array<float> indirect_blob;
        _src = Read1DArray(_src, indirect_blob);
        //      indirect = indirect_blob.v;
    }
    else if (_opaque_type == ClientSideBufferObjectReferenceType)
    { // a memory reference to client-side buffer
        unsigned int offset = 0;
        _src = ReadFixed<unsigned int>(_src, csbName);
        _src = ReadFixed<unsigned int>(_src, offset);
   //     indirect = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);
    }
    else if (_opaque_type == NoopType)
    { // Do nothing
    }

    /*    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    gRetracer.IncCurDrawId();
    if (unlikely(gRetracer.mOptions.mDumpStatic))
    {
        static int idcount = 0;
        GLint bufferId = 0;
        const GLuint *ptr;
        const GLsizeiptr size = sizeof(IndirectDrawElements);
        const GLenum target = GL_DRAW_INDIRECT_BUFFER;
        _glGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, &bufferId);
        if (bufferId == 0)
        {
            ptr = reinterpret_cast<GLuint*>(indirect);
        }
        else
        {
            _glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
            const GLintptr offset = (GLintptr)indirect; // no c++ cast will accept this...
            ptr = (const GLuint *)_glMapBufferRange(target, offset, size, GL_MAP_READ_BIT);
        }
        Json::Value entry;
        entry["index"] = idcount;
        entry["function"] = "glDrawElementsIndirect";
        entry["count"] = ptr[0];
        entry["instanceCount"] = ptr[1];
        entry["firstIndex"] = ptr[2];
        entry["baseVertex"] = static_cast<GLint>(ptr[3]);
        gRetracer.staticDump["indirect"].append(entry);
        glUnmapBuffer(target);
        idcount++;
    }
    if (unlikely(stateLoggingEnabled)) {
        gRetracer.getStateLogger().logFunction(gRetracer.getCurTid(), "glDrawElementsIndirect", gRetracer.GetCurCallId(), gRetracer.GetCurDrawId());
        gRetracer.getStateLogger().logDrawElementsIndirect(gRetracer.getCurTid(), type, indirect);
    }
    //  pre_glDraw();
    // ------------- retrace ------------------
    glDrawElementsIndirect(mode, type, indirect);
    // ---------- register handles ------------
    */


    if((gRetracer.preExecute == true && gRetracer.mCurCallNo > gRetracer.curPreExecuteState.endDrawCallNo) && gRetracer.mOptions.allDraws == false)
    {
        gRetracer.curPreExecuteState.newFinalPotr++;
        gRetracer.curPreExecuteState.beginDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)/FIRST_10_BIT;
        gRetracer.curPreExecuteState.endDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)%FIRST_10_BIT;
    }

    std::vector<unsigned int> textureList;

    if(gRetracer.preExecute == true && ((gRetracer.mCurCallNo >= gRetracer.curPreExecuteState.beginDrawCallNo && gRetracer.mCurCallNo <= gRetracer.curPreExecuteState.endDrawCallNo) || gRetracer.mOptions.allDraws == true))
    {
        //debugg:
        if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
        {
            if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true)
            {
                gRetracer.curContextState->readAllState(0, 1, csbName, gRetracer.callNoList, gRetracer.callNoListJudge);
                gRetracer.callNoList.insert(gRetracer.mCurCallNo);
                gRetracer.curFrameStoreState.setEndDraw(true);
            }
        }//for debug
        textureList.clear();
        if((gRetracer.mCurFrameNo >= gRetracer.mOptions.fastforwadFrame0 && gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1) && gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == false)
        {
            gRetracer.curContextState->readAllState(0, 1, csbName, gRetracer.callNoList, gRetracer.callNoListJudge);
            gRetracer.callNoList.insert(gRetracer.mCurCallNo);
        }
    }//if(gRetracer.preExecute == true){
    else if(gRetracer.preExecute == false)
    {
        gRetracer.curPreExecuteState.drawCallNo++;
        if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
        {
            if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true)
            {
                int drawTextureIdx = gRetracer.curContextState->curFrameBufferState.getDrawTextureIdx();
                gRetracer.curContextState->curTextureState.readCurTextureIdx(textureList);
                gRetracer.curPreExecuteState.newInsertCallIntoList(false, drawTextureIdx);
                gRetracer.curPreExecuteState.newInsertTextureListNo(textureList, gRetracer.curPreExecuteState.newQueryTextureNoList(drawTextureIdx));
                gRetracer.curPreExecuteState.newInsertOneCallNo(false, drawTextureIdx, gRetracer.mCurCallNo);
            }
        }//for debug
        textureList.clear();
        if((gRetracer.mCurFrameNo >= gRetracer.mOptions.fastforwadFrame0 && gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1) && gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == false)
        {
            gRetracer.curContextState->curTextureState.readCurTextureIdx(textureList);
            gRetracer.curPreExecuteState.newInsertCallIntoList(false, 0);
            gRetracer.curPreExecuteState.newInsertTextureListNo(textureList, gRetracer.curPreExecuteState.newQueryTextureNoList(0));
            gRetracer.curPreExecuteState.newInsertOneCallNo(false, 0, gRetracer.mCurCallNo);
        }
    }//if(gRetracer.preExecute == true)

    if(gRetracer.curPreExecuteState.endPreExecute == false)
    {
        if(gRetracer.mCurFrameNo > gRetracer.mOptions.fastforwadFrame1)
        {
            gRetracer.curPreExecuteState.newInsertCallIntoList(false, NO_IDX);
            gRetracer.curPreExecuteState.endPreExecute = true;
        }
    }
}

static void retrace_glDrawElementsInstanced(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int mode; // enum
    _src = ReadFixed(_src, mode);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    int type; // enum
    _src = ReadFixed(_src, type);
    unsigned int csbName = 0; // ClientSideBuffer name
    //  GLvoid* indices = NULL;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    if (_opaque_type == BufferObjectReferenceType)
    { // simple memory offset
        unsigned int indices_raw; // raw ptr
        _src = ReadFixed<unsigned int>(_src, indices_raw);
     //   indices = (GLvoid *)static_cast<uintptr_t>(indices_raw);
    }
    else if (_opaque_type == BlobType)
    { // blob type
        Array<float> indices_blob;
        _src = Read1DArray(_src, indices_blob);
    //    indices = indices_blob.v;
    }
    else if (_opaque_type == ClientSideBufferObjectReferenceType)
    { // a memory reference to client-side buffer
        unsigned int offset = 0;
        _src = ReadFixed<unsigned int>(_src, csbName);
        _src = ReadFixed<unsigned int>(_src, offset);
    //    indices = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);
    }
    else if (_opaque_type == NoopType)
    { // Do nothing
    }
    int instancecount; // literal
    _src = ReadFixed<int>(_src, instancecount);

    /*   // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    gRetracer.IncCurDrawId();
    if (unlikely(stateLoggingEnabled)) {
        gRetracer.getStateLogger().logFunction(gRetracer.getCurTid(), "glDrawElementsInstanced", gRetracer.GetCurCallId(), gRetracer.GetCurDrawId());
        gRetracer.getStateLogger().logState(gRetracer.getCurTid(), count, type, indices, instancecount);
    }
    //   pre_glDraw();
    // ------------- retrace ------------------
    glDrawElementsInstanced(mode, count, type, indices, instancecount);
    // ---------- register handles ------------
    */


    if((gRetracer.preExecute == true && gRetracer.mCurCallNo > gRetracer.curPreExecuteState.endDrawCallNo) && gRetracer.mOptions.allDraws == false)
    {
        gRetracer.curPreExecuteState.newFinalPotr++;
        gRetracer.curPreExecuteState.beginDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)/FIRST_10_BIT;
        gRetracer.curPreExecuteState.endDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)%FIRST_10_BIT;
    }

    std::vector<unsigned int> textureList;

    if(gRetracer.preExecute == true && ((gRetracer.mCurCallNo >= gRetracer.curPreExecuteState.beginDrawCallNo && gRetracer.mCurCallNo <= gRetracer.curPreExecuteState.endDrawCallNo) || gRetracer.mOptions.allDraws == true))// *(gRetracer.curPreExecuteState.finalPotr) == gRetracer.mCurCallNo)
    {
        //debugg:
        if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
        {
            if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true)
            {
                gRetracer.curContextState->readAllState(0, 1, csbName, gRetracer.callNoList, gRetracer.callNoListJudge);
                gRetracer.callNoList.insert(gRetracer.mCurCallNo);
                gRetracer.curFrameStoreState.setEndDraw(true);
            }
        }//for debug
        textureList.clear();
        if((gRetracer.mCurFrameNo >= gRetracer.mOptions.fastforwadFrame0 && gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1) && gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == false)
        {
            gRetracer.curContextState->readAllState(0, 1, csbName, gRetracer.callNoList, gRetracer.callNoListJudge);
            gRetracer.callNoList.insert(gRetracer.mCurCallNo);
        }
    }//if(gRetracer.preExecute == true){
    else if(gRetracer.preExecute == false)
    {
        gRetracer.curPreExecuteState.drawCallNo++;
        if(gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1)
        {
            if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == true)
            {
                int drawTextureIdx = gRetracer.curContextState->curFrameBufferState.getDrawTextureIdx();
                gRetracer.curContextState->curTextureState.readCurTextureIdx(textureList);
                gRetracer.curPreExecuteState.newInsertCallIntoList(false, drawTextureIdx);
                gRetracer.curPreExecuteState.newInsertTextureListNo(textureList, gRetracer.curPreExecuteState.newQueryTextureNoList(drawTextureIdx));
                gRetracer.curPreExecuteState.newInsertOneCallNo(false, drawTextureIdx, gRetracer.mCurCallNo);
            }
        }//for debug
        textureList.clear();
        if((gRetracer.mCurFrameNo >= gRetracer.mOptions.fastforwadFrame0 && gRetracer.mCurFrameNo <= gRetracer.mOptions.fastforwadFrame1) && gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer() == false)
        {
            gRetracer.curContextState->curTextureState.readCurTextureIdx(textureList);
            gRetracer.curPreExecuteState.newInsertCallIntoList(false, 0);
            gRetracer.curPreExecuteState.newInsertTextureListNo(textureList, gRetracer.curPreExecuteState.newQueryTextureNoList(0));
            gRetracer.curPreExecuteState.newInsertOneCallNo(false, 0, gRetracer.mCurCallNo);
        }
    }//if(gRetracer.preExecute == true)

    if(gRetracer.curPreExecuteState.endPreExecute == false)
    {
        if(gRetracer.mCurFrameNo > gRetracer.mOptions.fastforwadFrame1)
        {
            gRetracer.curPreExecuteState.newInsertCallIntoList(false, NO_IDX);
            gRetracer.curPreExecuteState.endPreExecute=true;
        }
    }
}

static void retrace_glDrawRangeElements(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int mode; // enum
    _src = ReadFixed(_src, mode);
    unsigned int start; // literal
    _src = ReadFixed<unsigned int>(_src, start);
    unsigned int end; // literal
    _src = ReadFixed<unsigned int>(_src, end);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    int type; // enum
    _src = ReadFixed(_src, type);
    unsigned int csbName = 0; // ClientSideBuffer name
    //  GLvoid* indices = NULL;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    if (_opaque_type == BufferObjectReferenceType)
    { // simple memory offset
        unsigned int indices_raw; // raw ptr
        _src = ReadFixed<unsigned int>(_src, indices_raw);
        //     indices = (GLvoid *)static_cast<uintptr_t>(indices_raw);
    } else if (_opaque_type == BlobType)
    { // blob type
        Array<float> indices_blob;
        _src = Read1DArray(_src, indices_blob);
        //     indices = indices_blob.v;
    }
    else if (_opaque_type == ClientSideBufferObjectReferenceType)
    { // a memory reference to client-side buffer
        unsigned int offset = 0;
        _src = ReadFixed<unsigned int>(_src, csbName);
        _src = ReadFixed<unsigned int>(_src, offset);
        //    indices = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);
    }
    else if (_opaque_type == NoopType)
    { // Do nothing
    }
    /*
    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    gRetracer.IncCurDrawId();
    if (unlikely(stateLoggingEnabled)) {
        gRetracer.getStateLogger().logFunction(gRetracer.getCurTid(), "glDrawRangeElements", gRetracer.GetCurCallId(), gRetracer.GetCurDrawId());
        gRetracer.getStateLogger().logState(gRetracer.getCurTid(), count, type, indices, 0);
    }
    //    pre_glDraw();
    // ------------- retrace ------------------
    glDrawRangeElements(mode, start, end, count, type, indices);
    // ---------- register handles ------------
    */
}

static void retrace_glEndQuery(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glEndQuery(target);
    // ---------- register handles ------------
}

static void retrace_glEndTransformFeedback(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glEndTransformFeedback();
    // ---------- register handles ------------
    gRetracer.callNoList.insert(gRetracer.mCurCallNo);
    gRetracer.curContextState->curGlobalState.TransformFeedbackSwitch = false;
}

static void retrace_glFenceSync(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int condition; // enum
    _src = ReadFixed(_src, condition);
    unsigned int flags; // literal
    _src = ReadFixed<unsigned int>(_src, flags);

    uint64_t old_ret; // literal
    _src = ReadFixed<uint64_t>(_src, old_ret);
    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    /*    GLsync ret = 0;
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    ret = glFenceSync(condition, flags);
    // ---------- register handles ------------
    Context& context = gRetracer.getCurrentContext();
    context.getSyncMap().LValue(old_ret) = ret;
    */
}

static void retrace_glFlushMappedBufferRange(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int offset; // literal
    _src = ReadFixed<int>(_src, offset);
    int length; // literal
    _src = ReadFixed<int>(_src, length);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glFlushMappedBufferRange(target, offset, length);
    // ---------- register handles ------------
    gRetracer.curContextState->curBufferState.ff_glFlushMappedBufferRange(target, gRetracer.mCurCallNo);
}

static void retrace_glFramebufferParameteri(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    int param; // literal
    _src = ReadFixed<int>(_src, param);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glFramebufferParameteri(target, pname, param);
    // ---------- register handles ------------
}

static void retrace_glFramebufferTextureLayer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int attachment; // enum
    _src = ReadFixed(_src, attachment);
    unsigned int texture; // literal
    _src = ReadFixed<unsigned int>(_src, texture);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int layer; // literal
    _src = ReadFixed<int>(_src, layer);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint textureNew = context.getTextureMap().RValue(texture);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glFramebufferTextureLayer(target, attachment, textureNew, level, layer);
    // ---------- register handles ------------
    */
    //if(attachment == GL_COLOR_ATTACHMENT0)
    //if(layer == 0)gRetracer.curContextState->curFrameBufferState.ff_glFramebufferTexture2D(target,attachment,GL_TEXTURE_CUBE_MAP_NEGATIVE_X,texture,gRetracer.mCurCallNo);
    //else if(layer == 1)gRetracer.curContextState->curFrameBufferState.ff_glFramebufferTexture2D(target,attachment,GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,texture,gRetracer.mCurCallNo);
    gRetracer.curContextState->curFrameBufferState.ff_glFramebufferTexture2D(target, attachment, GL_TEXTURE_2D, texture, gRetracer.mCurCallNo);
}

static void retrace_glGenProgramPipelines(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> old_pipelines; // array
    _src = Read1DArray(_src, old_pipelines);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    // ----------- out parameters  ------------
    std::vector<GLuint> _pipelines(n);
    GLuint *pipelines = _pipelines.data();
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glGenProgramPipelines(n, pipelines);
    // ---------- register handles ------------
    for (unsigned int i = 0; i < old_pipelines.cnt; ++i)
    {
        context._pipeline_map.LValue(old_pipelines[i]) = pipelines[i];
        context._pipeline_rev_map.LValue(pipelines[i]) = old_pipelines[i];
    }
    */
}

static void retrace_glGenQueries(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> old_ids; // array
    _src = Read1DArray(_src, old_ids);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    // ----------- out parameters  ------------
    std::vector<GLuint> _ids(n);
    GLuint *ids = _ids.data();
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glGenQueries(n, ids);
    // ---------- register handles ------------
    for (unsigned int i = 0; i < old_ids.cnt; ++i)
    {
        context._query_map.LValue(old_ids[i]) = ids[i];
    }
    */
}

static void retrace_glGenSamplers(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<unsigned int> old_samplers; // array
    _src = Read1DArray(_src, old_samplers);

    /*    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    // ----------- out parameters  ------------
    std::vector<GLuint> _samplers(count);
    GLuint *samplers = _samplers.data();
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glGenSamplers(count, samplers);
    // ---------- register handles ------------
    for (unsigned int i = 0; i < old_samplers.cnt; ++i)
    {
        context.getSamplerMap().LValue(old_samplers[i]) = samplers[i];
    }
    */
    gRetracer.curContextState->curTextureState.ff_glGenSamplers(count, old_samplers, gRetracer.mCurCallNo);
}

static void retrace_glGenTransformFeedbacks(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> old_ids; // array
    _src = Read1DArray(_src, old_ids);

    /*    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    // ----------- out parameters  ------------
    std::vector<GLuint> _ids(n);
    GLuint *ids = _ids.data();
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glGenTransformFeedbacks(n, ids);
    // ---------- register handles ------------
    for (unsigned int i = 0; i < old_ids.cnt; ++i)
    {
        context._feedback_map.LValue(old_ids[i]) = ids[i];
    }
    */
}

static void retrace_glGenVertexArrays(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int n; // literal
    _src = ReadFixed<int>(_src, n);
    Array<unsigned int> old_arrays; // array
    _src = Read1DArray(_src, old_arrays);

    /*    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    // ----------- out parameters  ------------
    std::vector<GLuint> _arrays(n);
    GLuint *arrays = _arrays.data();
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glGenVertexArrays(n, arrays);
    // ---------- register handles ------------
    for (unsigned int i = 0; i < old_arrays.cnt; ++i)
    {
        context._array_map.LValue(old_arrays[i]) = arrays[i];
    }
    */
    gRetracer.curContextState->curBufferState.ff_glGenVertexArrays(n, old_arrays, gRetracer.mCurCallNo);
}

static void retrace_glGetUniformBlockIndex(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    char* uniformBlockName; // string
    _src = ReadString(_src, uniformBlockName);

    unsigned int old_ret; // literal
    _src = ReadFixed<unsigned int>(_src, old_ret);
    /*    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    // ----------- out parameters  ------------
    GLuint ret = 0;
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    ret = glGetUniformBlockIndex(programNew, uniformBlockName);
    // ---------- register handles ------------
    context._uniformBlockIndex_map[programNew].LValue(old_ret) = ret;
    */
    //glGetUniformBlockIndex into glGetAttribLocation
    //    gRetracer.callNoList.insert(gRetracer.mCurCallNo);
    gRetracer.curContextState->curProgramState.ff_glGetAttribLocation(program, uniformBlockName, gRetracer.mCurCallNo);
}

static void retrace_glInvalidateFramebuffer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int numAttachments; // literal
    _src = ReadFixed<int>(_src, numAttachments);
    Array<unsigned int> attachments; // array
    _src = Read1DArray(_src, attachments);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    // RetraceOptions& opt = gRetracer.mOptions;
    /*   if(opt.mForceOffscreen)
    {
        for (int i = 0; i < numAttachments; i++)
        {
            unsigned int attachment = attachments[i];
            if (attachment == GL_COLOR)
                attachments[i] = GL_COLOR_ATTACHMENT0;
            else if (attachment == GL_DEPTH)
                attachments[i] = GL_DEPTH_ATTACHMENT;
            else if (attachment == GL_STENCIL)
                attachments[i] = GL_STENCIL_ATTACHMENT;
        }
    }*/
    //  glInvalidateFramebuffer(target, numAttachments, attachments);
    // ---------- register handles ------------
    /*
    if(gRetracer.curContextState->curFrameBufferState.isBindFrameBuffer()==true )
    {//printf("!!!!!!!!!!!!\n");
    //        std::vector<unsigned int> textureList;
        int drawTextureIdx = gRetracer.curContextState->curFrameBufferState.getDrawTextureIdx();

        gRetracer.curFrameStoreState.clearTextureNoList(drawTextureIdx);
    }
    */
    gRetracer.curContextState->curGlobalState.ff_glInvalidateFramebuffer(target, numAttachments, attachments, gRetracer.mCurCallNo);
}

static void retrace_glInvalidateSubFramebuffer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int numAttachments; // literal
    _src = ReadFixed<int>(_src, numAttachments);
    Array<unsigned int> attachments; // array
    _src = Read1DArray(_src, attachments);
    int x; // literal
    _src = ReadFixed<int>(_src, x);
    int y; // literal
    _src = ReadFixed<int>(_src, y);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glInvalidateSubFramebuffer(target, numAttachments, attachments, x, y, width, height);
    // ---------- register handles ------------
}

static void retrace_glMapBufferRange(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int offset; // literal
    _src = ReadFixed<int>(_src, offset);
    int length; // literal
    _src = ReadFixed<int>(_src, length);
    unsigned int access; // literal
    _src = ReadFixed<unsigned int>(_src, access);

    unsigned int csbName = 0; // ClientSideBuffer name
    //   GLvoid* old_ret = NULL;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    if (_opaque_type == BufferObjectReferenceType)
    { // simple memory offset
        unsigned int old_ret_raw; // raw ptr
        _src = ReadFixed<unsigned int>(_src, old_ret_raw);
        //     old_ret = (GLvoid *)static_cast<uintptr_t>(old_ret_raw);
    }
    else if (_opaque_type == BlobType)
    { // blob type
        Array<float> old_ret_blob;
        _src = Read1DArray(_src, old_ret_blob);
   //     old_ret = old_ret_blob.v;
    }
    else if (_opaque_type == ClientSideBufferObjectReferenceType)
    { // a memory reference to client-side buffer
        unsigned int offset = 0;
        _src = ReadFixed<unsigned int>(_src, csbName);
        _src = ReadFixed<unsigned int>(_src, offset);
   //     old_ret = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);
    }
    else if (_opaque_type == NoopType)
    { // Do nothing
    }
    // ----------- look up handles ------------
    /*    // ----------- out parameters  ------------
    GLvoid * ret = 0;
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    ret = glMapBufferRange(target, offset, length, access);
    // ---------- register handles ------------
    (void)old_ret;  // Unused variable

    Context& context = gRetracer.getCurrentContext();
    GLuint buffer = getBoundBuffer(target);

    if (access & GL_MAP_WRITE_BIT)
    {
        context._bufferToData_map[buffer] = ret;
    }
    else
    {
        context._bufferToData_map[buffer] = 0;
    }
    */

    //for fastforward
    gRetracer.curContextState->curBufferState.ff_glMapBufferRange(target, offset, length, gRetracer.mCurCallNo);
}

static void retrace_glMemoryBarrier(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int barriers; // literal
    _src = ReadFixed<unsigned int>(_src, barriers);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
  //  glMemoryBarrier(barriers);
    // ---------- register handles ------------

    gRetracer.callNoList.insert(gRetracer.mCurCallNo);
    if(gRetracer.curContextState->curGlobalState.computeMemoryBarrier == true)
    {
        gRetracer.callNoList.insert(gRetracer.mCurCallNo);
        gRetracer.curContextState->curGlobalState.computeMemoryBarrier = false;
    }
}

static void retrace_glMemoryBarrierByRegion(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int barriers; // literal
    _src = ReadFixed<unsigned int>(_src, barriers);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glMemoryBarrierByRegion(barriers);
    // ---------- register handles ------------
}

static void retrace_glPauseTransformFeedback(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    // glPauseTransformFeedback();
    // ---------- register handles ------------
}

static void retrace_glProgramBinary(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int binaryFormat; // enum
    _src = ReadFixed(_src, binaryFormat);
    Array<char> binary; // blob
    _src = Read1DArray(_src, binary);
    int length; // literal
    _src = ReadFixed<int>(_src, length);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramBinary(programNew, binaryFormat, binary, length);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramParameteri(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    int value; // literal
    _src = ReadFixed<int>(_src, value);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramParameteri(programNew, pname, value);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform1f(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    float v0; // literal
    _src = ReadFixed<float>(_src, v0);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform1f(programNew, locationNew, v0);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform1fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<float> value; // array
    _src = Read1DArray(_src, value);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform1fv(programNew, locationNew, count, value);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform1i(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int v0; // literal
    _src = ReadFixed<int>(_src, v0);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform1i(programNew, locationNew, v0);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform1iv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<int> value; // array
    _src = Read1DArray(_src, value);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform1iv(programNew, locationNew, count, value);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform1ui(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    unsigned int v0; // literal
    _src = ReadFixed<unsigned int>(_src, v0);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform1ui(programNew, locationNew, v0);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform1uiv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<unsigned int> value; // array
    _src = Read1DArray(_src, value);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform1uiv(programNew, locationNew, count, value);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform2f(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    float v0; // literal
    _src = ReadFixed<float>(_src, v0);
    float v1; // literal
    _src = ReadFixed<float>(_src, v1);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform2f(programNew, locationNew, v0, v1);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform2fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<float> value; // array
    _src = Read1DArray(_src, value);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform2fv(programNew, locationNew, count, value);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform2i(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int v0; // literal
    _src = ReadFixed<int>(_src, v0);
    int v1; // literal
    _src = ReadFixed<int>(_src, v1);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform2i(programNew, locationNew, v0, v1);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform2iv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<int> value; // array
    _src = Read1DArray(_src, value);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform2iv(programNew, locationNew, count, value);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform2ui(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    unsigned int v0; // literal
    _src = ReadFixed<unsigned int>(_src, v0);
    unsigned int v1; // literal
    _src = ReadFixed<unsigned int>(_src, v1);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform2ui(programNew, locationNew, v0, v1);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform2uiv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<unsigned int> value; // array
    _src = Read1DArray(_src, value);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform2uiv(programNew, locationNew, count, value);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform3f(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    float v0; // literal
    _src = ReadFixed<float>(_src, v0);
    float v1; // literal
    _src = ReadFixed<float>(_src, v1);
    float v2; // literal
    _src = ReadFixed<float>(_src, v2);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform3f(programNew, locationNew, v0, v1, v2);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform3fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<float> value; // array
    _src = Read1DArray(_src, value);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform3fv(programNew, locationNew, count, value);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform3i(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int v0; // literal
    _src = ReadFixed<int>(_src, v0);
    int v1; // literal
    _src = ReadFixed<int>(_src, v1);
    int v2; // literal
    _src = ReadFixed<int>(_src, v2);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform3i(programNew, locationNew, v0, v1, v2);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform3iv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<int> value; // array
    _src = Read1DArray(_src, value);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform3iv(programNew, locationNew, count, value);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform3ui(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    unsigned int v0; // literal
    _src = ReadFixed<unsigned int>(_src, v0);
    unsigned int v1; // literal
    _src = ReadFixed<unsigned int>(_src, v1);
    unsigned int v2; // literal
    _src = ReadFixed<unsigned int>(_src, v2);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform3ui(programNew, locationNew, v0, v1, v2);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform3uiv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<unsigned int> value; // array
    _src = Read1DArray(_src, value);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform3uiv(programNew, locationNew, count, value);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform4f(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    float v0; // literal
    _src = ReadFixed<float>(_src, v0);
    float v1; // literal
    _src = ReadFixed<float>(_src, v1);
    float v2; // literal
    _src = ReadFixed<float>(_src, v2);
    float v3; // literal
    _src = ReadFixed<float>(_src, v3);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform4f(programNew, locationNew, v0, v1, v2, v3);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform4fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<float> value; // array
    _src = Read1DArray(_src, value);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform4fv(programNew, locationNew, count, value);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform4i(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int v0; // literal
    _src = ReadFixed<int>(_src, v0);
    int v1; // literal
    _src = ReadFixed<int>(_src, v1);
    int v2; // literal
    _src = ReadFixed<int>(_src, v2);
    int v3; // literal
    _src = ReadFixed<int>(_src, v3);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform4i(programNew, locationNew, v0, v1, v2, v3);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform4iv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<int> value; // array
    _src = Read1DArray(_src, value);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform4iv(programNew, locationNew, count, value);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform4ui(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    unsigned int v0; // literal
    _src = ReadFixed<unsigned int>(_src, v0);
    unsigned int v1; // literal
    _src = ReadFixed<unsigned int>(_src, v1);
    unsigned int v2; // literal
    _src = ReadFixed<unsigned int>(_src, v2);
    unsigned int v3; // literal
    _src = ReadFixed<unsigned int>(_src, v3);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform4ui(programNew, locationNew, v0, v1, v2, v3);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniform4uiv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<unsigned int> value; // array
    _src = Read1DArray(_src, value);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniform4uiv(programNew, locationNew, count, value);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniformMatrix2fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    unsigned char transpose; // literal
    _src = ReadFixed<unsigned char>(_src, transpose);
    Array<float> value; // array
    _src = Read1DArray(_src, value);

    /*    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniformMatrix2fv(programNew, locationNew, count, transpose, value);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniformMatrix2x3fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    unsigned char transpose; // literal
    _src = ReadFixed<unsigned char>(_src, transpose);
    Array<float> value; // array
    _src = Read1DArray(_src, value);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniformMatrix2x3fv(programNew, locationNew, count, transpose, value);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniformMatrix2x4fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    unsigned char transpose; // literal
    _src = ReadFixed<unsigned char>(_src, transpose);
    Array<float> value; // array
    _src = Read1DArray(_src, value);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniformMatrix2x4fv(programNew, locationNew, count, transpose, value);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniformMatrix3fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    unsigned char transpose; // literal
    _src = ReadFixed<unsigned char>(_src, transpose);
    Array<float> value; // array
    _src = Read1DArray(_src, value);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniformMatrix3fv(programNew, locationNew, count, transpose, value);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniformMatrix3x2fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    unsigned char transpose; // literal
    _src = ReadFixed<unsigned char>(_src, transpose);
    Array<float> value; // array
    _src = Read1DArray(_src, value);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniformMatrix3x2fv(programNew, locationNew, count, transpose, value);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniformMatrix3x4fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    unsigned char transpose; // literal
    _src = ReadFixed<unsigned char>(_src, transpose);
    Array<float> value; // array
    _src = Read1DArray(_src, value);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniformMatrix3x4fv(programNew, locationNew, count, transpose, value);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniformMatrix4fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    unsigned char transpose; // literal
    _src = ReadFixed<unsigned char>(_src, transpose);
    Array<float> value; // array
    _src = Read1DArray(_src, value);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniformMatrix4fv(programNew, locationNew, count, transpose, value);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniformMatrix4x2fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    unsigned char transpose; // literal
    _src = ReadFixed<unsigned char>(_src, transpose);
    Array<float> value; // array
    _src = Read1DArray(_src, value);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniformMatrix4x2fv(programNew, locationNew, count, transpose, value);
    // ---------- register handles ------------
    */
}

static void retrace_glProgramUniformMatrix4x3fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    unsigned char transpose; // literal
    _src = ReadFixed<unsigned char>(_src, transpose);
    Array<float> value; // array
    _src = Read1DArray(_src, value);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glProgramUniformMatrix4x3fv(programNew, locationNew, count, transpose, value);
    // ---------- register handles ------------
    */
}

static void retrace_glReadBuffer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int src; // enum
    _src = ReadFixed(_src, src);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glReadBuffer(src);
    // ---------- register handles ------------
}

static void retrace_glRenderbufferStorageMultisample(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int samples; // literal
    _src = ReadFixed<int>(_src, samples);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glRenderbufferStorageMultisample(target, samples, internalformat, width, height);
    // ---------- register handles ------------
    gRetracer.curContextState->curFrameBufferState.curRenderBufferState.ff_glRenderbufferStorage(gRetracer.mCurCallNo);
}

static void retrace_glResumeTransformFeedback(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glResumeTransformFeedback();
    // ---------- register handles ------------
}

static void retrace_glSampleMaski(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int maskNumber; // literal
    _src = ReadFixed<unsigned int>(_src, maskNumber);
    unsigned int mask; // literal
    _src = ReadFixed<unsigned int>(_src, mask);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glSampleMaski(maskNumber, mask);
    // ---------- register handles ------------
}

static void retrace_glSamplerParameterf(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int sampler; // literal
    _src = ReadFixed<unsigned int>(_src, sampler);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    float param; // literal
    _src = ReadFixed<float>(_src, param);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint samplerNew = context.getSamplerMap().RValue(sampler);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glSamplerParameterf(samplerNew, pname, param);
    // ---------- register handles ------------
    */
    gRetracer.curContextState->curTextureState.ff_glSamplerParameter_Types(sampler, pname, gRetracer.mCurCallNo);
}

static void retrace_glSamplerParameterfv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int sampler; // literal
    _src = ReadFixed<unsigned int>(_src, sampler);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<float> param; // array
    _src = Read1DArray(_src, param);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint samplerNew = context.getSamplerMap().RValue(sampler);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glSamplerParameterfv(samplerNew, pname, param);
    // ---------- register handles ------------
    */
}

static void retrace_glSamplerParameteri(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int sampler; // literal
    _src = ReadFixed<unsigned int>(_src, sampler);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    int param; // literal
    _src = ReadFixed<int>(_src, param);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint samplerNew = context.getSamplerMap().RValue(sampler);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glSamplerParameteri(samplerNew, pname, param);
    // ---------- register handles ------------
    */
    gRetracer.curContextState->curTextureState.ff_glSamplerParameter_Types(sampler, pname, gRetracer.mCurCallNo);
}

static void retrace_glSamplerParameteriv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int sampler; // literal
    _src = ReadFixed<unsigned int>(_src, sampler);
    int pname; // enum
    _src = ReadFixed(_src, pname);
    Array<int> param; // array
    _src = Read1DArray(_src, param);

    /*    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint samplerNew = context.getSamplerMap().RValue(sampler);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glSamplerParameteriv(samplerNew, pname, param);
    // ---------- register handles ------------
    */
}

static void retrace_glTexImage3D(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int internalformat; // literal
    _src = ReadFixed<int>(_src, internalformat);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);
    int depth; // literal
    _src = ReadFixed<int>(_src, depth);
    int border; // literal
    _src = ReadFixed<int>(_src, border);
    int format; // enum
    _src = ReadFixed(_src, format);
    int type; // enum
    _src = ReadFixed(_src, type);
    //    GLvoid* pixels = NULL;
    Array<char> pixelsBlob;
    if (gRetracer.getFileFormatVersion() <= HEADER_VERSION_3)
    {
        _src = Read1DArray(_src, pixelsBlob);
        //      pixels = pixelsBlob.v;
    }
    else
    {
        unsigned int _opaque_type = 0;
        _src = ReadFixed(_src, _opaque_type);
        switch (_opaque_type)
        {
        case BlobType:
            _src = Read1DArray(_src, pixelsBlob);
            //       pixels = pixelsBlob.v;
            break;
        case BufferObjectReferenceType:
            unsigned int bufferOffset = 0;
            _src = ReadFixed<unsigned int>(_src, bufferOffset);
             //      pixels = (GLvoid *)static_cast<uintptr_t>(bufferOffset);
            break;
        }
    }

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
    // ---------- register handles ------------

}

static void retrace_glTexStorage2D(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int levels; // literal
    _src = ReadFixed<int>(_src, levels);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glTexStorage2D(target, levels, internalformat, width, height);
    // ---------- register handles ------------
    std::unordered_set<unsigned int> newList;

    gRetracer.curContextState->curTextureState.ff_glTypes_TexImage2D(target, 0, gRetracer.mCurCallNo, newList);
}

static void retrace_glTexStorage2DMultisample(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int samples; // literal
    _src = ReadFixed<int>(_src, samples);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);
    unsigned char fixedsamplelocations; // literal
    _src = ReadFixed<unsigned char>(_src, fixedsamplelocations);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glTexStorage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
    // ---------- register handles ------------
}

static void retrace_glTexStorage3D(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int levels; // literal
    _src = ReadFixed<int>(_src, levels);
    int internalformat; // enum
    _src = ReadFixed(_src, internalformat);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);
    int depth; // literal
    _src = ReadFixed<int>(_src, depth);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glTexStorage3D(target, levels, internalformat, width, height, depth);
    // ---------- register handles ------------
    std::unordered_set<unsigned int> newList;

    gRetracer.curContextState->curTextureState.ff_glTypes_TexImage2D(target, 0, gRetracer.mCurCallNo, newList);
}

static void retrace_glTexSubImage3D(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);
    int level; // literal
    _src = ReadFixed<int>(_src, level);
    int xoffset; // literal
    _src = ReadFixed<int>(_src, xoffset);
    int yoffset; // literal
    _src = ReadFixed<int>(_src, yoffset);
    int zoffset; // literal
    _src = ReadFixed<int>(_src, zoffset);
    int width; // literal
    _src = ReadFixed<int>(_src, width);
    int height; // literal
    _src = ReadFixed<int>(_src, height);
    int depth; // literal
    _src = ReadFixed<int>(_src, depth);
    int format; // enum
    _src = ReadFixed(_src, format);
    int type; // enum
    _src = ReadFixed(_src, type);
    //  GLvoid* pixels = NULL;
    Array<char> pixelsBlob;
    if (gRetracer.getFileFormatVersion() <= HEADER_VERSION_3)
    {
        _src = Read1DArray(_src, pixelsBlob);
        //      pixels = pixelsBlob.v;
    }
    else
    {
        unsigned int _opaque_type = 0;
        _src = ReadFixed(_src, _opaque_type);
        switch (_opaque_type)
        {
        case BlobType:
            _src = Read1DArray(_src, pixelsBlob);
            //       pixels = pixelsBlob.v;
            break;
        case BufferObjectReferenceType:
            unsigned int bufferOffset = 0;
            _src = ReadFixed<unsigned int>(_src, bufferOffset);
            //      pixels = (GLvoid *)static_cast<uintptr_t>(bufferOffset);
            break;
        }
    }

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
    // ---------- register handles ------------

    std::unordered_set<unsigned int> newList;
    unsigned int GL_PIXEL_UNPACK_BUFFERNo = gRetracer.curContextState->curBufferState.getCurGL_PIXEL_UNPACK_BUFFERNo();
    unsigned int GL_PIXEL_UNPACK_BUFFERIdx = gRetracer.curContextState->curBufferState.getCurGL_PIXEL_UNPACK_BUFFERIdx();
    if(GL_PIXEL_UNPACK_BUFFERIdx!=0)
    {
        gRetracer.curContextState->curBufferState.findABufferState(GL_PIXEL_UNPACK_BUFFERIdx, newList);
    }
    gRetracer.curContextState->curTextureState.ff_glTypes_TexSubImage2D(target, GL_PIXEL_UNPACK_BUFFERNo, gRetracer.mCurCallNo, newList);
}

static void retrace_glTransformFeedbackVaryings(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<const char*> varyings; // string array
    _src = ReadStringArray(_src, varyings);
    int bufferMode; // enum
    _src = ReadFixed(_src, bufferMode);

    /*    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glTransformFeedbackVaryings(programNew, count, varyings, bufferMode);
    // ---------- register handles ------------
    */
    gRetracer.callNoList.insert(gRetracer.mCurCallNo);
}

static void retrace_glUniform1ui(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    unsigned int v0; // literal
    _src = ReadFixed<unsigned int>(_src, v0);

    /*    // --------- assistant parameters ---------
    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform1ui(locationNew, v0);
    // ---------- register handles ------------
    */
    gRetracer.curContextState->curProgramState.ff_glUniform_Types(location, gRetracer.mCurCallNo);
}

static void retrace_glUniform1uiv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<unsigned int> value; // array
    _src = Read1DArray(_src, value);

    /*    // --------- assistant parameters ---------
    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform1uiv(locationNew, count, value);
    // ---------- register handles ------------
    */
    gRetracer.curContextState->curProgramState.ff_glUniform_Types(location, gRetracer.mCurCallNo);
}

static void retrace_glUniform2ui(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    unsigned int v0; // literal
    _src = ReadFixed<unsigned int>(_src, v0);
    unsigned int v1; // literal
    _src = ReadFixed<unsigned int>(_src, v1);

    /*    // --------- assistant parameters ---------
    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform2ui(locationNew, v0, v1);
    // ---------- register handles ------------
    */
}

static void retrace_glUniform2uiv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<unsigned int> value; // array
    _src = Read1DArray(_src, value);

    /*    // --------- assistant parameters ---------
    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform2uiv(locationNew, count, value);
    // ---------- register handles ------------
    */
}

static void retrace_glUniform3ui(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    unsigned int v0; // literal
    _src = ReadFixed<unsigned int>(_src, v0);
    unsigned int v1; // literal
    _src = ReadFixed<unsigned int>(_src, v1);
    unsigned int v2; // literal
    _src = ReadFixed<unsigned int>(_src, v2);

    /*    // --------- assistant parameters ---------
    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform3ui(locationNew, v0, v1, v2);
    // ---------- register handles ------------
    */
}

static void retrace_glUniform3uiv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<unsigned int> value; // array
    _src = Read1DArray(_src, value);

    /*    // --------- assistant parameters ---------
    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform3uiv(locationNew, count, value);
    // ---------- register handles ------------
    */
}

static void retrace_glUniform4ui(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    unsigned int v0; // literal
    _src = ReadFixed<unsigned int>(_src, v0);
    unsigned int v1; // literal
    _src = ReadFixed<unsigned int>(_src, v1);
    unsigned int v2; // literal
    _src = ReadFixed<unsigned int>(_src, v2);
    unsigned int v3; // literal
    _src = ReadFixed<unsigned int>(_src, v3);

    /*    // --------- assistant parameters ---------
    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform4ui(locationNew, v0, v1, v2, v3);
    // ---------- register handles ------------
    */
}

static void retrace_glUniform4uiv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    Array<unsigned int> value; // array
    _src = Read1DArray(_src, value);

    /*    // --------- assistant parameters ---------
    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniform4uiv(locationNew, count, value);
    // ---------- register handles ------------
    */
    gRetracer.curContextState->curProgramState.ff_glUniform_Types(location, gRetracer.mCurCallNo);
}

static void retrace_glUniformBlockBinding(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);
    unsigned int uniformBlockIndex; // literal
    _src = ReadFixed<unsigned int>(_src, uniformBlockIndex);
    unsigned int uniformBlockBinding; // literal
    _src = ReadFixed<unsigned int>(_src, uniformBlockBinding);
    /*
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint programNew = context.getProgramMap().RValue(program);
    GLuint uniformBlockIndexNew = context._uniformBlockIndex_map[programNew].RValue(uniformBlockIndex);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniformBlockBinding(programNew, uniformBlockIndexNew, uniformBlockBinding);
    // ---------- register handles ------------
    */
    //gRetracer.callNoList.insert(gRetracer.mCurCallNo);
    gRetracer.curContextState->curProgramState.ff_glUniformBlockBinding(program, gRetracer.mCurCallNo);
}

static void retrace_glUniformMatrix2x3fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    unsigned char transpose; // literal
    _src = ReadFixed<unsigned char>(_src, transpose);
    Array<float> value; // array
    _src = Read1DArray(_src, value);

    /*    // --------- assistant parameters ---------
    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniformMatrix2x3fv(locationNew, count, transpose, value);
    // ---------- register handles ------------
    */
}

static void retrace_glUniformMatrix2x4fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    unsigned char transpose; // literal
    _src = ReadFixed<unsigned char>(_src, transpose);
    Array<float> value; // array
    _src = Read1DArray(_src, value);

    /*    // --------- assistant parameters ---------
    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniformMatrix2x4fv(locationNew, count, transpose, value);
    // ---------- register handles ------------
    */
}

static void retrace_glUniformMatrix3x2fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    unsigned char transpose; // literal
    _src = ReadFixed<unsigned char>(_src, transpose);
    Array<float> value; // array
    _src = Read1DArray(_src, value);

    /*   // --------- assistant parameters ---------
    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniformMatrix3x2fv(locationNew, count, transpose, value);
    // ---------- register handles ------------
    */
}

static void retrace_glUniformMatrix3x4fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    unsigned char transpose; // literal
    _src = ReadFixed<unsigned char>(_src, transpose);
    Array<float> value; // array
    _src = Read1DArray(_src, value);

    /*    // --------- assistant parameters ---------
    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniformMatrix3x4fv(locationNew, count, transpose, value);
    // ---------- register handles ------------
    */
}

static void retrace_glUniformMatrix4x2fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    unsigned char transpose; // literal
    _src = ReadFixed<unsigned char>(_src, transpose);
    Array<float> value; // array
    _src = Read1DArray(_src, value);

    /*    // --------- assistant parameters ---------
    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniformMatrix4x2fv(locationNew, count, transpose, value);
    // ---------- register handles ------------
    */
}

static void retrace_glUniformMatrix4x3fv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int location; // literal
    _src = ReadFixed<int>(_src, location);
    int count; // literal
    _src = ReadFixed<int>(_src, count);
    unsigned char transpose; // literal
    _src = ReadFixed<unsigned char>(_src, transpose);
    Array<float> value; // array
    _src = Read1DArray(_src, value);

    /*    // --------- assistant parameters ---------
    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;
    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)
    {
        GLint pipeline = 0;
        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
        if (pipeline != 0)
        {
            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);
        }
    }
    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLint locationNew = context._uniformLocation_map[programNew].RValue(location);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUniformMatrix4x3fv(locationNew, count, transpose, value);
    // ---------- register handles ------------
    */
}

static void retrace_glUnmapBuffer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    int target; // enum
    _src = ReadFixed(_src, target);

    unsigned char old_ret; // literal
    _src = ReadFixed<unsigned char>(_src, old_ret);
    // ----------- look up handles ------------
    /*    // ----------- out parameters  ------------
    GLboolean ret = 0;
    // ------------- pre retrace ------------------
    GLuint bufferId = getBoundBuffer(target);
    gRetracer.getCurrentContext()._bufferToData_map.erase(bufferId);
    // ------------- retrace ------------------
    ret = glUnmapBuffer(target);
    // ---------- register handles ------------
    (void)ret;  // Ignored return value
    */

    //gRetracer.curContextState->curClientSideBufferState.ff_glUnmapBuffer(target, gRetracer.mCurCallNo);
    gRetracer.curContextState->curBufferState.ff_glUnmapBuffer(target, gRetracer.mCurCallNo);
}

static void retrace_glUseProgramStages(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int pipeline; // literal
    _src = ReadFixed<unsigned int>(_src, pipeline);
    unsigned int stages; // literal
    _src = ReadFixed<unsigned int>(_src, stages);
    unsigned int program; // literal
    _src = ReadFixed<unsigned int>(_src, program);

    // ----------- look up handles ------------
    /*    Context& context = gRetracer.getCurrentContext();
    GLuint pipelineNew = context._pipeline_map.RValue(pipeline);
    GLuint programNew = context.getProgramMap().RValue(program);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glUseProgramStages(pipelineNew, stages, programNew);
    // ---------- register handles ------------
    */
}

static void retrace_glValidateProgramPipeline(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int pipeline; // literal
    _src = ReadFixed<unsigned int>(_src, pipeline);

    /*    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLuint pipelineNew = context._pipeline_map.RValue(pipeline);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glValidateProgramPipeline(pipelineNew);
    // ---------- register handles ------------
    */
}

static void retrace_glVertexAttribBinding(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int attribindex; // literal
    _src = ReadFixed<unsigned int>(_src, attribindex);
    unsigned int bindingindex; // literal
    _src = ReadFixed<unsigned int>(_src, bindingindex);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glVertexAttribBinding(attribindex, bindingindex);
    // ---------- register handles ------------
}

static void retrace_glVertexAttribDivisor(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    unsigned int divisor; // literal
    _src = ReadFixed<unsigned int>(_src, divisor);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glVertexAttribDivisor(index, divisor);
    // ---------- register handles ------------

    gRetracer.curContextState->curGlobalState.ff_glVertexAttribDivisor(index, gRetracer.mCurCallNo);
}

static void retrace_glVertexAttribFormat(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int attribindex; // literal
    _src = ReadFixed<unsigned int>(_src, attribindex);
    int size; // literal
    _src = ReadFixed<int>(_src, size);
    int type; // enum
    _src = ReadFixed(_src, type);
    unsigned char normalized; // literal
    _src = ReadFixed<unsigned char>(_src, normalized);
    unsigned int relativeoffset; // literal
    _src = ReadFixed<unsigned int>(_src, relativeoffset);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glVertexAttribFormat(attribindex, size, type, normalized, relativeoffset);
    // ---------- register handles ------------
}

static void retrace_glVertexAttribI4i(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    int x; // literal
    _src = ReadFixed<int>(_src, x);
    int y; // literal
    _src = ReadFixed<int>(_src, y);
    int z; // literal
    _src = ReadFixed<int>(_src, z);
    int w; // literal
    _src = ReadFixed<int>(_src, w);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glVertexAttribI4i(index, x, y, z, w);
    // ---------- register handles ------------
}

static void retrace_glVertexAttribI4iv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    Array<int> v; // array
    _src = Read1DArray(_src, v);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glVertexAttribI4iv(index, v);
    // ---------- register handles ------------
}

static void retrace_glVertexAttribI4ui(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    unsigned int x; // literal
    _src = ReadFixed<unsigned int>(_src, x);
    unsigned int y; // literal
    _src = ReadFixed<unsigned int>(_src, y);
    unsigned int z; // literal
    _src = ReadFixed<unsigned int>(_src, z);
    unsigned int w; // literal
    _src = ReadFixed<unsigned int>(_src, w);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glVertexAttribI4ui(index, x, y, z, w);
    // ---------- register handles ------------
}

static void retrace_glVertexAttribI4uiv(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    Array<unsigned int> v; // array
    _src = Read1DArray(_src, v);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glVertexAttribI4uiv(index, v);
    // ---------- register handles ------------
}

static void retrace_glVertexAttribIFormat(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int attribindex; // literal
    _src = ReadFixed<unsigned int>(_src, attribindex);
    int size; // literal
    _src = ReadFixed<int>(_src, size);
    int type; // enum
    _src = ReadFixed(_src, type);
    unsigned int relativeoffset; // literal
    _src = ReadFixed<unsigned int>(_src, relativeoffset);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //   glVertexAttribIFormat(attribindex, size, type, relativeoffset);
    // ---------- register handles ------------
}

static void retrace_glVertexAttribIPointer(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int index; // literal
    _src = ReadFixed<unsigned int>(_src, index);
    int size; // literal
    _src = ReadFixed<int>(_src, size);
    int type; // enum
    _src = ReadFixed(_src, type);
    int stride; // literal
    _src = ReadFixed<int>(_src, stride);
    unsigned int csbName = NO_IDX; // ClientSideBuffer name
    //  GLvoid* pointer = NULL;
    unsigned int _opaque_type = 0;
    _src = ReadFixed(_src, _opaque_type);
    if (_opaque_type == BufferObjectReferenceType)
    { // simple memory offset
        unsigned int pointer_raw; // raw ptr
        _src = ReadFixed<unsigned int>(_src, pointer_raw);
        //       pointer = (GLvoid *)static_cast<uintptr_t>(pointer_raw);
    }
    else if (_opaque_type == BlobType)
    { // blob type
        Array<float> pointer_blob;
        _src = Read1DArray(_src, pointer_blob);
        //       pointer = pointer_blob.v;
    }
    else if (_opaque_type == ClientSideBufferObjectReferenceType)
    { // a memory reference to client-side buffer
        unsigned int offset = 0;
        _src = ReadFixed<unsigned int>(_src, csbName);
        _src = ReadFixed<unsigned int>(_src, offset);
        //    pointer = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);
    }
    else if (_opaque_type == NoopType)
    { // Do nothing
    }

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glVertexAttribIPointer(index, size, type, stride, pointer);
    // ---------- register handles ------------
    //for fastforwad

    unsigned int bindbufferIdx = gRetracer.curContextState->curBufferState.getCurGL_ARRAY_BUFFERIdx();
    unsigned int bindbufferNo = gRetracer.curContextState->curBufferState.getCurGL_ARRAY_BUFFERNo();
    gRetracer.curContextState->curGlobalState.ff_glVertexAttribPointer(index, gRetracer.mCurCallNo, csbName, bindbufferIdx, bindbufferNo);
}

static void retrace_glVertexBindingDivisor(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    unsigned int bindingindex; // literal
    _src = ReadFixed<unsigned int>(_src, bindingindex);
    unsigned int divisor; // literal
    _src = ReadFixed<unsigned int>(_src, divisor);

    // ----------- look up handles ------------
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    //  glVertexBindingDivisor(bindingindex, divisor);
    // ---------- register handles ------------
}

static void retrace_glWaitSync(char* _src)
{
    // ------------- deserialize --------------
    // ------- params & ret definition --------
    uint64_t sync; // literal
    _src = ReadFixed<uint64_t>(_src, sync);
    unsigned int flags; // literal
    _src = ReadFixed<unsigned int>(_src, flags);
    uint64_t timeout; // literal
    _src = ReadFixed<uint64_t>(_src, timeout);

    /*    // ----------- look up handles ------------
    Context& context = gRetracer.getCurrentContext();
    GLsync syncNew = context.getSyncMap().RValue(sync);
    // ----------- out parameters  ------------
    // ------------- pre retrace ------------------
    // ------------- retrace ------------------
    glWaitSync(syncNew, flags, timeout);
    // ---------- register handles ------------
    */
}


const common::EntryMap retracer::gles_callbacks = {
    {"glActiveTexture", (void*)retrace_glActiveTexture},
    {"glAttachShader", (void*)retrace_glAttachShader},
    {"glBindAttribLocation", (void*)retrace_glBindAttribLocation},
    {"glBindBuffer", (void*)retrace_glBindBuffer},
    {"glBindFramebuffer", (void*)retrace_glBindFramebuffer},
    {"glBindRenderbuffer", (void*)retrace_glBindRenderbuffer},
    {"glBindTexture", (void*)retrace_glBindTexture},
    {"glBlendColor", (void*)retrace_glBlendColor},
    {"glBlendEquation", (void*)retrace_glBlendEquation},
    {"glBlendEquationSeparate", (void*)retrace_glBlendEquationSeparate},
    {"glBlendFunc", (void*)retrace_glBlendFunc},
    {"glBlendFuncSeparate", (void*)retrace_glBlendFuncSeparate},
    {"glBufferData", (void*)retrace_glBufferData},
    {"glBufferSubData", (void*)retrace_glBufferSubData},
    {"glCheckFramebufferStatus", (void*)retrace_glCheckFramebufferStatus},
    {"glClear", (void*)retrace_glClear},
    {"glClearColor", (void*)retrace_glClearColor},
    {"glClearDepthf", (void*)retrace_glClearDepthf},
    {"glClearStencil", (void*)retrace_glClearStencil},
    {"glColorMask", (void*)retrace_glColorMask},
    {"glCompileShader", (void*)retrace_glCompileShader},
    {"glCompressedTexImage2D", (void*)retrace_glCompressedTexImage2D},
    {"glCompressedTexSubImage2D", (void*)retrace_glCompressedTexSubImage2D},
    {"glCopyTexImage2D", (void*)retrace_glCopyTexImage2D},
    {"glCopyTexSubImage2D", (void*)retrace_glCopyTexSubImage2D},
    {"glCreateProgram", (void*)retrace_glCreateProgram},
    {"glCreateShader", (void*)retrace_glCreateShader},
    {"glCullFace", (void*)retrace_glCullFace},
    {"glDeleteBuffers", (void*)retrace_glDeleteBuffers},
    {"glDeleteFramebuffers", (void*)retrace_glDeleteFramebuffers},
    {"glDeleteProgram", (void*)retrace_glDeleteProgram},
    {"glDeleteRenderbuffers", (void*)retrace_glDeleteRenderbuffers},
    {"glDeleteShader", (void*)retrace_glDeleteShader},
    {"glDeleteTextures", (void*)retrace_glDeleteTextures},
    {"glDepthFunc", (void*)retrace_glDepthFunc},
    {"glDepthMask", (void*)retrace_glDepthMask},
    {"glDepthRangef", (void*)retrace_glDepthRangef},
    {"glDetachShader", (void*)retrace_glDetachShader},
    {"glDisable", (void*)retrace_glDisable},
    {"glDisableVertexAttribArray", (void*)retrace_glDisableVertexAttribArray},
    {"glDrawArrays", (void*)retrace_glDrawArrays},
    {"glDrawElements", (void*)retrace_glDrawElements},
    {"glEnable", (void*)retrace_glEnable},
    {"glEnableVertexAttribArray", (void*)retrace_glEnableVertexAttribArray},
    {"glFinish", (void*)retrace_glFinish},
    {"glFlush", (void*)retrace_glFlush},
    {"glFramebufferRenderbuffer", (void*)retrace_glFramebufferRenderbuffer},
    {"glFramebufferTexture2D", (void*)retrace_glFramebufferTexture2D},
    {"glFrontFace", (void*)retrace_glFrontFace},
    {"glGenBuffers", (void*)retrace_glGenBuffers},
    {"glGenFramebuffers", (void*)retrace_glGenFramebuffers},
    {"glGenRenderbuffers", (void*)retrace_glGenRenderbuffers},
    {"glGenTextures", (void*)retrace_glGenTextures},
    {"glGenerateMipmap", (void*)retrace_glGenerateMipmap},
    {"glGetBooleanv", (void*)ignore},
    {"glGetFloatv", (void*)ignore},
    {"glGetIntegerv", (void*)ignore},
    {"glGetActiveAttrib", (void*)ignore},
    {"glGetActiveUniform", (void*)ignore},
    {"glGetAttachedShaders", (void*)ignore},
    {"glGetAttribLocation", (void*)retrace_glGetAttribLocation},
    {"glGetBufferParameteriv", (void*)ignore},
    {"glGetError", (void*)ignore},
    {"glGetFramebufferAttachmentParameteriv", (void*)ignore},
    {"glGetProgramInfoLog", (void*)ignore},
    {"glGetProgramiv", (void*)ignore},
    {"glGetRenderbufferParameteriv", (void*)ignore},
    {"glGetShaderInfoLog", (void*)ignore},
    {"glGetShaderPrecisionFormat", (void*)ignore},
    {"glGetShaderSource", (void*)ignore},
    {"glGetShaderiv", (void*)ignore},
    {"glGetString", (void*)ignore},
    {"glGetTexParameterfv", (void*)ignore},
    {"glGetTexParameteriv", (void*)ignore},
    {"glGetUniformfv", (void*)ignore},
    {"glGetUniformiv", (void*)ignore},
    {"glGetUniformLocation", (void*)retrace_glGetUniformLocation},
    {"glGetVertexAttribfv", (void*)ignore},
    {"glGetVertexAttribiv", (void*)ignore},
    {"glGetVertexAttribPointerv", (void*)ignore},
    {"glHint", (void*)retrace_glHint},
    {"glIsBuffer", (void*)ignore},
    {"glIsEnabled", (void*)ignore},
    {"glIsFramebuffer", (void*)ignore},
    {"glIsProgram", (void*)ignore},
    {"glIsRenderbuffer", (void*)ignore},
    {"glIsShader", (void*)ignore},
    {"glIsTexture", (void*)ignore},
    {"glLineWidth", (void*)retrace_glLineWidth},
    {"glLinkProgram", (void*)retrace_glLinkProgram},
    {"glPixelStorei", (void*)retrace_glPixelStorei},
    {"glPolygonOffset", (void*)retrace_glPolygonOffset},
    {"glReadPixels", (void*)retrace_glReadPixels},
    {"glReleaseShaderCompiler", (void*)retrace_glReleaseShaderCompiler},
    {"glRenderbufferStorage", (void*)retrace_glRenderbufferStorage},
    {"glSampleCoverage", (void*)retrace_glSampleCoverage},
    {"glScissor", (void*)retrace_glScissor},
    {"glShaderBinary", (void*)retrace_glShaderBinary},
    {"glShaderSource", (void*)retrace_glShaderSource},
    {"glStencilFunc", (void*)retrace_glStencilFunc},
    {"glStencilFuncSeparate", (void*)retrace_glStencilFuncSeparate},
    {"glStencilMask", (void*)retrace_glStencilMask},
    {"glStencilMaskSeparate", (void*)retrace_glStencilMaskSeparate},
    {"glStencilOp", (void*)retrace_glStencilOp},
    {"glStencilOpSeparate", (void*)retrace_glStencilOpSeparate},
    {"glTexImage2D", (void*)retrace_glTexImage2D},
    {"glTexParameterf", (void*)retrace_glTexParameterf},
    {"glTexParameterfv", (void*)retrace_glTexParameterfv},
    {"glTexParameteri", (void*)retrace_glTexParameteri},
    {"glTexParameteriv", (void*)retrace_glTexParameteriv},
    {"glTexSubImage2D", (void*)retrace_glTexSubImage2D},
    {"glUniform1f", (void*)retrace_glUniform1f},
    {"glUniform2f", (void*)retrace_glUniform2f},
    {"glUniform3f", (void*)retrace_glUniform3f},
    {"glUniform4f", (void*)retrace_glUniform4f},
    {"glUniform1i", (void*)retrace_glUniform1i},
    {"glUniform2i", (void*)retrace_glUniform2i},
    {"glUniform3i", (void*)retrace_glUniform3i},
    {"glUniform4i", (void*)retrace_glUniform4i},
    {"glUniform1fv", (void*)retrace_glUniform1fv},
    {"glUniform2fv", (void*)retrace_glUniform2fv},
    {"glUniform3fv", (void*)retrace_glUniform3fv},
    {"glUniform4fv", (void*)retrace_glUniform4fv},
    {"glUniform1iv", (void*)retrace_glUniform1iv},
    {"glUniform2iv", (void*)retrace_glUniform2iv},
    {"glUniform3iv", (void*)retrace_glUniform3iv},
    {"glUniform4iv", (void*)retrace_glUniform4iv},
    {"glUniformMatrix2fv", (void*)retrace_glUniformMatrix2fv},
    {"glUniformMatrix3fv", (void*)retrace_glUniformMatrix3fv},
    {"glUniformMatrix4fv", (void*)retrace_glUniformMatrix4fv},
    {"glUseProgram", (void*)retrace_glUseProgram},
    {"glValidateProgram", (void*)retrace_glValidateProgram},
    {"glVertexAttrib1f", (void*)retrace_glVertexAttrib1f},
    {"glVertexAttrib2f", (void*)retrace_glVertexAttrib2f},
    {"glVertexAttrib3f", (void*)retrace_glVertexAttrib3f},
    {"glVertexAttrib4f", (void*)retrace_glVertexAttrib4f},
    {"glVertexAttrib1fv", (void*)retrace_glVertexAttrib1fv},
    {"glVertexAttrib2fv", (void*)retrace_glVertexAttrib2fv},
    {"glVertexAttrib3fv", (void*)retrace_glVertexAttrib3fv},
    {"glVertexAttrib4fv", (void*)retrace_glVertexAttrib4fv},
    {"glVertexAttribPointer", (void*)retrace_glVertexAttribPointer},
    {"glViewport", (void*)retrace_glViewport},
    {"glAlphaFunc", (void*)retrace_glAlphaFunc},
    {"glAlphaFuncx", (void*)retrace_glAlphaFuncx},
    {"glClearColorx", (void*)retrace_glClearColorx},
    {"glClearDepthx", (void*)retrace_glClearDepthx},
    {"glClientActiveTexture", (void*)retrace_glClientActiveTexture},
    {"glClipPlanef", (void*)retrace_glClipPlanef},
    {"glClipPlanex", (void*)retrace_glClipPlanex},
    {"glColor4f", (void*)retrace_glColor4f},
    {"glColor4ub", (void*)retrace_glColor4ub},
    {"glColor4x", (void*)retrace_glColor4x},
    {"glColorPointer", (void*)retrace_glColorPointer},
    {"glDepthRangex", (void*)retrace_glDepthRangex},
    {"glDisableClientState", (void*)retrace_glDisableClientState},
    {"glEnableClientState", (void*)retrace_glEnableClientState},
    {"glFogf", (void*)retrace_glFogf},
    {"glFogx", (void*)retrace_glFogx},
    {"glFogfv", (void*)retrace_glFogfv},
    {"glFogxv", (void*)retrace_glFogxv},
    {"glFrustumf", (void*)retrace_glFrustumf},
    {"glFrustumx", (void*)retrace_glFrustumx},
    {"glGetFixedv", (void*)ignore},
    {"glGetClipPlanef", (void*)ignore},
    {"glGetClipPlanex", (void*)ignore},
    {"glGetLightfv", (void*)ignore},
    {"glGetLightxv", (void*)ignore},
    {"glGetMaterialfv", (void*)ignore},
    {"glGetMaterialxv", (void*)ignore},
    {"glGetPointerv", (void*)ignore},
    {"glGetPointervKHR", (void*)ignore},
    {"glGetTexEnvfv", (void*)ignore},
    {"glGetTexEnviv", (void*)ignore},
    {"glGetTexEnvxv", (void*)ignore},
    {"glGetTexParameterxv", (void*)ignore},
    {"glLightf", (void*)retrace_glLightf},
    {"glLightfv", (void*)retrace_glLightfv},
    {"glLightx", (void*)retrace_glLightx},
    {"glLightxv", (void*)retrace_glLightxv},
    {"glLightModelf", (void*)retrace_glLightModelf},
    {"glLightModelx", (void*)retrace_glLightModelx},
    {"glLightModelfv", (void*)retrace_glLightModelfv},
    {"glLightModelxv", (void*)retrace_glLightModelxv},
    {"glLineWidthx", (void*)retrace_glLineWidthx},
    {"glLoadIdentity", (void*)retrace_glLoadIdentity},
    {"glLoadMatrixf", (void*)retrace_glLoadMatrixf},
    {"glLoadMatrixx", (void*)retrace_glLoadMatrixx},
    {"glLogicOp", (void*)retrace_glLogicOp},
    {"glMaterialf", (void*)retrace_glMaterialf},
    {"glMaterialx", (void*)retrace_glMaterialx},
    {"glMaterialfv", (void*)retrace_glMaterialfv},
    {"glMaterialxv", (void*)retrace_glMaterialxv},
    {"glMatrixMode", (void*)retrace_glMatrixMode},
    {"glMultMatrixf", (void*)retrace_glMultMatrixf},
    {"glMultMatrixx", (void*)retrace_glMultMatrixx},
    {"glMultiTexCoord4f", (void*)retrace_glMultiTexCoord4f},
    {"glMultiTexCoord4x", (void*)retrace_glMultiTexCoord4x},
    {"glNormal3f", (void*)retrace_glNormal3f},
    {"glNormal3x", (void*)retrace_glNormal3x},
    {"glNormalPointer", (void*)retrace_glNormalPointer},
    {"glOrthof", (void*)retrace_glOrthof},
    {"glOrthox", (void*)retrace_glOrthox},
    {"glPointParameterf", (void*)retrace_glPointParameterf},
    {"glPointParameterx", (void*)retrace_glPointParameterx},
    {"glPointParameterfv", (void*)retrace_glPointParameterfv},
    {"glPointParameterxv", (void*)retrace_glPointParameterxv},
    {"glPointSize", (void*)retrace_glPointSize},
    {"glPointSizex", (void*)retrace_glPointSizex},
    {"glPointSizePointerOES", (void*)retrace_glPointSizePointerOES},
    {"glPolygonOffsetx", (void*)retrace_glPolygonOffsetx},
    {"glPopMatrix", (void*)retrace_glPopMatrix},
    {"glPushMatrix", (void*)retrace_glPushMatrix},
    {"glRotatef", (void*)retrace_glRotatef},
    {"glRotatex", (void*)retrace_glRotatex},
    {"glSampleCoveragex", (void*)retrace_glSampleCoveragex},
    {"glScalef", (void*)retrace_glScalef},
    {"glScalex", (void*)retrace_glScalex},
    {"glShadeModel", (void*)retrace_glShadeModel},
    {"glTexCoordPointer", (void*)retrace_glTexCoordPointer},
    {"glTexEnvf", (void*)retrace_glTexEnvf},
    {"glTexEnvi", (void*)retrace_glTexEnvi},
    {"glTexEnvx", (void*)retrace_glTexEnvx},
    {"glTexEnvfv", (void*)retrace_glTexEnvfv},
    {"glTexEnviv", (void*)retrace_glTexEnviv},
    {"glTexEnvxv", (void*)retrace_glTexEnvxv},
    {"glTexParameterx", (void*)retrace_glTexParameterx},
    {"glTexParameterxv", (void*)retrace_glTexParameterxv},
    {"glTranslatef", (void*)retrace_glTranslatef},
    {"glTranslatex", (void*)retrace_glTranslatex},
    {"glVertexPointer", (void*)retrace_glVertexPointer},
    {"glBlendEquationSeparateOES", (void*)retrace_glBlendEquationSeparateOES},
    {"glBlendFuncSeparateOES", (void*)retrace_glBlendFuncSeparateOES},
    {"glBlendEquationOES", (void*)retrace_glBlendEquationOES},
    {"glIsRenderbufferOES", (void*)ignore},
    {"glBindRenderbufferOES", (void*)retrace_glBindRenderbufferOES},
    {"glDeleteRenderbuffersOES", (void*)retrace_glDeleteRenderbuffersOES},
    {"glGenRenderbuffersOES", (void*)retrace_glGenRenderbuffersOES},
    {"glRenderbufferStorageOES", (void*)retrace_glRenderbufferStorageOES},
    {"glGetRenderbufferParameterivOES", (void*)ignore},
    {"glIsFramebufferOES", (void*)ignore},
    {"glBindFramebufferOES", (void*)retrace_glBindFramebufferOES},
    {"glDeleteFramebuffersOES", (void*)retrace_glDeleteFramebuffersOES},
    {"glGenFramebuffersOES", (void*)retrace_glGenFramebuffersOES},
    {"glCheckFramebufferStatusOES", (void*)retrace_glCheckFramebufferStatusOES},
    {"glFramebufferTexture2DOES", (void*)retrace_glFramebufferTexture2DOES},
    {"glFramebufferRenderbufferOES", (void*)retrace_glFramebufferRenderbufferOES},
    {"glGetFramebufferAttachmentParameterivOES", (void*)ignore},
    {"glGenerateMipmapOES", (void*)retrace_glGenerateMipmapOES},
    {"glCurrentPaletteMatrixOES", (void*)retrace_glCurrentPaletteMatrixOES},
    {"glLoadPaletteFromModelViewMatrixOES", (void*)retrace_glLoadPaletteFromModelViewMatrixOES},
    {"glMatrixIndexPointerOES", (void*)retrace_glMatrixIndexPointerOES},
    {"glWeightPointerOES", (void*)retrace_glWeightPointerOES},
    {"glQueryMatrixxOES", (void*)retrace_glQueryMatrixxOES},
    {"glTexGenfOES", (void*)retrace_glTexGenfOES},
    {"glTexGenfvOES", (void*)retrace_glTexGenfvOES},
    {"glTexGeniOES", (void*)retrace_glTexGeniOES},
    {"glTexGenivOES", (void*)retrace_glTexGenivOES},
    {"glTexGenxOES", (void*)retrace_glTexGenxOES},
    {"glTexGenxvOES", (void*)retrace_glTexGenxvOES},
    {"glGetTexGenfvOES", (void*)ignore},
    {"glGetTexGenivOES", (void*)ignore},
    {"glGetTexGenxvOES", (void*)ignore},
    {"glGetBufferPointervOES", (void*)ignore},
    {"glMapBufferOES", (void*)retrace_glMapBufferOES},
    {"glUnmapBufferOES", (void*)retrace_glUnmapBufferOES},
    {"glTexImage3DOES", (void*)retrace_glTexImage3DOES},
    {"glTexSubImage3DOES", (void*)retrace_glTexSubImage3DOES},
    {"glCopyTexSubImage3DOES", (void*)retrace_glCopyTexSubImage3DOES},
    {"glCompressedTexImage3DOES", (void*)retrace_glCompressedTexImage3DOES},
    {"glCompressedTexSubImage3DOES", (void*)retrace_glCompressedTexSubImage3DOES},
    {"glFramebufferTexture3DOES", (void*)retrace_glFramebufferTexture3DOES},
    {"glGetProgramBinaryOES", (void*)ignore},
    {"glProgramBinaryOES", (void*)retrace_glProgramBinaryOES},
    {"glDiscardFramebufferEXT", (void*)retrace_glDiscardFramebufferEXT},
    {"glBindVertexArrayOES", (void*)retrace_glBindVertexArrayOES},
    {"glDeleteVertexArraysOES", (void*)retrace_glDeleteVertexArraysOES},
    {"glGenVertexArraysOES", (void*)retrace_glGenVertexArraysOES},
    {"glIsVertexArrayOES", (void*)ignore},
    {"glCoverageMaskNV", (void*)retrace_glCoverageMaskNV},
    {"glCoverageOperationNV", (void*)retrace_glCoverageOperationNV},
    {"glRenderbufferStorageMultisampleEXT", (void*)retrace_glRenderbufferStorageMultisampleEXT},
    {"glFramebufferTexture2DMultisampleEXT", (void*)retrace_glFramebufferTexture2DMultisampleEXT},
    {"glRenderbufferStorageMultisampleIMG", (void*)retrace_glRenderbufferStorageMultisampleIMG},
    {"glFramebufferTexture2DMultisampleIMG", (void*)retrace_glFramebufferTexture2DMultisampleIMG},
    {"glRenderbufferStorageMultisampleAPPLE", (void*)retrace_glRenderbufferStorageMultisampleAPPLE},
    {"glResolveMultisampleFramebufferAPPLE", (void*)retrace_glResolveMultisampleFramebufferAPPLE},
    {"glBlitFramebufferANGLE", (void*)retrace_glBlitFramebufferANGLE},
    {"glRenderbufferStorageMultisampleANGLE", (void*)retrace_glRenderbufferStorageMultisampleANGLE},
    {"glDrawBuffersNV", (void*)retrace_glDrawBuffersNV},
    {"glReadBufferNV", (void*)retrace_glReadBufferNV},
    {"glLabelObjectEXT", (void*)retrace_glLabelObjectEXT},
    {"glGetObjectLabelEXT", (void*)ignore},
    {"glInsertEventMarkerEXT", (void*)retrace_glInsertEventMarkerEXT},
    {"glPushGroupMarkerEXT", (void*)retrace_glPushGroupMarkerEXT},
    {"glPopGroupMarkerEXT", (void*)retrace_glPopGroupMarkerEXT},
    {"glGenQueriesEXT", (void*)retrace_glGenQueriesEXT},
    {"glDeleteQueriesEXT", (void*)retrace_glDeleteQueriesEXT},
    {"glIsQueryEXT", (void*)ignore},
    {"glBeginQueryEXT", (void*)retrace_glBeginQueryEXT},
    {"glEndQueryEXT", (void*)retrace_glEndQueryEXT},
    {"glGetQueryivEXT", (void*)ignore},
    {"glGetQueryObjectuivEXT", (void*)ignore},
    {"glUseProgramStagesEXT", (void*)retrace_glUseProgramStagesEXT},
    {"glActiveShaderProgramEXT", (void*)retrace_glActiveShaderProgramEXT},
    {"glCreateShaderProgramvEXT", (void*)retrace_glCreateShaderProgramvEXT},
    {"glBindProgramPipelineEXT", (void*)retrace_glBindProgramPipelineEXT},
    {"glDeleteProgramPipelinesEXT", (void*)retrace_glDeleteProgramPipelinesEXT},
    {"glGenProgramPipelinesEXT", (void*)retrace_glGenProgramPipelinesEXT},
    {"glIsProgramPipelineEXT", (void*)ignore},
    {"glGetProgramPipelineivEXT", (void*)ignore},
    {"glValidateProgramPipelineEXT", (void*)retrace_glValidateProgramPipelineEXT},
    {"glGetProgramPipelineInfoLogEXT", (void*)ignore},
    {"glDrawTexiOES", (void*)retrace_glDrawTexiOES},
    {"glBlendBarrierKHR", (void*)retrace_glBlendBarrierKHR},
    {"glBlendBarrierNV", (void*)retrace_glBlendBarrierNV},
    {"glCreateClientSideBuffer", (void*)retrace_glCreateClientSideBuffer},
    {"glDeleteClientSideBuffer", (void*)retrace_glDeleteClientSideBuffer},
    {"glClientSideBufferData", (void*)retrace_glClientSideBufferData},
    {"glClientSideBufferSubData", (void*)retrace_glClientSideBufferSubData},
    {"glCopyClientSideBuffer", (void*)retrace_glCopyClientSideBuffer},
    {"glPatchClientSideBuffer", (void*)retrace_glPatchClientSideBuffer},
    {"paMandatoryExtensions", (void*)retrace_paMandatoryExtensions},
    {"glAssertBuffer_ARM", (void*)retrace_glAssertBuffer_ARM},
    {"glStateDump_ARM", (void*)retrace_glStateDump_ARM},
    {"glDebugMessageCallback", (void*)ignore},
    {"glDebugMessageControl", (void*)retrace_glDebugMessageControl},
    {"glDebugMessageInsert", (void*)retrace_glDebugMessageInsert},
    {"glPushDebugGroup", (void*)retrace_glPushDebugGroup},
    {"glPopDebugGroup", (void*)retrace_glPopDebugGroup},
    {"glObjectLabel", (void*)retrace_glObjectLabel},
    {"glGetObjectLabel", (void*)ignore},
    {"glObjectPtrLabel", (void*)retrace_glObjectPtrLabel},
    {"glGetDebugMessageLog", (void*)ignore},
    {"glDebugMessageCallbackKHR", (void*)ignore},
    {"glDebugMessageControlKHR", (void*)retrace_glDebugMessageControlKHR},
    {"glDebugMessageInsertKHR", (void*)retrace_glDebugMessageInsertKHR},
    {"glPushDebugGroupKHR", (void*)retrace_glPushDebugGroupKHR},
    {"glPopDebugGroupKHR", (void*)retrace_glPopDebugGroupKHR},
    {"glObjectLabelKHR", (void*)retrace_glObjectLabelKHR},
    {"glGetObjectLabelKHR", (void*)ignore},
    {"glObjectPtrLabelKHR", (void*)retrace_glObjectPtrLabelKHR},
    {"glGetDebugMessageLogKHR", (void*)ignore},
    {"glMinSampleShadingOES", (void*)retrace_glMinSampleShadingOES},
    {"glTexStorage3DMultisampleOES", (void*)retrace_glTexStorage3DMultisampleOES},
    {"glEnableiEXT", (void*)retrace_glEnableiEXT},
    {"glDisableiEXT", (void*)retrace_glDisableiEXT},
    {"glIsEnablediEXT", (void*)ignore},
    {"glBlendEquationiEXT", (void*)retrace_glBlendEquationiEXT},
    {"glBlendEquationSeparateiEXT", (void*)retrace_glBlendEquationSeparateiEXT},
    {"glBlendFunciEXT", (void*)retrace_glBlendFunciEXT},
    {"glBlendFuncSeparateiEXT", (void*)retrace_glBlendFuncSeparateiEXT},
    {"glColorMaskiEXT", (void*)retrace_glColorMaskiEXT},
    {"glEnableiOES", (void*)retrace_glEnableiOES},
    {"glDisableiOES", (void*)retrace_glDisableiOES},
    {"glIsEnablediOES", (void*)ignore},
    {"glBlendEquationiOES", (void*)retrace_glBlendEquationiOES},
    {"glBlendEquationSeparateiOES", (void*)retrace_glBlendEquationSeparateiOES},
    {"glBlendFunciOES", (void*)retrace_glBlendFunciOES},
    {"glBlendFuncSeparateiOES", (void*)retrace_glBlendFuncSeparateiOES},
    {"glColorMaskiOES", (void*)retrace_glColorMaskiOES},
    {"glViewportArrayvOES", (void*)retrace_glViewportArrayvOES},
    {"glViewportIndexedfOES", (void*)retrace_glViewportIndexedfOES},
    {"glViewportIndexedfvOES", (void*)retrace_glViewportIndexedfvOES},
    {"glScissorArrayvOES", (void*)retrace_glScissorArrayvOES},
    {"glScissorIndexedOES", (void*)retrace_glScissorIndexedOES},
    {"glScissorIndexedvOES", (void*)retrace_glScissorIndexedvOES},
    {"glDepthRangeArrayfvOES", (void*)retrace_glDepthRangeArrayfvOES},
    {"glDepthRangeIndexedfOES", (void*)retrace_glDepthRangeIndexedfOES},
    {"glGetFloati_vOES", (void*)ignore},
    {"glFramebufferTextureEXT", (void*)retrace_glFramebufferTextureEXT},
    {"glPrimitiveBoundingBoxEXT", (void*)retrace_glPrimitiveBoundingBoxEXT},
    {"glPatchParameteriEXT", (void*)retrace_glPatchParameteriEXT},
    {"glTexParameterIivEXT", (void*)retrace_glTexParameterIivEXT},
    {"glTexParameterIuivEXT", (void*)retrace_glTexParameterIuivEXT},
    {"glGetTexParameterIivEXT", (void*)ignore},
    {"glGetTexParameterIuivEXT", (void*)ignore},
    {"glSamplerParameterIivEXT", (void*)retrace_glSamplerParameterIivEXT},
    {"glSamplerParameterIuivEXT", (void*)retrace_glSamplerParameterIuivEXT},
    {"glGetSamplerParameterIivEXT", (void*)ignore},
    {"glGetSamplerParameterIuivEXT", (void*)ignore},
    {"glTexBufferEXT", (void*)retrace_glTexBufferEXT},
    {"glTexBufferRangeEXT", (void*)retrace_glTexBufferRangeEXT},
    {"glCopyImageSubDataEXT", (void*)retrace_glCopyImageSubDataEXT},
    {"glAlphaFuncQCOM", (void*)retrace_glAlphaFuncQCOM},
    {"glQueryCounterEXT", (void*)retrace_glQueryCounterEXT},
    {"glGetQueryObjectivEXT", (void*)ignore},
    {"glGetQueryObjecti64vEXT", (void*)ignore},
    {"glGetQueryObjectui64vEXT", (void*)ignore},
    {"glGetPerfMonitorGroupsAMD", (void*)ignore},
    {"glGetPerfMonitorCountersAMD", (void*)ignore},
    {"glGetPerfMonitorGroupStringAMD", (void*)ignore},
    {"glGetPerfMonitorCounterStringAMD", (void*)ignore},
    {"glGenPerfMonitorsAMD", (void*)retrace_glGenPerfMonitorsAMD},
    {"glDeletePerfMonitorsAMD", (void*)retrace_glDeletePerfMonitorsAMD},
    {"glSelectPerfMonitorCountersAMD", (void*)retrace_glSelectPerfMonitorCountersAMD},
    {"glBeginPerfMonitorAMD", (void*)retrace_glBeginPerfMonitorAMD},
    {"glEndPerfMonitorAMD", (void*)retrace_glEndPerfMonitorAMD},
    {"glGetPerfMonitorCounterDataAMD", (void*)ignore},
    {"glBeginPerfQueryINTEL", (void*)ignore},
    {"glCreatePerfQueryINTEL", (void*)ignore},
    {"glDeletePerfQueryINTEL", (void*)ignore},
    {"glEndPerfQueryINTEL", (void*)ignore},
    {"glGetFirstPerfQueryIdINTEL", (void*)ignore},
    {"glGetNextPerfQueryIdINTEL", (void*)ignore},
    {"glGetPerfCounterInfoINTEL", (void*)ignore},
    {"glGetPerfQueryDataINTEL", (void*)ignore},
    {"glGetPerfQueryIdByNameINTEL", (void*)ignore},
    {"glGetPerfQueryInfoINTEL", (void*)ignore},
    {"glGetGraphicsResetStatus", (void*)ignore},
    {"glReadnPixels", (void*)retrace_glReadnPixels},
    {"glGetnUniformfv", (void*)ignore},
    {"glGetnUniformiv", (void*)ignore},
    {"glGetnUniformuiv", (void*)ignore},
    {"glTexStorage3DMultisample", (void*)retrace_glTexStorage3DMultisample},
    {"glEnablei", (void*)retrace_glEnablei},
    {"glDisablei", (void*)retrace_glDisablei},
    {"glIsEnabledi", (void*)ignore},
    {"glBlendEquationi", (void*)retrace_glBlendEquationi},
    {"glBlendEquationSeparatei", (void*)retrace_glBlendEquationSeparatei},
    {"glBlendFunci", (void*)retrace_glBlendFunci},
    {"glBlendFuncSeparatei", (void*)retrace_glBlendFuncSeparatei},
    {"glColorMaski", (void*)retrace_glColorMaski},
    {"glFramebufferTexture", (void*)retrace_glFramebufferTexture},
    {"glPrimitiveBoundingBox", (void*)retrace_glPrimitiveBoundingBox},
    {"glPatchParameteri", (void*)retrace_glPatchParameteri},
    {"glTexParameterIiv", (void*)retrace_glTexParameterIiv},
    {"glTexParameterIuiv", (void*)retrace_glTexParameterIuiv},
    {"glGetTexParameterIiv", (void*)ignore},
    {"glGetTexParameterIuiv", (void*)ignore},
    {"glSamplerParameterIiv", (void*)retrace_glSamplerParameterIiv},
    {"glSamplerParameterIuiv", (void*)retrace_glSamplerParameterIuiv},
    {"glGetSamplerParameterIiv", (void*)ignore},
    {"glGetSamplerParameterIuiv", (void*)ignore},
    {"glTexBuffer", (void*)retrace_glTexBuffer},
    {"glTexBufferRange", (void*)retrace_glTexBufferRange},
    {"glCopyImageSubData", (void*)retrace_glCopyImageSubData},
    {"glDrawElementsBaseVertex", (void*)retrace_glDrawElementsBaseVertex},
    {"glDrawRangeElementsBaseVertex", (void*)retrace_glDrawRangeElementsBaseVertex},
    {"glDrawElementsInstancedBaseVertex", (void*)retrace_glDrawElementsInstancedBaseVertex},
    {"glBlendBarrier", (void*)retrace_glBlendBarrier},
    {"glMinSampleShading", (void*)retrace_glMinSampleShading},
    {"glTexStorage1DEXT", (void*)retrace_glTexStorage1DEXT},
    {"glTexStorage2DEXT", (void*)retrace_glTexStorage2DEXT},
    {"glTexStorage3DEXT", (void*)retrace_glTexStorage3DEXT},
    {"glPatchParameteriOES", (void*)retrace_glPatchParameteriOES},
    {"glFramebufferTextureMultiviewOVR", (void*)retrace_glFramebufferTextureMultiviewOVR},
    {"glFramebufferTextureMultisampleMultiviewOVR", (void*)retrace_glFramebufferTextureMultisampleMultiviewOVR},
    {"glClearTexImageEXT", (void*)retrace_glClearTexImageEXT},
    {"glClearTexSubImageEXT", (void*)retrace_glClearTexSubImageEXT},
    {"glBufferStorageEXT", (void*)retrace_glBufferStorageEXT},
    {"glDrawTransformFeedbackEXT", (void*)retrace_glDrawTransformFeedbackEXT},
    {"glDrawTransformFeedbackInstancedEXT", (void*)retrace_glDrawTransformFeedbackInstancedEXT},
    {"glActiveShaderProgram", (void*)retrace_glActiveShaderProgram},
    {"glBeginQuery", (void*)retrace_glBeginQuery},
    {"glBeginTransformFeedback", (void*)retrace_glBeginTransformFeedback},
    {"glBindBufferBase", (void*)retrace_glBindBufferBase},
    {"glBindBufferRange", (void*)retrace_glBindBufferRange},
    {"glBindImageTexture", (void*)retrace_glBindImageTexture},
    {"glBindProgramPipeline", (void*)retrace_glBindProgramPipeline},
    {"glBindSampler", (void*)retrace_glBindSampler},
    {"glBindTransformFeedback", (void*)retrace_glBindTransformFeedback},
    {"glBindVertexArray", (void*)retrace_glBindVertexArray},
    {"glBindVertexBuffer", (void*)retrace_glBindVertexBuffer},
    {"glBlitFramebuffer", (void*)retrace_glBlitFramebuffer},
    {"glClearBufferfi", (void*)retrace_glClearBufferfi},
    {"glClearBufferfv", (void*)retrace_glClearBufferfv},
    {"glClearBufferiv", (void*)retrace_glClearBufferiv},
    {"glClearBufferuiv", (void*)retrace_glClearBufferuiv},
    {"glClientWaitSync", (void*)retrace_glClientWaitSync},
    {"glCompressedTexImage3D", (void*)retrace_glCompressedTexImage3D},
    {"glCompressedTexSubImage3D", (void*)retrace_glCompressedTexSubImage3D},
    {"glCopyBufferSubData", (void*)retrace_glCopyBufferSubData},
    {"glCopyTexSubImage3D", (void*)retrace_glCopyTexSubImage3D},
    {"glCreateShaderProgramv", (void*)retrace_glCreateShaderProgramv},
    {"glDeleteProgramPipelines", (void*)retrace_glDeleteProgramPipelines},
    {"glDeleteQueries", (void*)retrace_glDeleteQueries},
    {"glDeleteSamplers", (void*)retrace_glDeleteSamplers},
    {"glDeleteSync", (void*)retrace_glDeleteSync},
    {"glDeleteTransformFeedbacks", (void*)retrace_glDeleteTransformFeedbacks},
    {"glDeleteVertexArrays", (void*)retrace_glDeleteVertexArrays},
    {"glDispatchCompute", (void*)retrace_glDispatchCompute},
    {"glDispatchComputeIndirect", (void*)retrace_glDispatchComputeIndirect},
    {"glDrawArraysIndirect", (void*)retrace_glDrawArraysIndirect},
    {"glDrawArraysInstanced", (void*)retrace_glDrawArraysInstanced},
    {"glDrawBuffers", (void*)retrace_glDrawBuffers},
    {"glDrawElementsIndirect", (void*)retrace_glDrawElementsIndirect},
    {"glDrawElementsInstanced", (void*)retrace_glDrawElementsInstanced},
    {"glDrawRangeElements", (void*)retrace_glDrawRangeElements},
    {"glEndQuery", (void*)retrace_glEndQuery},
    {"glEndTransformFeedback", (void*)retrace_glEndTransformFeedback},
    {"glFenceSync", (void*)retrace_glFenceSync},
    {"glFlushMappedBufferRange", (void*)retrace_glFlushMappedBufferRange},
    {"glFramebufferParameteri", (void*)retrace_glFramebufferParameteri},
    {"glFramebufferTextureLayer", (void*)retrace_glFramebufferTextureLayer},
    {"glGenProgramPipelines", (void*)retrace_glGenProgramPipelines},
    {"glGenQueries", (void*)retrace_glGenQueries},
    {"glGenSamplers", (void*)retrace_glGenSamplers},
    {"glGenTransformFeedbacks", (void*)retrace_glGenTransformFeedbacks},
    {"glGenVertexArrays", (void*)retrace_glGenVertexArrays},
    {"glGetActiveUniformBlockName", (void*)ignore},
    {"glGetActiveUniformBlockiv", (void*)ignore},
    {"glGetActiveUniformsiv", (void*)ignore},
    {"glGetBooleani_v", (void*)ignore},
    {"glGetBufferParameteri64v", (void*)ignore},
    {"glGetBufferPointerv", (void*)ignore},
    {"glGetFragDataLocation", (void*)ignore},
    {"glGetFramebufferParameteriv", (void*)ignore},
    {"glGetInteger64i_v", (void*)ignore},
    {"glGetInteger64v", (void*)ignore},
    {"glGetIntegeri_v", (void*)ignore},
    {"glGetInternalformativ", (void*)ignore},
    {"glGetMultisamplefv", (void*)ignore},
    {"glGetProgramBinary", (void*)ignore},
    {"glGetProgramInterfaceiv", (void*)ignore},
    {"glGetProgramPipelineInfoLog", (void*)ignore},
    {"glGetProgramPipelineiv", (void*)ignore},
    {"glGetProgramResourceIndex", (void*)ignore},
    {"glGetProgramResourceLocation", (void*)ignore},
    {"glGetProgramResourceName", (void*)ignore},
    {"glGetProgramResourceiv", (void*)ignore},
    {"glGetQueryObjectuiv", (void*)ignore},
    {"glGetQueryiv", (void*)ignore},
    {"glGetSamplerParameterfv", (void*)ignore},
    {"glGetSamplerParameteriv", (void*)ignore},
    {"glGetStringi", (void*)ignore},
    {"glGetSynciv", (void*)ignore},
    {"glGetTexLevelParameterfv", (void*)ignore},
    {"glGetTexLevelParameteriv", (void*)ignore},
    {"glGetTransformFeedbackVarying", (void*)ignore},
    {"glGetUniformBlockIndex", (void*)retrace_glGetUniformBlockIndex},
    {"glGetUniformIndices", (void*)ignore},
    {"glGetUniformuiv", (void*)ignore},
    {"glGetVertexAttribIiv", (void*)ignore},
    {"glGetVertexAttribIuiv", (void*)ignore},
    {"glInvalidateFramebuffer", (void*)retrace_glInvalidateFramebuffer},
    {"glInvalidateSubFramebuffer", (void*)retrace_glInvalidateSubFramebuffer},
    {"glIsProgramPipeline", (void*)ignore},
    {"glIsQuery", (void*)ignore},
    {"glIsSampler", (void*)ignore},
    {"glIsSync", (void*)ignore},
    {"glIsTransformFeedback", (void*)ignore},
    {"glIsVertexArray", (void*)ignore},
    {"glMapBufferRange", (void*)retrace_glMapBufferRange},
    {"glMemoryBarrier", (void*)retrace_glMemoryBarrier},
    {"glMemoryBarrierByRegion", (void*)retrace_glMemoryBarrierByRegion},
    {"glPauseTransformFeedback", (void*)retrace_glPauseTransformFeedback},
    {"glProgramBinary", (void*)retrace_glProgramBinary},
    {"glProgramParameteri", (void*)retrace_glProgramParameteri},
    {"glProgramUniform1f", (void*)retrace_glProgramUniform1f},
    {"glProgramUniform1fv", (void*)retrace_glProgramUniform1fv},
    {"glProgramUniform1i", (void*)retrace_glProgramUniform1i},
    {"glProgramUniform1iv", (void*)retrace_glProgramUniform1iv},
    {"glProgramUniform1ui", (void*)retrace_glProgramUniform1ui},
    {"glProgramUniform1uiv", (void*)retrace_glProgramUniform1uiv},
    {"glProgramUniform2f", (void*)retrace_glProgramUniform2f},
    {"glProgramUniform2fv", (void*)retrace_glProgramUniform2fv},
    {"glProgramUniform2i", (void*)retrace_glProgramUniform2i},
    {"glProgramUniform2iv", (void*)retrace_glProgramUniform2iv},
    {"glProgramUniform2ui", (void*)retrace_glProgramUniform2ui},
    {"glProgramUniform2uiv", (void*)retrace_glProgramUniform2uiv},
    {"glProgramUniform3f", (void*)retrace_glProgramUniform3f},
    {"glProgramUniform3fv", (void*)retrace_glProgramUniform3fv},
    {"glProgramUniform3i", (void*)retrace_glProgramUniform3i},
    {"glProgramUniform3iv", (void*)retrace_glProgramUniform3iv},
    {"glProgramUniform3ui", (void*)retrace_glProgramUniform3ui},
    {"glProgramUniform3uiv", (void*)retrace_glProgramUniform3uiv},
    {"glProgramUniform4f", (void*)retrace_glProgramUniform4f},
    {"glProgramUniform4fv", (void*)retrace_glProgramUniform4fv},
    {"glProgramUniform4i", (void*)retrace_glProgramUniform4i},
    {"glProgramUniform4iv", (void*)retrace_glProgramUniform4iv},
    {"glProgramUniform4ui", (void*)retrace_glProgramUniform4ui},
    {"glProgramUniform4uiv", (void*)retrace_glProgramUniform4uiv},
    {"glProgramUniformMatrix2fv", (void*)retrace_glProgramUniformMatrix2fv},
    {"glProgramUniformMatrix2x3fv", (void*)retrace_glProgramUniformMatrix2x3fv},
    {"glProgramUniformMatrix2x4fv", (void*)retrace_glProgramUniformMatrix2x4fv},
    {"glProgramUniformMatrix3fv", (void*)retrace_glProgramUniformMatrix3fv},
    {"glProgramUniformMatrix3x2fv", (void*)retrace_glProgramUniformMatrix3x2fv},
    {"glProgramUniformMatrix3x4fv", (void*)retrace_glProgramUniformMatrix3x4fv},
    {"glProgramUniformMatrix4fv", (void*)retrace_glProgramUniformMatrix4fv},
    {"glProgramUniformMatrix4x2fv", (void*)retrace_glProgramUniformMatrix4x2fv},
    {"glProgramUniformMatrix4x3fv", (void*)retrace_glProgramUniformMatrix4x3fv},
    {"glReadBuffer", (void*)retrace_glReadBuffer},
    {"glRenderbufferStorageMultisample", (void*)retrace_glRenderbufferStorageMultisample},
    {"glResumeTransformFeedback", (void*)retrace_glResumeTransformFeedback},
    {"glSampleMaski", (void*)retrace_glSampleMaski},
    {"glSamplerParameterf", (void*)retrace_glSamplerParameterf},
    {"glSamplerParameterfv", (void*)retrace_glSamplerParameterfv},
    {"glSamplerParameteri", (void*)retrace_glSamplerParameteri},
    {"glSamplerParameteriv", (void*)retrace_glSamplerParameteriv},
    {"glTexImage3D", (void*)retrace_glTexImage3D},
    {"glTexStorage2D", (void*)retrace_glTexStorage2D},
    {"glTexStorage2DMultisample", (void*)retrace_glTexStorage2DMultisample},
    {"glTexStorage3D", (void*)retrace_glTexStorage3D},
    {"glTexSubImage3D", (void*)retrace_glTexSubImage3D},
    {"glTransformFeedbackVaryings", (void*)retrace_glTransformFeedbackVaryings},
    {"glUniform1ui", (void*)retrace_glUniform1ui},
    {"glUniform1uiv", (void*)retrace_glUniform1uiv},
    {"glUniform2ui", (void*)retrace_glUniform2ui},
    {"glUniform2uiv", (void*)retrace_glUniform2uiv},
    {"glUniform3ui", (void*)retrace_glUniform3ui},
    {"glUniform3uiv", (void*)retrace_glUniform3uiv},
    {"glUniform4ui", (void*)retrace_glUniform4ui},
    {"glUniform4uiv", (void*)retrace_glUniform4uiv},
    {"glUniformBlockBinding", (void*)retrace_glUniformBlockBinding},
    {"glUniformMatrix2x3fv", (void*)retrace_glUniformMatrix2x3fv},
    {"glUniformMatrix2x4fv", (void*)retrace_glUniformMatrix2x4fv},
    {"glUniformMatrix3x2fv", (void*)retrace_glUniformMatrix3x2fv},
    {"glUniformMatrix3x4fv", (void*)retrace_glUniformMatrix3x4fv},
    {"glUniformMatrix4x2fv", (void*)retrace_glUniformMatrix4x2fv},
    {"glUniformMatrix4x3fv", (void*)retrace_glUniformMatrix4x3fv},
    {"glUnmapBuffer", (void*)retrace_glUnmapBuffer},
    {"glUseProgramStages", (void*)retrace_glUseProgramStages},
    {"glValidateProgramPipeline", (void*)retrace_glValidateProgramPipeline},
    {"glVertexAttribBinding", (void*)retrace_glVertexAttribBinding},
    {"glVertexAttribDivisor", (void*)retrace_glVertexAttribDivisor},
    {"glVertexAttribFormat", (void*)retrace_glVertexAttribFormat},
    {"glVertexAttribI4i", (void*)retrace_glVertexAttribI4i},
    {"glVertexAttribI4iv", (void*)retrace_glVertexAttribI4iv},
    {"glVertexAttribI4ui", (void*)retrace_glVertexAttribI4ui},
    {"glVertexAttribI4uiv", (void*)retrace_glVertexAttribI4uiv},
    {"glVertexAttribIFormat", (void*)retrace_glVertexAttribIFormat},
    {"glVertexAttribIPointer", (void*)retrace_glVertexAttribIPointer},
    {"glVertexBindingDivisor", (void*)retrace_glVertexBindingDivisor},
    {"glWaitSync", (void*)retrace_glWaitSync},
};

