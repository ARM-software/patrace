#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <unistd.h>
#include <ctime>

#include "common/out_file.hpp"
#include "common/image.hpp"
#include "common/trace_model.hpp"

#include "dispatch/eglproc_auto.hpp"
#include "helper/eglsize.hpp"
#include "helper/shaderutility.hpp"
#include "helper/depth_dumper.hpp"

#include "json/reader.h"
#include "json/writer.h"

#include "tool/utils.hpp"

#include <retracer/config.hpp>
#include <retracer/glws.hpp>
#include <retracer/retrace_api.hpp>
#include <retracer/retracer.hpp>
#include <retracer/state.hpp>

// Enable this to print enum strings in debug messages
#define FF_ENUMSTRING 0
#if FF_ENUMSTRING
    // Also need to add "common_eglstate" to target_link_libraries
    #include "eglstate/common.hpp"
#else
    #define EnumString(x) "<Missing EnumString>"
#endif

// Needed for glGetTexLevelParameteriv and related enums
#include "../../thirdparty/opengl-registry/api/GLES3/gl31.h"
#include "../../thirdparty/opengl-registry/api/GLES2/gl2ext.h"


// Need to fetch the function ourselves to check if it is available
typedef void (GLES_CALLCONVENTION * PFN_GLGETTEXLEVELPARAMETERIV)(GLenum target, GLint level, GLenum pname, GLint *params);
static PFN_GLGETTEXLEVELPARAMETERIV ff_glGetTexLevelParameteriv;

enum FastForwardOptionFlags
{
    FASTFORWARD_RESTORE_TEXTURES  = 1 << 0,
    FASTFORWARD_RESTORE_DEFAUTL_FBO= 1 << 1,
};

struct FastForwardOptions
{
    std::string mOutputFileName;
    std::string mComment;
    unsigned int mTargetFrame;
    unsigned int mTargetDrawCallNo;
    unsigned int mEndFrame;
    unsigned int mFlags;
    unsigned int mFbo0Repeat;

    FastForwardOptions()
        : mOutputFileName("fastforward.pat")
        , mComment("")
        , mTargetFrame(0)
        , mTargetDrawCallNo(0)
        , mEndFrame(UINT32_MAX)
        , mFlags(FASTFORWARD_RESTORE_TEXTURES)
        , mFbo0Repeat(0)
    {}
};

namespace RetraceAndTrim
{
class ScratchBuffer
{
public:
    ScratchBuffer(size_t initialCapacity = 0)
        : mVector()
    {
        resizeToFit(initialCapacity);
    }

    // Only use the returned ptr after making sure the buffer is large enough
    char* bufferPtr()
    {
        return mVector.data();
    }

    // Pointers returned from bufferPtr() previously
    // are invalid after calling this!
    void resizeToFit(size_t len)
    {
        mVector.reserve(len);
    }

private:
    // Noncopyable
    ScratchBuffer(const ScratchBuffer&);
    ScratchBuffer& operator=(const ScratchBuffer&);
    std::vector<char> mVector;
};

// NOTE: This emits commands in the _V4_ trace file format!
class TraceCommandEmitter
{
public:
    enum TexDimension{
        Tex2D,
        Tex3D
    };
    TraceCommandEmitter(common::OutFile& outFile, int threadId)
        : mScratchBuff(0)
        , mOutFile(outFile)
        , mThreadId(threadId)
        // NOTE: getId checks that the ids are valid, and aborts if not.
        , mGlGenBuffersId(getId("glGenBuffers"))
        , mGlDeleteBuffersId(getId("glDeleteBuffers"))
        , mGlBufferDataId(getId("glBufferData"))
        , mGlBindBufferId(getId("glBindBuffer"))
        , mGlMapBufferRangeId(getId("glMapBufferRange"))
        , mGlUnmapBufferId(getId("glUnmapBuffer"))
        , mGlBindVertexArrayId(getId("glBindVertexArray"))
        , mGlVertexAttribPointerId(getId("glVertexAttribPointer"))
        , mGlVertexAttribIPointerId(getId("glVertexAttribIPointer"))
        , mGlEnableVertexAttribArrayId(getId("glEnableVertexAttribArray"))
        , mGlDisableVertexAttribArrayId(getId("glDisableVertexAttribArray"))
        , mGlBindFramebufferId(getId("glBindFramebuffer"))
        , mGlGenTexturesId(getId("glGenTextures"))
        , mGlDeleteTexturesId(getId("glDeleteTextures"))
        , mGlTexImage2DId(getId("glTexImage2D"))
        , mGlTexSubImage2DId(getId("glTexSubImage2D"))
        , mGlTexSubImage3DId(getId("glTexSubImage3D"))
        , mGlActiveTextureId(getId("glActiveTexture"))
        , mGlBindTextureId(getId("glBindTexture"))
        , mGlTexParameteriId(getId("glTexParameteri"))
        , mGlTexParameterfId(getId("glTexParameterf"))
        , mGlPixelStoreiId(getId("glPixelStorei"))
        , mGlCreateShaderId(getId("glCreateShader"))
        , mGlShaderSourceId(getId("glShaderSource"))
        , mGlCompileShaderId(getId("glCompileShader"))
        , mGlCreateProgramId(getId("glCreateProgram"))
        , mGlAttachSahderId(getId("glAttachShader"))
        , mGlLinkProgramId(getId("glLinkProgram"))
        , mGlUseProgramId(getId("glUseProgram"))
        , mGlDeleteShaderId(getId("glDeleteShader"))
        , mGlDeleteProgramId(getId("glDeleteProgram"))
        , mGlBindSamplerId(getId("glBindSampler"))
        , mGlEnable(getId("glEnable"))
        , mGlDisable(getId("glDisable"))
        , mGlClear(getId("glClear"))
        , mGlClearColor(getId("glClearColor"))
        , mGlViewportId(getId("glViewport"))
        , mGlColorMaskId(getId("glColorMask"))
        , mGlDrawElements(getId("glDrawElements"))
        , mGlFrontFace(getId("glFrontFace"))
        , mEglSwapBuffers(getId("eglSwapBuffers"))
    {}

    void emitDisable(GLenum cap)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(int));

        char* const bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;
        dest = writeBCall(dest, mGlDisable);
        dest = common::WriteFixed<int>(dest, cap); // enum

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitEnable(GLenum cap)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(int));

        char* const bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;
        dest = writeBCall(dest, mGlEnable);
        dest = common::WriteFixed<int>(dest, cap); // enum

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitClear(GLbitfield mask)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(int));

        char* const bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;
        dest = writeBCall(dest, mGlClear);
        dest = common::WriteFixed<int>(dest, mask);

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(GLfloat) * 4);

        char* const bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;
        dest = writeBCall(dest, mGlClearColor);
        dest = common::WriteFixed<float>(dest, red);
        dest = common::WriteFixed<float>(dest, green);
        dest = common::WriteFixed<float>(dest, blue);
        dest = common::WriteFixed<float>(dest, alpha);

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall_vlen) + sizeof(int) * 3 + size + 32);

        char* const bufStart = mScratchBuff.bufferPtr();
        char* tmpBuf = bufStart;

        // Make room for BCall_vlen at start of buffer
        tmpBuf += sizeof(common::BCall_vlen);

        tmpBuf = common::WriteFixed<int>(tmpBuf, target); // enum
        tmpBuf = common::WriteFixed<int>(tmpBuf, size); // literal
        tmpBuf = common::Write1DArray<char>(tmpBuf, (unsigned int)size, (const char*)data); // blob
        tmpBuf = common::WriteFixed<int>(tmpBuf, usage); // enum

        // Write BCall_vlen to bufStart
        int toNext = tmpBuf - bufStart;
        writeBCall_vlen(bufStart, mGlBufferDataId, toNext);

        mOutFile.Write(bufStart, toNext);
    }

    void emitTexSubImage(TexDimension dimension, GLenum target, GLint level,
                         GLint xoffset, GLint yoffset, GLint zoffset,
                         GLsizei width, GLsizei height, GLsizei depth,
                         GLenum format, GLenum type, unsigned int textureSize, const char* data)
    {
        if (dimension == Tex2D)
            mScratchBuff.resizeToFit(sizeof(common::BCall_vlen) + sizeof(int) * 9 + textureSize + 32);
        else if (dimension == Tex3D)
            mScratchBuff.resizeToFit(sizeof(common::BCall_vlen) + sizeof(int) * 11 + textureSize + 32);

        char* const bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;

        // Written last (need to know toNext)
        dest += sizeof(common::BCall_vlen);

        dest = common::WriteFixed<int>(dest, target); // enum target
        dest = common::WriteFixed<int>(dest, level); // literal level
        dest = common::WriteFixed<int>(dest, xoffset); // literal xoffset
        dest = common::WriteFixed<int>(dest, yoffset); // literal yoffset
        if (dimension == Tex3D)
            dest = common::WriteFixed<int>(dest, zoffset); // literal zoffset
        dest = common::WriteFixed<unsigned int>(dest, width); // literal width
        dest = common::WriteFixed<unsigned int>(dest, height); // literal height
        if (dimension == Tex3D)
            dest = common::WriteFixed<unsigned int>(dest, depth); // literal depth
        dest = common::WriteFixed<int>(dest, format); // enum format
        dest = common::WriteFixed<int>(dest, type); // enum type
        dest = common::WriteFixed<unsigned int>(dest, common::BlobType);
        dest = common::Write1DArray<char>(dest, textureSize, data);

        // NOTE: written to bufStart, not dest
        int toNext = dest - bufStart;
        if (dimension == Tex2D)
            writeBCall_vlen(bufStart, mGlTexSubImage2DId, toNext);
        else if (dimension == Tex3D)
            writeBCall_vlen(bufStart, mGlTexSubImage3DId, toNext);

        mOutFile.Write(bufStart, toNext);
    }

    void emitPixelStorei(GLenum pname, GLint param)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(int) * 2);

        char* const bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;
        dest = writeBCall(dest, mGlPixelStoreiId);
        dest = common::WriteFixed<int>(dest, pname); // enum
        dest = common::WriteFixed<unsigned int>(dest, param); // literal

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitTexParameteri(unsigned int target, GLenum pname, unsigned int param)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(int) * 3 + 32);

        char* bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;
        dest = writeBCall(dest, mGlTexParameteriId);
        dest = common::WriteFixed<int>(dest, target); // enum
        dest = common::WriteFixed<int>(dest, pname); // enum
        dest = common::WriteFixed<int>(dest, param); // literal

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitTexParameterf(unsigned int target, GLenum pname, float param)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(int) * 3);

        char* bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;
        dest = writeBCall(dest, mGlTexParameterfId);
        dest = common::WriteFixed<int>(dest, target); // enum
        dest = common::WriteFixed<int>(dest, pname); // enum
        dest = common::WriteFixed<float>(dest, param); // literal

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitBindTexture(GLenum target, GLuint tex)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(int) * 2);

        char* const bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;
        dest = writeBCall(dest, mGlBindTextureId);
        dest = common::WriteFixed<int>(dest, target); // enum
        dest = common::WriteFixed<unsigned int>(dest, tex); // literal

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitBindFramebuffer(GLenum target, GLint id)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(int) * 2 + 32);

        char* const bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;
        dest = writeBCall(dest, mGlBindFramebufferId);
        dest = common::WriteFixed<int>(dest, (int) target); // enum
        dest = common::WriteFixed<unsigned int>(dest, id); // literal

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitBindBuffer(GLenum target, GLint id)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(int) * 2 + 32);

        char* const bufStart = mScratchBuff.bufferPtr();
        char* dest = bufStart;
        dest = writeBCall(dest, mGlBindBufferId);
        dest = common::WriteFixed<int>(dest, (int) target); // enum
        dest = common::WriteFixed<unsigned int>(dest, id); // literal

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitCreateShader(GLenum type, GLuint shader)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(int) * 2 + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;
        dest = writeBCall(dest, mGlCreateShaderId);
        dest = common::WriteFixed(dest, (int)type);
        dest = common::WriteFixed(dest, shader);

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitShaderSource(GLuint shader, GLsizei count, const GLchar **string, const GLint *length)
    {
        GLsizei size;

        size = sizeof(common::BCall_vlen) + sizeof(GLuint) + sizeof(GLsizei) + 32;
        size += count * sizeof(GLuint) + sizeof(GLuint) + 4;

        if (length != NULL)
        {
            for (int i = 0; i < count; ++i)
            {
                if (length[i])
                {
                    size += length[i] + 5;
                }
            }
            size += sizeof(GLuint) + count * sizeof(GLint) + 4;
        }
        else
        {
            for (int i = 0; i < count; ++i)
            {
                if (string[i])
                {
                    size += strlen(string[i]) + 5;
                }
            }
        }
        mScratchBuff.resizeToFit(size);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest += sizeof(common::BCall_vlen);
        dest = common::WriteFixed<unsigned int>(dest, shader);        // literal
        dest = common::WriteFixed<int>(dest, count);                  // literal
        dest = common::WriteStringArray(dest, count, string);         // string array
        dest = common::Write1DArray<int>(dest, count, (int *)length); // array

        // Write BCall_vlen to bufStart
        int toNext = dest - bufStart;
        writeBCall_vlen(bufStart, mGlShaderSourceId, toNext);

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitCompileShader(GLuint shader)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(GLuint) + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest = writeBCall(dest, mGlCompileShaderId);
        dest = common::WriteFixed<unsigned int>(dest, shader);

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitCreateProgram(GLuint program)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(GLuint) + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest = writeBCall(dest, mGlCreateProgramId);
        dest = common::WriteFixed<unsigned int>(dest, program);

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitAttachShader(GLuint program, GLuint shader)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(GLuint) * 2 + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest = writeBCall(dest, mGlAttachSahderId);
        dest = common::WriteFixed<unsigned int>(dest, program);
        dest = common::WriteFixed<unsigned int>(dest, shader);

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitLinkProgram(GLuint program)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(GLuint) + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest = writeBCall(dest, mGlLinkProgramId);
        dest = common::WriteFixed<unsigned int>(dest, program);

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitUseProgram(GLuint program)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(GLuint) + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest = writeBCall(dest, mGlUseProgramId);
        dest = common::WriteFixed<unsigned int>(dest, program);

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitDeleteShader(GLuint shader)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(GLuint) + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest = writeBCall(dest, mGlDeleteShaderId);
        dest = common::WriteFixed<unsigned int>(dest, shader);

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitDeleteProgram(GLuint program)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(GLuint) + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest = writeBCall(dest, mGlDeleteProgramId);
        dest = common::WriteFixed<unsigned int>(dest, program);

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitActiveTexture(GLenum target)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(GLuint) + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest = writeBCall(dest, mGlActiveTextureId);
        dest = common::WriteFixed<int>(dest, target);

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitGenTextures(GLsizei n, GLuint *textures)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall_vlen) + sizeof(GLuint) * n + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest += sizeof(common::BCall_vlen);
        dest = common::WriteFixed<int>(dest, n);                                      // literal
        dest = common::Write1DArray<unsigned int>(dest, n, (unsigned int *)textures); // array

        // Write BCall_vlen to bufStart
        int toNext = dest - bufStart;
        writeBCall_vlen(bufStart, mGlGenTexturesId, toNext);

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitTexImage2D(GLenum target,
                        GLint level,
                        GLint internalformat,
                        GLsizei width,
                        GLsizei height,
                        GLint border,
                        GLenum format,
                        GLenum type,
                        const GLvoid *pixels,
                        GLsizei size)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall_vlen) + sizeof(GLint) * 10 + size + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        // Written last (need to know toNext)
        dest += sizeof(common::BCall_vlen);

        dest = common::WriteFixed<int>(dest, target);         // enum
        dest = common::WriteFixed<int>(dest, level);          // literal
        dest = common::WriteFixed<int>(dest, internalformat); // enum
        dest = common::WriteFixed<int>(dest, width);          // literal
        dest = common::WriteFixed<int>(dest, height);         // literal
        dest = common::WriteFixed<int>(dest, border);         // literal
        dest = common::WriteFixed<int>(dest, format);         // enum
        dest = common::WriteFixed<int>(dest, type);           // enum
        dest = common::WriteFixed<unsigned int>(dest, common::Opaque_Type_TM::BlobType);
        dest = common::Write1DArray<char>(dest, size, (const char *)pixels);

        // NOTE: written to bufStart, not dest
        int toNext = dest - bufStart;
        writeBCall_vlen(bufStart, mGlTexImage2DId, toNext);

        mOutFile.Write(bufStart, toNext);
    }

    void emitGenBuffers(GLsizei n, GLuint *buffer)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall_vlen) + sizeof(GLuint) * (n + 2) + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest += sizeof(common::BCall_vlen);

        dest = common::WriteFixed<int>(dest, n);
        dest = common::Write1DArray<unsigned int>(dest, n, (unsigned int *)buffer);

        // NOTE: written to bufStart, not dest
        int toNext = dest - bufStart;
        writeBCall_vlen(bufStart, mGlGenBuffersId, toNext);

        mOutFile.Write(bufStart, toNext);
    }

    void emitBindVertexArray(GLuint array)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(GLuint) + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest = writeBCall(dest, mGlBindVertexArrayId);
        dest = common::WriteFixed<unsigned int>(dest, array);

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitVertexAttibPointer(GLuint index,
                                GLint size,
                                GLenum type,
                                GLboolean normalized,
                                GLsizei stride,
                                const GLvoid *pointer)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall_vlen) + sizeof(GLuint) * 6 + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest += sizeof(common::BCall_vlen);
        dest = common::WriteFixed<unsigned int>(dest, index);                                             // literal
        dest = common::WriteFixed<int>(dest, size);                                                       // literal
        dest = common::WriteFixed<int>(dest, type);                                                       // enum
        dest = common::WriteFixed<unsigned char>(dest, normalized);                                       // literal
        dest = common::WriteFixed<int>(dest, stride);                                                     // literal
        dest = common::WriteFixed<unsigned int>(dest, common::Opaque_Type_TM::BufferObjectReferenceType); // IS Simple Memory Offset
        dest = common::WriteFixed<unsigned int>(dest, (uintptr_t)pointer);                                // opaque -> ptr

        // NOTE: written to bufStart, not dest
        int toNext = dest - bufStart;
        writeBCall_vlen(bufStart, mGlVertexAttribPointerId, toNext);

        mOutFile.Write(bufStart, toNext);
    }

    void emitEnableVertexAttribArray(GLuint index)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(GLuint) + 4);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest = writeBCall(dest, mGlEnableVertexAttribArrayId);
        dest = common::WriteFixed<unsigned int>(dest, index);

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitViewport(GLint x, GLint y, GLsizei width, GLsizei height)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(GLint) * 4 + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest = writeBCall(dest, mGlViewportId);
        dest = common::WriteFixed<int>(dest, x);      // literal
        dest = common::WriteFixed<int>(dest, y);      // literal
        dest = common::WriteFixed<int>(dest, width);  // literal
        dest = common::WriteFixed<int>(dest, height); // literal

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(unsigned char) * 4 + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest = writeBCall(dest, mGlColorMaskId);
        dest = common::WriteFixed<unsigned char>(dest, red);   // literal
        dest = common::WriteFixed<unsigned char>(dest, green); // literal
        dest = common::WriteFixed<unsigned char>(dest, blue);  // literal
        dest = common::WriteFixed<unsigned char>(dest, alpha); // literal

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall_vlen) + sizeof(int) * 3 + sizeof(unsigned int) * 2 + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest += sizeof(common::BCall_vlen);
        dest = common::WriteFixed<int>(dest, mode);                                                       // enum
        dest = common::WriteFixed<int>(dest, count);                                                      // literal
        dest = common::WriteFixed<int>(dest, type);                                                       // enum
        dest = common::WriteFixed<unsigned int>(dest, common::Opaque_Type_TM::BufferObjectReferenceType); // ISN'T *BLOB*
        dest = common::WriteFixed<unsigned int>(dest, (uintptr_t)indices);                                // opaque -> ptr

        // NOTE: written to bufStart, not dest
        int toNext = dest - bufStart;
        writeBCall_vlen(bufStart, mGlDrawElements, toNext);

        mOutFile.Write(bufStart, toNext);
    }

    void emitFrontFace(GLenum mode)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(int) + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest = writeBCall(dest, mGlFrontFace);
        dest = common::WriteFixed<int>(dest, mode); // enum

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitDeleteTextures(GLsizei n, const GLuint *textures)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall_vlen) + sizeof(GLuint) * (n + 2) + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest += sizeof(common::BCall_vlen);
        dest = common::WriteFixed<int>(dest, n);                                      // literal
        dest = common::Write1DArray<unsigned int>(dest, n, (unsigned int *)textures); // array

        // NOTE: written to bufStart, not dest
        int toNext = dest - bufStart;
        writeBCall_vlen(bufStart, mGlDeleteTexturesId, toNext);

        mOutFile.Write(bufStart, toNext);
    }

    void emitVertexAttribIPointer(GLuint index,
                                  GLint size,
                                  GLenum type,
                                  GLsizei stride,
                                  const GLvoid *pointer)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall_vlen) + sizeof(int) * 5 + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest += sizeof(common::BCall_vlen);
        dest = common::WriteFixed<unsigned int>(dest, index);                                             // literal
        dest = common::WriteFixed<int>(dest, size);                                                       // literal
        dest = common::WriteFixed<int>(dest, type);                                                       // enum
        dest = common::WriteFixed<int>(dest, stride);                                                     // literal
        dest = common::WriteFixed<unsigned int>(dest, common::Opaque_Type_TM::BufferObjectReferenceType); // IS Simple Memory Offset
        dest = common::WriteFixed<unsigned int>(dest, (uintptr_t)pointer);                                // opaque -> ptr

        // NOTE: written to bufStart, not dest
        int toNext = dest - bufStart;
        writeBCall_vlen(bufStart, mGlVertexAttribIPointerId, toNext);

        mOutFile.Write(bufStart, toNext);
    }

    void emitGlDisableVertexAttribArray(GLuint index)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(unsigned int) + 4);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest = writeBCall(dest, mGlDisableVertexAttribArrayId);
        dest = common::WriteFixed<unsigned int>(dest, index); // literal

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitDeleteBuffers(GLsizei n, const GLuint *buffers)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall_vlen) + sizeof(GLuint) * (n + 2) + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest += sizeof(common::BCall_vlen);
        dest = common::WriteFixed<int>(dest, n);                                     // literal
        dest = common::Write1DArray<unsigned int>(dest, n, (unsigned int *)buffers); // array

        // NOTE: written to bufStart, not dest
        int toNext = dest - bufStart;
        writeBCall_vlen(bufStart, mGlDeleteBuffersId, toNext);

        mOutFile.Write(bufStart, toNext);
    }

    void emitBindSampler(GLuint unit, GLuint sampler)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(GLuint) * 2 + 4);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest = writeBCall(dest, mGlBindSamplerId);
        dest = common::WriteFixed<unsigned int>(dest, unit);
        dest = common::WriteFixed<unsigned int>(dest, sampler);

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitSwapBuffers(GLint dpy, GLint surface)
    {
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(GLint) * 3 + 4);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest = writeBCall(dest, mEglSwapBuffers);
        dest = common::WriteFixed<GLint>(dest, dpy);  // dpy
        dest = common::WriteFixed<GLint>(dest, surface);   // eglSurface
        dest = common::WriteFixed<int>(dest, 1); //result

        mOutFile.Write(bufStart, dest - bufStart);
    }

    void emitMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access, GLvoid *result)
    {
        DBG_LOG("emitMapBufferRange\n");
        mScratchBuff.resizeToFit(sizeof(common::BCall_vlen) + sizeof(int)*3 + sizeof(unsigned int)*3 + 32);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest += sizeof(common::BCall_vlen);
        dest = common::WriteFixed<int>(dest, target); // enum
        dest = common::WriteFixed<int>(dest, offset); // literal
        dest = common::WriteFixed<int>(dest, length); // literal
        dest = common::WriteFixed<unsigned int>(dest, access); // literal
        dest = common::WriteFixed<unsigned int>(dest, common::Opaque_Type_TM::BufferObjectReferenceType); // IS Simple Memory Offset
        dest = common::WriteFixed<unsigned int>(dest, (uintptr_t)result);

        int toNext = dest - bufStart;
        writeBCall_vlen(bufStart, mGlMapBufferRangeId, toNext);

        mOutFile.Write(bufStart, toNext);
    }

    void emitUnmapBuffer(GLenum target)
    {
        DBG_LOG("emitUnmapBuffer\n");
        mScratchBuff.resizeToFit(sizeof(common::BCall) + sizeof(unsigned int) + sizeof(unsigned char) + 4);

        char *const bufStart = mScratchBuff.bufferPtr();
        char *dest = bufStart;

        dest = writeBCall(dest, mGlUnmapBufferId);
        dest = common::WriteFixed<int>(dest, target); // enum
        dest = common::WriteFixed<unsigned char>(dest, 1); // result

        mOutFile.Write(bufStart, dest - bufStart);
    }

private:
    ScratchBuffer mScratchBuff;
    common::OutFile& mOutFile;
    int mThreadId;
    int mGlGenBuffersId;
    int mGlDeleteBuffersId;
    int mGlBufferDataId;
    int mGlBindBufferId;
    int mGlMapBufferRangeId;
    int mGlUnmapBufferId;
    int mGlBindVertexArrayId;
    int mGlVertexAttribPointerId;
    int mGlVertexAttribIPointerId;
    int mGlEnableVertexAttribArrayId;
    int mGlDisableVertexAttribArrayId;
    int mGlBindFramebufferId;
    int mGlGenTexturesId;
    int mGlDeleteTexturesId;
    int mGlTexImage2DId;
    int mGlTexSubImage2DId;
    int mGlTexSubImage3DId;
    int mGlActiveTextureId;
    int mGlBindTextureId;
    int mGlTexParameteriId;
    int mGlTexParameterfId;
    int mGlPixelStoreiId;
    int mGlCreateShaderId;
    int mGlShaderSourceId;
    int mGlCompileShaderId;
    int mGlCreateProgramId;
    int mGlAttachSahderId;
    int mGlLinkProgramId;
    int mGlUseProgramId;
    int mGlDeleteShaderId;
    int mGlDeleteProgramId;
    int mGlBindSamplerId;
    int mGlEnable;
    int mGlDisable;
    int mGlClear;
    int mGlClearColor;
    int mGlViewportId;
    int mGlColorMaskId;
    int mGlDrawElements;
    int mGlFrontFace;
    int mEglSwapBuffers;

    int getId(const char* name)
    {
        int id = common::gApiInfo.NameToId(name);

        if (id == 0)
        {
            DBG_LOG("Error: couldn't get sigbook-id for function %s", name);
            os::abort();
        }

        return id;
    }

    char* writeBCall(char* dest, int funcId)
    {
        common::BCall bcall;
        bcall.funcId = funcId;
        bcall.tid = mThreadId;
        bcall.reserved = 0;
        bcall.errNo = 0;
        bcall.source = 0;

        unsigned int bcallSize = sizeof(bcall);
        memcpy(dest, &bcall, bcallSize);

        return dest + bcallSize;
    }

    char* writeBCall_vlen(char* dest, int funcId, int toNext)
    {
        common::BCall_vlen bcv;
        bcv.funcId = funcId;
        bcv.tid = mThreadId;
        bcv.reserved = 0;
        bcv.errNo = 0;
        bcv.toNext = toNext;
        bcv.source = 0;

        unsigned int bcallSize = sizeof(bcv);
        memcpy(dest, &bcv, bcallSize);

        return dest + bcallSize;
    }
};

class BufferSaver
{
public:
    static void run(retracer::Context& retracerContext, common::OutFile& outFile, int threadId)
    {
        const auto buffers = retracerContext.getBufferMap().GetCopy();
        const auto revBuffers = retracerContext.getBufferRevMap().GetCopy();

        // Create helper which adds command to tracefile
        TraceCommandEmitter traceCommandEmitter(outFile, threadId);

        // Read buffer-id bound to GL_ARRAY_BUFFER locally (to restore when
        // done)
        GLint oldBoundBuffer = 0;
        _glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &oldBoundBuffer);

        // Find the corresponding trace-buffer-id as well (to rebind
        // when done in trace)
        GLint oldBoundBufferTrace = 0;
        auto search = revBuffers.find(oldBoundBuffer);
        if (search != revBuffers.end())
            oldBoundBufferTrace = search->second;

        for (const auto it : buffers)
        {
            const unsigned int traceBufferId = it.first;
            const unsigned int retraceBufferId = it.second;

            DBG_LOG("Saving buffer %d\n", traceBufferId);

            // Output glBindBuffer(GL_ARRAY_BUFFER, traceBufferId) to new
            // tracefile.
            traceCommandEmitter.emitBindBuffer(GL_ARRAY_BUFFER, traceBufferId);

            // Output glBufferData to new tracefile
            {
                // Bind buffer
                _glBindBuffer(GL_ARRAY_BUFFER, retraceBufferId);

                // Get buffer info
                GLint64 buffLength = 0;
                _glGetBufferParameteri64v(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &buffLength);
                GLint buffUsage = 0;
                _glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_USAGE, &buffUsage);

                // TODO: Skipping these buffers for now, but the right thing might actually be to output a glBufferData with size 0.
                if (buffLength == 0)
                {
                    DBG_LOG("Got 0 as buffLength. Bug or empty buffer?\n");
                    continue;
                }

                GLint pre_mapped = 0;
                GLint64 pre_offset = 0;
                GLint64 pre_mapLen = 0;
                GLint pre_access = 0;
                glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_MAPPED, &pre_mapped);
                if (pre_mapped)
                {
                    // Output glUnmapBuffer(GL_ARRAY_BUFFER) to new tracefile
                    traceCommandEmitter.emitUnmapBuffer(GL_ARRAY_BUFFER);
                    _glGetBufferParameteri64v(GL_ARRAY_BUFFER, GL_BUFFER_MAP_OFFSET, &pre_offset);
                    _glGetBufferParameteri64v(GL_ARRAY_BUFFER, GL_BUFFER_MAP_LENGTH, &pre_mapLen);
                    _glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_ACCESS_FLAGS, &pre_access);
                    _glUnmapBuffer(GL_ARRAY_BUFFER);
                }

                // Map and output to trace
                void* data = _glMapBufferRange(GL_ARRAY_BUFFER, 0, buffLength, GL_MAP_READ_BIT);
                {
                    if (!data)
                    {
                        DBG_LOG("glMapBufferRange returned NULL\n");
                        os::abort();
                    }

                    // Emit glBufferData(GL_ARRAY_BUFFER, len, data, usage);
                    traceCommandEmitter.emitBufferData(GL_ARRAY_BUFFER, buffLength, data, buffUsage);
                }
                _glUnmapBuffer(GL_ARRAY_BUFFER);
                if (pre_mapped)
                {
                    // Emit glMapBufferRange() to new tracefile
                    traceCommandEmitter.emitMapBufferRange(GL_ARRAY_BUFFER, pre_offset, pre_mapLen, pre_access, retracerContext._bufferToData_map.at(retraceBufferId));
                    _glMapBufferRange(GL_ARRAY_BUFFER, pre_offset, pre_mapLen, pre_access);
                }
            }
        }

        // Rebind previously bound buffer in trace:
        // output glBindBuffer(GL_ARRAY_BUFFER, oldBoundBufferTrace)
        traceCommandEmitter.emitBindBuffer(GL_ARRAY_BUFFER, oldBoundBufferTrace);

        // Bind previously bound buffer locally
        _glBindBuffer(GL_ARRAY_BUFFER, oldBoundBuffer);
    }
};

bool checkError(const std::string &msg)
{
    GLenum error = glGetError();

    if (error == GL_NO_ERROR)
    {
        return false;
    }

    DBG_LOG("glGetError: 0x%x (context/msg: %s)\n", error, msg.c_str());

    return true;
}

class TextureSaver
{
public:
    TextureSaver(retracer::Context& retracerContext, common::OutFile& outFile, int threadId)
        : mRetracerContext(retracerContext), mOutFile(outFile), mThreadId(threadId), mScratchBuff()
    {
        TexTypeInfo info2d("2D", GL_TEXTURE_2D, GL_TEXTURE_BINDING_2D, TraceCommandEmitter::Tex2D);
        TexTypeInfo info2dArray("2D_Array", GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BINDING_2D_ARRAY, TraceCommandEmitter::Tex3D);
        TexTypeInfo infoCubemap("Cubemap", GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BINDING_CUBE_MAP, TraceCommandEmitter::Tex2D);
        TexTypeInfo infoCubemapArray("Cubemap_Array", GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_BINDING_CUBE_MAP_ARRAY, TraceCommandEmitter::Tex3D);
        texTypeToInfo[DepthDumper::Tex2D] = info2d;
        texTypeToInfo[DepthDumper::Tex2DArray] = info2dArray;
        texTypeToInfo[DepthDumper::TexCubemap] = infoCubemap;
        texTypeToInfo[DepthDumper::TexCubemapArray] = infoCubemapArray;

        depthDumper.initializeDepthCopyer();
    }

    ~TextureSaver()
    {
    }

    void run()
    {
        checkError("TextureSaver::run begin");

        TraceCommandEmitter traceCommandEmitter(mOutFile, mThreadId);

        const auto textures = mRetracerContext.getTextureMap().GetCopy();
        const auto revTextures = mRetracerContext.getTextureRevMap().GetCopy();
        const auto revBuffers = mRetracerContext.getBufferRevMap().GetCopy();

        // To download textures, we need GL_PIXEL_PACK_BUFFER to be unbound.
        // To upload textures in the trace, we need GL_PIXEL_UNPACK_BUFFER to be unbound.

        // Find currently bound GL_PIXEL_UNPACK_BUFFER
        GLint oldUnpackBuffer = 0;
        _glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &oldUnpackBuffer);

        // Clear bingding locally
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        // Find corresponding buffer-id in the trace-file
        GLint oldUnpackBufferTrace = 0;
        auto search = revBuffers.find(oldUnpackBuffer);
        if (search != revBuffers.end())
            oldUnpackBufferTrace = search->second;
        // Emit command to clear the binding. We restore it when done below.
        traceCommandEmitter.emitBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        // Read buffer-id bound to GL_PIXEL_PACK_BUFFER_BINDING locally (to restore when done)
        GLint oldPackBuffer;
        _glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &oldPackBuffer);

        // Clear binding locally (so we can download texture data)
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        struct StoreParams
        {
            GLenum e;
            GLint value;
        } storeParams[] = {
            {GL_PACK_ROW_LENGTH, 0},
            {GL_UNPACK_IMAGE_HEIGHT, 0},
            {GL_PACK_SKIP_ROWS, 0},
            {GL_PACK_SKIP_PIXELS, 0},
            {GL_UNPACK_SKIP_IMAGES, 0},
            {GL_PACK_ALIGNMENT, 4},
            {GL_UNPACK_ROW_LENGTH, 0},
            {GL_UNPACK_IMAGE_HEIGHT, 0},
            {GL_UNPACK_SKIP_ROWS, 0},
            {GL_UNPACK_SKIP_PIXELS, 0},
            {GL_UNPACK_SKIP_ROWS, 0},
            {GL_UNPACK_ALIGNMENT, 4},
        };

        // Save glPixelStorei-params
        for (unsigned int i = 0; i < sizeof(storeParams)/sizeof(storeParams[0]); i++)
        {
            _glGetIntegerv(storeParams[i].e, &storeParams[i].value);
        }

        // Set and emit sane values
        for (unsigned int i = 0; i < sizeof(storeParams)/sizeof(storeParams[0]); i++)
        {
            // Set all to 0...
            GLint value = 0;

            // ... except alignment
            if (storeParams[i].e == GL_UNPACK_ALIGNMENT || storeParams[i].e == GL_PACK_ALIGNMENT)
                value = 1;

            // Set locally
            _glPixelStorei(storeParams[i].e, value);

            // Emit command to trace
            traceCommandEmitter.emitPixelStorei(storeParams[i].e, value);
        }

        // Save textures
        for (const auto it : textures)
        {
            const unsigned int traceTextureId = it.first;
            const unsigned int retraceTextureId = it.second;

            if (retraceTextureId == 0)
            {
                continue;
            }

            //assert(revTextures[retraceTextureId] == traceTextureId);
            if (revTextures.at(retraceTextureId) != traceTextureId)
            {
                DBG_LOG("WARNING: Reverse texture lookup failed: retrace-ID %d's rev. was %d, but should be %d.\n", retraceTextureId, revTextures.at(retraceTextureId), traceTextureId);
            }

            bool ok = false;

            // We don't know the appropriate target for the texture, so we try different kinds
            // until we find a match.
            // These methods will return false if it's not a texture of this type.

            // supports GL_TEXTURE_2D, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_ARRAY textures for now
            for (unsigned int i = DepthDumper::Tex2D; i < DepthDumper::TexEnd; ++i)
            {
                ok = saveTexture(static_cast<DepthDumper::TexType>(i), traceCommandEmitter, traceTextureId, retraceTextureId);
                if (ok)
                    break;
            }

            if (!ok)
            {
                DBG_LOG("DEBUG: Texture %d (retrace-id %d) couldn't be saved.\n", traceTextureId, retraceTextureId);
            }
        }

        // Restore values
        for (unsigned int i = 0; i < sizeof(storeParams)/sizeof(storeParams[0]); i++)
        {
            // Set
            _glPixelStorei(storeParams[i].e, storeParams[i].value);

            // Emit
            traceCommandEmitter.emitPixelStorei(storeParams[i].e, storeParams[i].value);
        }

        // Rebind previously bound UNPACK-buffer in trace
        traceCommandEmitter.emitBindBuffer(GL_PIXEL_UNPACK_BUFFER, oldUnpackBufferTrace);

        // Bind previously bound PACK-buffer and UNPACK-buffer locally
        glBindBuffer(GL_PIXEL_PACK_BUFFER, oldPackBuffer);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, oldUnpackBuffer);

        checkError("TextureSaver::run end");
    }

private:
    struct TexTypeInfo
    {
        std::string name;
        GLenum target;
        GLenum bindTarget;
        TraceCommandEmitter::TexDimension texDimension;
        TexTypeInfo()
        {}
        TexTypeInfo(std::string _name, GLenum _target, GLenum _bindTarget, TraceCommandEmitter::TexDimension _texDimension):
            name(_name),
            target(_target),
            bindTarget(_bindTarget),
            texDimension(_texDimension)
        {}
    };

    std::map<DepthDumper::TexType, TexTypeInfo> texTypeToInfo;

    bool isArrayTex(DepthDumper::TexType type)
    {
        if (type == DepthDumper::Tex2D || type == DepthDumper::TexCubemap)
            return false;
        return true;
    }

    bool isCubemapTex(DepthDumper::TexType type)
    {
        if (type == DepthDumper::TexCubemap)
            return true;
        return false;
    }

    struct TextureInfo
    {
        struct MipmapSize
        {
            GLint width;
            GLint height;
            GLint depth;
        };
        std::vector<MipmapSize> mMipmapSizes;

        GLint mInternalFormat;
        bool mIsCompressed;

        // Only set if not compressed!
        // Can be GL_NONE, GL_SIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_FLOAT, GL_INT, and GL_UNSIGNED_INT
        GLint mRedType;
        GLint mGreenType;
        GLint mBlueType;
        GLint mAlphaType;
        GLint mDepthType;

        GLint mRedSize;
        GLint mGreenSize;
        GLint mBlueSize;
        GLint mAlphaSize;
        GLint mDepthSize;
    };

    DepthDumper depthDumper;

    // Gets info for texture bound to target (e.g. GL_TEXTURE_2D)
    static TextureInfo getTextureInfo(GLuint target)
    {
        if (target == GL_TEXTURE_CUBE_MAP)
            target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
        checkError("getTextureInfo begin");

        TextureInfo info;
        ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_INTERNAL_FORMAT, &info.mInternalFormat);

        GLint isCompressed;
        ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_COMPRESSED, &isCompressed);
        if (isCompressed == GL_TRUE)
        {
            info.mIsCompressed = true;
        }
        else
        {
            // _glGetTexLevelParameteriv with GL_TEXTURE_COMPRESSED can return GL_FALSE even if the
            // texture is compressed (e.g. for GL_ETC1_RGB8_OES). So we have to do some additional checks.
            switch (info.mInternalFormat)
            {
            case GL_ETC1_RGB8_OES:
            case GL_COMPRESSED_R11_EAC:
            case GL_COMPRESSED_SIGNED_R11_EAC:
            case GL_COMPRESSED_RG11_EAC:
            case GL_COMPRESSED_SIGNED_RG11_EAC:
            case GL_COMPRESSED_RGB8_ETC2:
            case GL_COMPRESSED_SRGB8_ETC2:
            case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
            case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
            case GL_COMPRESSED_RGBA8_ETC2_EAC:
            case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
            case GL_COMPRESSED_RGBA_ASTC_4x4_KHR :
            case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:
            case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:
            case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:
            case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:
            case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:
            case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:
            case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:
            case GL_COMPRESSED_RGBA_ASTC_10x5_KHR :
            case GL_COMPRESSED_RGBA_ASTC_10x6_KHR :
            case GL_COMPRESSED_RGBA_ASTC_10x8_KHR :
            case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:
            case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:
            case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:
            case GL_PALETTE4_RGB8_OES:
            case GL_PALETTE4_RGBA8_OES:
            case GL_PALETTE4_R5_G6_B5_OES :
            case GL_PALETTE4_RGBA4_OES:
            case GL_PALETTE4_RGB5_A1_OES:
            case GL_PALETTE8_RGB8_OES:
            case GL_PALETTE8_RGBA8_OES:
            case GL_PALETTE8_R5_G6_B5_OES :
            case GL_PALETTE8_RGBA4_OES:
            case GL_PALETTE8_RGB5_A1_OES:
            case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            case GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG:
            case GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG:
            case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:
            case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:
                info.mIsCompressed = true;
                break;
            default:
                info.mIsCompressed = false;
            }
        }


        // Midgard DDK (4aa1dd85062b) asserts on texture 7 of Egypt HD 25fps (ETC1) when asking for GL_TEXTURE_RED_TYPE.
        // ==>[ASSERT] (eglretrace) : In file: midg/src/helpers/src/mali_midg_pixel_format.c   line: 734 midg_pixel_format_get_info
        // !midg_pixel_format_is_compressed( pfs )
        // .. even though the texture is correctly reported to be compressed when querying GL_TEXTURE_COMPRESSED.
        if (!info.mIsCompressed)
        {
            ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_RED_TYPE, &info.mRedType);
            ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_GREEN_TYPE, &info.mGreenType);
            ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_BLUE_TYPE, &info.mBlueType);
            ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_ALPHA_TYPE, &info.mAlphaType);
            ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_DEPTH_TYPE, &info.mDepthType);

            ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_RED_SIZE, &info.mRedSize);
            ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_GREEN_SIZE, &info.mGreenSize);
            ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_BLUE_SIZE, &info.mBlueSize);
            ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_ALPHA_SIZE, &info.mAlphaSize);
            ff_glGetTexLevelParameteriv(target, 0, GL_TEXTURE_DEPTH_SIZE, &info.mDepthSize);
        }

        // Find number of mipmap levels
        GLint maxLevel;
        GLint baseLevel;
        if (target == GL_TEXTURE_CUBE_MAP_POSITIVE_X) {
            _glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, &maxLevel);
            _glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, &baseLevel);
        }
        else {
            _glGetTexParameteriv(target, GL_TEXTURE_MAX_LEVEL, &maxLevel);
            _glGetTexParameteriv(target, GL_TEXTURE_BASE_LEVEL, &baseLevel);
        }
        DBG_LOG("maxLevel by _glGetTexParameteriv = %d\n", maxLevel);
        DBG_LOG("baseLevel by _glGetTexParameteriv = %d\n", baseLevel);

        // Read size of each mipmap
        for (int i = 0; i <= maxLevel; i++)
        {
            TextureInfo::MipmapSize s;
            ff_glGetTexLevelParameteriv(target, i, GL_TEXTURE_WIDTH,  &s.width);
            ff_glGetTexLevelParameteriv(target, i, GL_TEXTURE_HEIGHT, &s.height);
            ff_glGetTexLevelParameteriv(target, i, GL_TEXTURE_DEPTH, &s.depth);

            if (s.width == 0 || s.height == 0)
            {
                // This level is not valid. Stop querying.
                break;
            }
            else
            {
                info.mMipmapSizes.push_back(s);
            }
        }

        checkError("getTextureInfo end");
        return info;
    }

    // This tries to bind texture 'id' to 'target'; return true on success
    // false on failure. Does NOT restore the old binding if it fails!
    bool tryBindTexture(GLenum target, GLuint id)
    {
        // Clear errors
        checkError("tryBindTexture begin");

        // Try to bind texture
        _glBindTexture(target, id);

        GLenum error = glGetError();
        if (error == GL_NO_ERROR)
        {
            // Bingo!
            return true;
        }

        // Must not have been a texture appropriate for this target
        return false;
    }

    // Returns TRUE if the texture is a texture of the type we want (even if the function failed to save it), and FALSE if it isn't
    bool saveTexture(DepthDumper::TexType texType, TraceCommandEmitter& traceCommandEmitter, unsigned int traceTextureId, unsigned int retraceTextureId)
    {
        auto typeInfoIter = texTypeToInfo.find(texType);
        if (typeInfoIter == texTypeToInfo.end())
        {
            DBG_LOG("Cannot find texture type %d\n", texType);
            return false;
        }
        const TexTypeInfo &typeInfo = typeInfoIter->second;
        bool isArray = isArrayTex(texType);
        bool isCubemap = isCubemapTex(texType);
        checkError("saveTexture" + typeInfo.name + " begin");

        // Remember currently bound texture
        GLint retraceRestoreTexure;
        _glGetIntegerv(typeInfo.bindTarget, &retraceRestoreTexure);

        // Try to bind the to-be-saved texture to the appropriate target
        if (!tryBindTexture(typeInfo.target, retraceTextureId))
        {
            // It's not this kind of texture; restore binding and return false.
            _glBindTexture(typeInfo.target, retraceRestoreTexure);
            return false;
        }

        // It's a texture of the type we want, so we return true (we attempted to save it)
        bool retval = true;

        // Find corresponding currently-bound texture in trace
        GLint traceRestoreTexture = mRetracerContext.getTextureRevMap().RValue(retraceRestoreTexure);

        DBG_LOG("Looking up %s texture %d (retrace-id %d).\n", typeInfo.name.c_str(), traceTextureId, retraceTextureId);
        DBG_LOG("Texture %d was bound in retracer. This corresponds to %d in trace. This binding will be saved and restored.\n", retraceRestoreTexure, traceRestoreTexture);

        TextureInfo texInfo = getTextureInfo(typeInfo.target);

        if (texInfo.mIsCompressed)
        {
            DBG_LOG("NOTE: Skipped saving texture %d (retrace-id %d) because it is compressed (internalformat is %s (0x%X)). (Assumption: compressed textures can't be changed after upload.)\n\n", traceTextureId, retraceTextureId, EnumString(texInfo.mInternalFormat), texInfo.mInternalFormat);
            _glBindTexture(typeInfo.target, retraceRestoreTexure);
            return retval;
        }
        else if (texInfo.mInternalFormat == GL_RGB9_E5)
        {
            DBG_LOG("NOTE: Skipped saving texture %d (retrace-id %d) because its internalformat is %s (0x%X). (Assumption: This kind of textures can't be changed after upload.)\n\n", traceTextureId, retraceTextureId, EnumString(texInfo.mInternalFormat), texInfo.mInternalFormat);
            _glBindTexture(typeInfo.target, retraceRestoreTexure);
            return retval;
        }

        DBG_LOG("Attempting to save %s texture %d (retrace-id %d).\n", typeInfo.name.c_str(), traceTextureId, retraceTextureId);
        DBG_LOG("Texture has red-type:   %s (0x%x)\n", EnumString(texInfo.mRedType), texInfo.mRedType);
        DBG_LOG("Texture has green-type: %s (0x%x)\n", EnumString(texInfo.mGreenType), texInfo.mGreenType);
        DBG_LOG("Texture has blue-type:  %s (0x%x)\n", EnumString(texInfo.mBlueType), texInfo.mBlueType);
        DBG_LOG("Texture has alpha-type: %s (0x%x)\n", EnumString(texInfo.mAlphaType), texInfo.mAlphaType);
        DBG_LOG("Texture has depth-type: %s (0x%x)\n", EnumString(texInfo.mDepthType), texInfo.mDepthType);
        DBG_LOG("Texture has red-size:   (%d)\n", texInfo.mRedSize);
        DBG_LOG("Texture has green-size: (%d)\n", texInfo.mGreenSize);
        DBG_LOG("Texture has blue-size:  (%d)\n", texInfo.mBlueSize);
        DBG_LOG("Texture has alpha-size: (%d)\n", texInfo.mAlphaSize);
        DBG_LOG("Texture has depth-size: (%d)\n", texInfo.mDepthSize);
        DBG_LOG("Texture has internalformat: %s (0x%x)\n", EnumString(texInfo.mInternalFormat), texInfo.mInternalFormat);

        // Write BindTexture
        traceCommandEmitter.emitBindTexture(typeInfo.target, traceTextureId);

        // Save each mipmap level
        const int numMipmapLevels = texInfo.mMipmapSizes.size();
        for (int curMipmapLevel = 0; curMipmapLevel < numMipmapLevels; curMipmapLevel++)
        {
            TextureInfo::MipmapSize mipmapSize = texInfo.mMipmapSizes.at(curMipmapLevel);

            GLint readTexFormat = texInfo.mInternalFormat;
            GLint readTexType = GL_UNSIGNED_BYTE;

            if (texInfo.mInternalFormat == GL_LUMINANCE ||
                texInfo.mInternalFormat == GL_LUMINANCE_ALPHA ||
                texInfo.mInternalFormat == GL_LUMINANCE8_OES ||
                texInfo.mInternalFormat == GL_LUMINANCE4_ALPHA4_OES ||
                texInfo.mInternalFormat == GL_LUMINANCE8_ALPHA8_OES ||
                texInfo.mInternalFormat == GL_LUMINANCE32F_EXT ||
                texInfo.mInternalFormat == GL_LUMINANCE_ALPHA32F_EXT ||
                texInfo.mInternalFormat == GL_LUMINANCE16F_EXT ||
                texInfo.mInternalFormat == GL_LUMINANCE_ALPHA16F_EXT)
            {
                DBG_LOG("NOTE: Skipped saving texture %d (retrace-id %d) because "
                        "its internalformat is %s. It cannot be bound to a FBO. "
                        "(Assumption: it can't be changed after upload.)\n",
                        traceTextureId, retraceTextureId, EnumString(texInfo.mInternalFormat));
                traceCommandEmitter.emitBindTexture(typeInfo.target, traceRestoreTexture);
                _glBindTexture(typeInfo.target, retraceRestoreTexure);
                return retval;
            }
            if (texInfo.mRedType && texInfo.mGreenType && texInfo.mBlueType)
            {
                if (texInfo.mAlphaType)
                    if (texInfo.mInternalFormat == GL_RGBA16UI)
                        readTexFormat = GL_RGBA_INTEGER;
                    else
                        readTexFormat = GL_RGBA;
                else
                    if (texInfo.mInternalFormat == GL_RGB16UI)
                        readTexFormat = GL_RGB_INTEGER;
                    else
                        readTexFormat = GL_RGB;
                if (texInfo.mRedSize == 32 && texInfo.mGreenSize == 32 && texInfo.mBlueSize == 32 && texInfo.mAlphaSize == 32) // GL_RGBA32F
                {
                    readTexType = GL_FLOAT;
                }
                else if (texInfo.mRedSize == 16 && texInfo.mGreenSize == 16 && texInfo.mBlueSize == 16 && texInfo.mAlphaSize == 16) // GL_RGBA16F
                {
                    if (texInfo.mInternalFormat == GL_RGBA16UI)
                        readTexType = GL_UNSIGNED_INT;
                    else
                        readTexType = GL_HALF_FLOAT;
                }
                else if (texInfo.mRedSize == 11 && texInfo.mGreenSize == 11 && texInfo.mBlueSize == 10) // GL_R11G11B10
                {
                    readTexType = GL_UNSIGNED_INT_10F_11F_11F_REV;
                }
                else if (texInfo.mRedSize == 10 && texInfo.mGreenSize == 10 && texInfo.mBlueSize == 10 && texInfo.mAlphaSize == 2) // GL_RGB10_A2
                {
                    readTexType = GL_UNSIGNED_INT_2_10_10_10_REV;
                }
                else if (texInfo.mRedSize == 4 && texInfo.mGreenSize == 4 && texInfo.mBlueSize == 4 && texInfo.mAlphaSize == 4) // GL_RGBA4
                {
                    readTexType = GL_UNSIGNED_SHORT_4_4_4_4;
                }
                else if (texInfo.mRedSize == 5 && texInfo.mGreenSize == 5 && texInfo.mBlueSize == 5 && texInfo.mAlphaSize == 1) // GL_RGB5A_1
                {
                    readTexType = GL_UNSIGNED_SHORT_5_5_5_1;
                }
                else if (texInfo.mRedSize == 5 && texInfo.mGreenSize == 6 && texInfo.mBlueSize == 5) // GL_RGB565
                {
                    readTexType = GL_UNSIGNED_SHORT_5_6_5;
                }
            }
            else
            {
                if (texInfo.mInternalFormat == GL_R16F)
                {
                    readTexFormat = GL_RED_EXT;
                    readTexType = GL_HALF_FLOAT;
                }
                else if (texInfo.mInternalFormat == GL_R32F)
                {
                    readTexFormat = GL_RED;
                    readTexType = GL_FLOAT;
                }
                else if (texInfo.mInternalFormat == GL_R8)
                {
                    readTexFormat = GL_RED;
                    readTexType = GL_UNSIGNED_BYTE;
                }
                else if (texInfo.mInternalFormat == GL_RG16F)
                {
                    readTexFormat = GL_RG_EXT;
                    readTexType = GL_HALF_FLOAT;
                }
                else if (texInfo.mInternalFormat == GL_RG32F)
                {
                    readTexFormat = GL_RG_EXT;
                    readTexType = GL_FLOAT;
                }
                else if (texInfo.mInternalFormat == GL_RG8)
                {
                    readTexFormat = GL_RG_EXT;
                    readTexType = GL_UNSIGNED_BYTE;
                }
                else if (texInfo.mInternalFormat == GL_DEPTH_COMPONENT16)
                {
                    readTexFormat = GL_DEPTH_COMPONENT;
                    readTexType = GL_UNSIGNED_INT;
                }
                else if (texInfo.mInternalFormat == GL_DEPTH_COMPONENT24)
                {
                    readTexFormat = GL_DEPTH_COMPONENT;
                    readTexType = GL_UNSIGNED_INT;
                }
                else if (texInfo.mInternalFormat == GL_DEPTH_COMPONENT32F)
                {
                    readTexFormat = GL_DEPTH_COMPONENT;
                    readTexType = GL_FLOAT;
                }
                else if (texInfo.mInternalFormat == GL_DEPTH24_STENCIL8 || texInfo.mInternalFormat == GL_DEPTH_STENCIL)
                {
                    readTexFormat = GL_DEPTH_STENCIL;
                    readTexType = GL_UNSIGNED_INT_24_8;
                }
                else if (texInfo.mInternalFormat == GL_RG32UI)
                {
                    readTexFormat = GL_RG_INTEGER;
                    readTexType = GL_UNSIGNED_INT;
                }
                else if (texInfo.mInternalFormat == GL_R16UI)
                {
                    readTexFormat = GL_RED_INTEGER;
                    readTexType = GL_UNSIGNED_SHORT;
                }
                else if (texInfo.mInternalFormat == GL_ALPHA)
                {
                    readTexFormat = GL_ALPHA;
                    readTexType = GL_UNSIGNED_BYTE;
                }
                else
                {
                    DBG_LOG("Couldn't figure out format to use for internalformat 0x%x\n", texInfo.mInternalFormat);
                    DBG_LOG("Querying the texture determined it wasn't RGB(A) (using glGetTexLevelParameteriv with GL_TEXTURE_X_TYPE), and those cases aren't handled yet.\n");
                    traceCommandEmitter.emitBindTexture(GL_TEXTURE_2D, traceRestoreTexture);
                    _glBindTexture(GL_TEXTURE_2D, retraceRestoreTexure);
                    return retval;
                }
            }

            size_t textureSize = _gl_image_size(readTexFormat, readTexType, mipmapSize.width, mipmapSize.height, 1, true);
            assert(textureSize > 0);


// TODO: Investigate what's needed to get the data for as many types of textures as possible
#if 0
            if (readTexFormat == GL_ALPHA)
                readTexFormat = GL_RED;

            GLint implReadFormat, implReadType;
            _glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &implReadFormat);
            _glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE,   &implReadType);

            switch(texInfo.mInternalFormat)
            {
            case GL_RGBA:
                readTexFormat = GL_RGBA;
                break;
            case GL_RGB:
                readTexFormat = GL_RGB;
                break;
            default:
                DBG_LOG("Not sure which type to use for glReadPixels for internalformat %x. Trying %x.\n", texInfo.mInternalFormat, readTexFormat);
            }
#endif

            ScratchBuffer texData(textureSize);
            bool readError = false;

            // Read texture data
            checkError("Read texture data begin");

            GLuint fbo = 0;
            GLint prev_fbo = 0;
            GLint prev_readBuff = 0;
            GLint prev_pixelPackBuffer = 0;
            GLenum status = 0;

            // Remember old bindings
            _glGetIntegerv(GL_READ_BUFFER, &prev_readBuff);
            _glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &prev_pixelPackBuffer);
            _glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prev_fbo);

            _glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

            int textureNum = isCubemap ? 6 : mipmapSize.depth;
            for (int i = 0; i < textureNum; ++i)          // all layers of array texture
            {
                _glGenFramebuffers(1, &fbo);
                _glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

                if (readTexFormat == GL_DEPTH_COMPONENT || readTexFormat == GL_DEPTH_STENCIL) {
#ifdef ENABLE_X11
                    if (isArray) {
                        _glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, retraceTextureId, curMipmapLevel, i);
                    }
                    else {
                        if (texType == DepthDumper::TexCubemap) {
                            _glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, retraceTextureId, curMipmapLevel);
                        }
                        else {
                            _glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, retraceTextureId, curMipmapLevel);
                            if (readTexFormat == GL_DEPTH_STENCIL)
                            {
                                _glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, retraceTextureId, curMipmapLevel);
                            }
                        }
                    }
                    status = _glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
                    if (status != GL_FRAMEBUFFER_COMPLETE) {
                        DBG_LOG("glCheckFramebufferStatus error: 0x%x\n", status);
                        readError |= true;
                    }
#endif  // ENABLE_X11 end
                }
                else {
                    if (isArray) {
                        _glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, retraceTextureId, curMipmapLevel, i);
                    }
                    else {
                        if (texType == DepthDumper::TexCubemap) {
                            _glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, retraceTextureId, curMipmapLevel);
                        }
                        else {
                            _glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, retraceTextureId, curMipmapLevel);
                        }
                    }
                    status = _glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
                    if (status != GL_FRAMEBUFFER_COMPLETE) {
                        DBG_LOG("glCheckFramebufferStatus error: 0x%x\n", status);
                        readError |= true;
                    }
                }

// The default support list of "format" of glReadPixels doesn't contain GL_DEPTH_COMPONENT.
// According to my test, nvidia GPU supports it but arm GPU doesn't.
// So we have to render the depth texture to a color texture and then glReadPixels from this color texture.
// There might be some precision problems, but it is the best we can do now.
// And it passed the test of GFXbench 5 Aztec whose depth texture internalFormat is GL_DEPTH_COMPONENT24.
#ifdef ENABLE_X11
                _glReadBuffer(GL_COLOR_ATTACHMENT0);

                checkError("_glReadPixels begin");
                {
                    DBG_LOG("ReadPixels: w=%d, h=%d, format=0x%X=%s, type=0x%X=%s, data=%p\n", mipmapSize.width, mipmapSize.height, readTexFormat, EnumString(readTexFormat), readTexType, EnumString(readTexType), texData.bufferPtr());
                    _glReadPixels(0, 0, mipmapSize.width, mipmapSize.height, readTexFormat, readTexType, texData.bufferPtr());
                }
                readError |= checkError("_glReadPixels end");
#else   // ENABLE_X11 not being defined
                if (readTexFormat == GL_DEPTH_COMPONENT || readTexFormat == GL_DEPTH_STENCIL) {
                    depthDumper.get_depth_texture_image(retraceTextureId, mipmapSize.width, mipmapSize.height, texData.bufferPtr(), texInfo.mInternalFormat, texType, i);
                    if (texInfo.mInternalFormat == GL_DEPTH_COMPONENT16 || texInfo.mInternalFormat == GL_DEPTH_COMPONENT24) {
                        float *fp = (float*)texData.bufferPtr();
                        unsigned int *ip = (unsigned int *)texData.bufferPtr();
                        for (int i = 0; i < mipmapSize.width * mipmapSize.height; ++i) {
                            unsigned int factor = 0xFFFFFFFFu;
                            ip[i] = (double)fp[i] * factor;
                        }
                    }
                    else if (texInfo.mInternalFormat == GL_DEPTH_COMPONENT32F) {
                        DBG_LOG("WARNING: The texture of internalFormat GL_DEPTH_COMPONENT32F was never tested before. So there might be some problems!\n");
                    }
                    DBG_LOG("depth dump: w=%d, h=%d, format=0x%X=%s, type=0x%X=%s, data=%p\n", mipmapSize.width, mipmapSize.height, readTexFormat, EnumString(readTexFormat), readTexType, EnumString(readTexType), texData.bufferPtr());
                }
                else {
                    _glReadBuffer(GL_COLOR_ATTACHMENT0);

                    checkError("_glReadPixels begin");
                    {
                        DBG_LOG("ReadPixels: w=%d, h=%d, format=0x%X=%s, type=0x%X=%s, data=%p\n", mipmapSize.width, mipmapSize.height, readTexFormat, EnumString(readTexFormat), readTexType, EnumString(readTexType), texData.bufferPtr());
                        _glReadPixels(0, 0, mipmapSize.width, mipmapSize.height, readTexFormat, readTexType, texData.bufferPtr());
                    }
                    readError |= checkError("_glReadPixels end");
                }
#endif  // ENABLE_X11 end
                _glDeleteFramebuffers(1, &fbo);
                checkError("Read texture data end");

                // Write glTexSubImage2D for this level
                if(!readError)
                {
                    GLenum target = 0;
                    int zoffset = 0, depth = 0;
                    switch (texType) {
                    case DepthDumper::Tex2D:
                        target = GL_TEXTURE_2D;
                        zoffset = 0;
                        depth = 0;
                        break;
                    case DepthDumper::Tex2DArray:
                        target = GL_TEXTURE_2D_ARRAY;
                        zoffset = i;
                        depth = 1;
                        break;
                    case DepthDumper::TexCubemap:
                        target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
                        zoffset = 0;
                        depth = 0;
                        break;
                    case DepthDumper::TexCubemapArray:
                        target = GL_TEXTURE_CUBE_MAP_ARRAY;
                        zoffset = i;
                        depth = 1;
                        break;
                    default:
                        break;
                    }
                    traceCommandEmitter.emitTexSubImage(typeInfo.texDimension, // dimension
                        target,             // target
                        curMipmapLevel,     // level
                        0,                  // xoffset
                        0,                  // yoffset
                        zoffset,            // zoffset
                        mipmapSize.width,   // width
                        mipmapSize.height,  // height
                        depth,              // depth
                        readTexFormat,      // format
                        readTexType,        // type
                        textureSize,
                        (const char*) texData.bufferPtr());
                }
                else
                {
                    DBG_LOG("---->> Failed to save the texture because glReadPixels failed. <<-----\n");
                }
            }
            // Restore old state
            _glBindBuffer(GL_PIXEL_PACK_BUFFER, prev_pixelPackBuffer);
            _glBindFramebuffer(GL_READ_FRAMEBUFFER, prev_fbo);
            _glReadBuffer(prev_readBuff);
        }

        // Restore old tex in trace
        traceCommandEmitter.emitBindTexture(typeInfo.target, traceRestoreTexture);

        // Restore previously bound texture
        _glBindTexture(typeInfo.target, retraceRestoreTexure);

        checkError("saveTexture" + typeInfo.name + " end");
        return retval;
    }
private:
    retracer::Context& mRetracerContext;
    common::OutFile& mOutFile;
    int mThreadId;
    ScratchBuffer mScratchBuff;
};

class DefaultFboSaver
{
public:
    DefaultFboSaver(retracer::Context& retracerContext, common::OutFile& outFile, int threadId, unsigned int repeat, int dpy, int surface)
        : mRetracerContext(retracerContext),
          mOutFile(outFile),
          mThreadId(threadId),
          mDisplay(dpy),
          mSurface(surface),
          mRepeat(repeat),
          mScratchBuff(),
          mCmdEmitter(outFile, threadId)
    {
    }

    ~DefaultFboSaver()
    {
    }

#define CHECK_FBO0_DUMP_RESULT(success)\
        if (success == false)   \
        {                       \
            DBG_LOG("Fail to dump defautl FBO color buffer\n");   \
            return; \
        }

    void run()
    {
        bool success;

        SaveFramebufStates();
        SavePixelStoreStates();

        success = InitStates();
        CHECK_FBO0_DUMP_RESULT(success)

        success = getColorbufInfo();
        CHECK_FBO0_DUMP_RESULT(success)

        success = getInternalFormat();
        CHECK_FBO0_DUMP_RESULT(success);

        success = readFbo0();
        CHECK_FBO0_DUMP_RESULT(success);

        emitRenderFbo0();

        RestoreFramebufStates();
        RestorePixelStoreStates();
    }

private:
    GLenum SaveFramebufStates()
    {
        _glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &mReadFbo);
        _glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &mDrawFbo);
        _glGetIntegerv(GL_READ_BUFFER, &mReadBuffer);

        return _glGetError();
    }
    GLenum RestoreFramebufStates()
    {
        _glBindFramebuffer(GL_READ_FRAMEBUFFER, mReadFbo);
        _glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mDrawFbo);
        _glReadBuffer(mReadBuffer);

        return _glGetError();
    }

    bool SavePixelStoreStates()
    {
        GLenum error;
        _glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &mPackBuffer);
        _glGetIntegerv(GL_PACK_ROW_LENGTH, &mPixelStore.pack.row_lenth);
        _glGetIntegerv(GL_PACK_SKIP_PIXELS, &mPixelStore.pack.skip_pixels);
        _glGetIntegerv(GL_PACK_SKIP_ROWS, &mPixelStore.pack.skip_rows);
        _glGetIntegerv(GL_PACK_ALIGNMENT, &mPixelStore.pack.alignment);
        error = _glGetError();
        if (error != GL_NO_ERROR)
        {
            DBG_LOG("Save pack pixel stores state fail, error: 0x%x\n", error);
        }
        _glGetIntegerv(GL_UNPACK_ROW_LENGTH,   &mPixelStore.unpack.row_lenth);
        _glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &mPixelStore.unpack.image_height);
        _glGetIntegerv(GL_UNPACK_SKIP_ROWS,    &mPixelStore.unpack.skip_rows);
        _glGetIntegerv(GL_UNPACK_SKIP_PIXELS,  &mPixelStore.unpack.skip_pixels);
        _glGetIntegerv(GL_UNPACK_SKIP_IMAGES,  &mPixelStore.unpack.skip_images);
        _glGetIntegerv(GL_UNPACK_ALIGNMENT,  &mPixelStore.unpack.alignment);
        error = _glGetError();
        if (error != GL_NO_ERROR)
        {
            DBG_LOG("Save unpack pixel stores state fail, error: 0x%x\n", error);
        }
        return true;
    }

    bool RestorePixelStoreStates()
    {
        GLenum error;
        _glPixelStorei(GL_PACK_ROW_LENGTH, mPixelStore.pack.row_lenth);
        _glPixelStorei(GL_PACK_SKIP_PIXELS, mPixelStore.pack.skip_pixels);
        _glPixelStorei(GL_PACK_SKIP_ROWS, mPixelStore.pack.skip_rows);
        _glPixelStorei(GL_PACK_ALIGNMENT, mPixelStore.pack.alignment);
        error = _glGetError();
        if (error != GL_NO_ERROR)
        {
            DBG_LOG("Restore pack pixel stores state fail, error: 0x%x\n", error);
        }
        _glPixelStorei(GL_UNPACK_ROW_LENGTH, mPixelStore.unpack.row_lenth);
        _glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, mPixelStore.unpack.image_height);
        _glPixelStorei(GL_UNPACK_SKIP_ROWS, mPixelStore.unpack.skip_rows);
        _glPixelStorei(GL_UNPACK_SKIP_PIXELS, mPixelStore.unpack.skip_pixels);
        _glPixelStorei(GL_UNPACK_SKIP_IMAGES, mPixelStore.unpack.skip_images);
        _glPixelStorei(GL_UNPACK_ALIGNMENT, mPixelStore.unpack.alignment);
        error = _glGetError();
        if (error != GL_NO_ERROR)
        {
            DBG_LOG("Restore unpack pixel stores state fail, error: 0x%x\n", error);
        }
        _glBindBuffer(GL_PIXEL_PACK_BUFFER, mPackBuffer);
        return true;
    }

    bool InitPixelStoreStates()
    {
        GLenum error;
        _glPixelStorei(GL_PACK_ROW_LENGTH, 0);
        _glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
        _glPixelStorei(GL_PACK_SKIP_ROWS, 0);
        _glPixelStorei(GL_PACK_ALIGNMENT, 4);
        error = _glGetError();
        if (error != GL_NO_ERROR)
        {
            DBG_LOG("Init pack pixel stores state fail, error: 0x%x\n", error);
        }
        _glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        _glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
        _glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
        _glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        _glPixelStorei(GL_UNPACK_SKIP_IMAGES, 0);
        _glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        error = _glGetError();
        if (error != GL_NO_ERROR)
        {
            DBG_LOG("Restore unpack pixel stores state fail, error: 0x%x\n",error);
        }
        return true;
    }

    bool InitStates()
    {
        GLenum error;
        _glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        _glBindFramebuffer(GL_READ_FRAMEBUFFER, mDrawFbo);
        _glReadBuffer(mDrawFbo == 0 ? GL_BACK:GL_COLOR_ATTACHMENT0);
        error = _glGetError();
        if (error != GL_NO_ERROR)
        {
            DBG_LOG("Init read frame buffer fail, error: 0x%x\n",error);
            return false;
        }
        GLenum status = _glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            DBG_LOG("glCheckFramebufferStatus error: 0x%x\n", status);
            return false;
        }
        InitPixelStoreStates();
        return true;
    }

    bool checkEGLError(EGLBoolean eglRet, const char* str)
    {
        if (eglRet != EGL_TRUE)
        {
            EGLint error = eglGetError();
            DBG_LOG("%s fail because of error: 0x%x\n", str, error);
            return false;
        }
        return true;
    }

    bool getFrameBufInfo()
    {
        GLenum error;
        mColorBufInfo.FrameBuf.format = 0;
        mColorBufInfo.FrameBuf.type   = 0;
        _glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &mColorBufInfo.FrameBuf.format);
        _glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &mColorBufInfo.FrameBuf.type);
        error =_glGetError();

        if (error != GL_NO_ERROR ||
            mColorBufInfo.FrameBuf.format == 0 ||
            mColorBufInfo.FrameBuf.type == 0)
        {
            DBG_LOG("Fail to get default color buffer format and type.\n");
            return false;
        }
        glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER,
                                              mDrawFbo==0 ? GL_BACK:GL_COLOR_ATTACHMENT0,
                                              GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE,
                                              &mColorBufInfo.FrameBuf.ComponentType);
        error = _glGetError();
        if (error != GL_NO_ERROR)
        {
            DBG_LOG("Fail to get frame buffer color attachment component type, error:  0x%x\n", error);
            return false;
        }
        glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER,
                                              mDrawFbo==0 ? GL_BACK:GL_COLOR_ATTACHMENT0,
                                              GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING,
                                              &mColorBufInfo.FrameBuf.ColorEncoding);
        error = _glGetError();
        if (error != GL_NO_ERROR)
        {
            DBG_LOG("Fail to get frame buffer color attachment component type, error:  0x%x\n", error);
            return false;
        }
        return true;
    }

    template <class T>
    T getObjectMaxId(std::unordered_map<T, T> &ObjectMap)
    {
        T maxId = 0;
        for (const auto it: ObjectMap)
        {
            maxId = it.first > maxId ? it.first:maxId;
        }
        return maxId;
    }
#define SAVE_EXIT_EGL_SURF_INFO(ret)    \
    if (Config != NULL)                 \
    {                                   \
        free(Config);                   \
    }                                   \
    return ret;

    bool getEGLSurfaceInfo()
    {
        EGLBoolean success;
        EGLint ConfigId = -1;
        EGLConfig *Config = NULL;
        EGLint numConfig;

        EGLSurface DrawSurf = _eglGetCurrentSurface(EGL_DRAW);
        EGLDisplay CurDisplay = _eglGetCurrentDisplay();

        mColorBufInfo.Surf.width = 0;
        mColorBufInfo.Surf.height = 0;
        success = _eglQuerySurface(CurDisplay,
                                   DrawSurf,
                                   EGL_WIDTH,
                                   &mColorBufInfo.Surf.width);
        if (checkEGLError(success, "Get default FBO color surface width") != true)
        {
            SAVE_EXIT_EGL_SURF_INFO(false)
        }
        success = _eglQuerySurface(CurDisplay,
                                   DrawSurf,
                                   EGL_HEIGHT,
                                   &mColorBufInfo.Surf.height);
        if (checkEGLError(success, "Get default FBO color surface height") != true)
        {
            SAVE_EXIT_EGL_SURF_INFO(false)
        }
        success = _eglQuerySurface(CurDisplay,
                                   DrawSurf,
                                   EGL_GL_COLORSPACE,
                                   &mColorBufInfo.Surf.colorSpace);
        if (success != EGL_TRUE)
        {
            DBG_LOG("Failed to get color space error 0x%x, force color space to EGL_GL_COLORSPACE_LINEAR!\n", success);
            mColorBufInfo.Surf.colorSpace = EGL_GL_COLORSPACE_LINEAR;
        }
        success = _eglQuerySurface(CurDisplay,
                                   DrawSurf,
                                   EGL_CONFIG_ID,
                                   &ConfigId);
        if (checkEGLError(success, "Get default FBO color surface config_id") != true)
        {
            SAVE_EXIT_EGL_SURF_INFO(false)
        }
        success = _eglGetConfigs(CurDisplay,
                                 NULL,
                                 0,
                                 &numConfig);
        if (checkEGLError(success, "Get EGL config number") != true)
        {
            SAVE_EXIT_EGL_SURF_INFO(false)
        }
        Config = (EGLConfig *)malloc(sizeof(EGLConfig) * numConfig);
        if (Config == NULL)
        {
            DBG_LOG("Allocate EGL configs fail!");
        }
        success = _eglGetConfigs(CurDisplay,
                                 Config,
                                 numConfig,
                                 &numConfig);
        if (checkEGLError(success, "Get EGL configs") != true)
        {
            SAVE_EXIT_EGL_SURF_INFO(false)
        }

        EGLint i;
        EGLint id;
        for (i = 0; i < numConfig; i++)
        {
            success = _eglGetConfigAttrib(CurDisplay,
                                          Config[i],
                                          EGL_CONFIG_ID,
                                          &id);
            if (checkEGLError(success, "Get EGL config") == true)
            {
                if (id == ConfigId)
                {
                    break;
                }
            }
        }
        if (i >= numConfig)
        {
            DBG_LOG("Fail to find a EGL config coresponding to the default FBO\n");
            SAVE_EXIT_EGL_SURF_INFO(false)
        }

        success = _eglGetConfigAttrib(CurDisplay,
                                      Config[i],
                                      EGL_ALPHA_SIZE,
                                      &mColorBufInfo.Surf.ComponentSize.A);
        if (checkEGLError(success, "Get default FBO color surface A size") != true)
        {
            SAVE_EXIT_EGL_SURF_INFO(false)
        }
        success = _eglGetConfigAttrib(CurDisplay,
                                      Config[i],
                                      EGL_RED_SIZE,
                                      &mColorBufInfo.Surf.ComponentSize.R);
        if (checkEGLError(success, "Get default FBO color surface R size") != true)
        {
            SAVE_EXIT_EGL_SURF_INFO(false)
        }
        success = _eglGetConfigAttrib(CurDisplay,
                                      Config[i],
                                      EGL_GREEN_SIZE,
                                      &mColorBufInfo.Surf.ComponentSize.G);
        if (checkEGLError(success, "Get default FBO color surface G size") != true)
        {
            SAVE_EXIT_EGL_SURF_INFO(false)
        }
        success = _eglGetConfigAttrib(CurDisplay,
                                      Config[i],
                                      EGL_BLUE_SIZE,
                                      &mColorBufInfo.Surf.ComponentSize.B);
        if (checkEGLError(success, "Get default FBO color surface B size") != true)
        {
            SAVE_EXIT_EGL_SURF_INFO(false)
        }
        success = _eglGetConfigAttrib(CurDisplay,
                                      Config[i],
                                      EGL_LUMINANCE_SIZE,
                                      &mColorBufInfo.Surf.ComponentSize.L);
        if (checkEGLError(success, "Get default FBO color surface L size") != true)
        {
            mColorBufInfo.Surf.ComponentSize.L = 0;
        }
        success = _eglGetConfigAttrib(CurDisplay,
                                      Config[i],
                                      EGL_SAMPLES,
                                      &mColorBufInfo.Surf.samples);
        if (checkEGLError(success, "Get default FBO color surface samples") != true)
        {
            mColorBufInfo.Surf.samples = 1;
        }
        success = _eglGetConfigAttrib(CurDisplay,
                                      Config[i],
                                      EGL_SAMPLE_BUFFERS,
                                      &mColorBufInfo.Surf.sampleBuffers);
        if (checkEGLError(success, "Get default FBO color surface sample buffers") != true)
        {
            mColorBufInfo.Surf.sampleBuffers = 0;
        }
        success = _eglGetConfigAttrib(CurDisplay,
                                      Config[i],
                                      EGL_COLOR_BUFFER_TYPE,
                                      &mColorBufInfo.Surf.colorBufferType);
        if (checkEGLError(success, "Get default FBO color surface color buffer type") != true)
        {
            mColorBufInfo.Surf.colorBufferType = EGL_RGB_BUFFER;
        }
        SAVE_EXIT_EGL_SURF_INFO(true)
    }

    bool getColorbufInfo()
    {
        if (getFrameBufInfo() == false)
        {
            return false;
        }

        if (getEGLSurfaceInfo() == false)
        {
            return false;
        }

        return true;
    }
    bool getRGBAInternalFormat()
    {
        bool Ret = false;
        switch (mColorBufInfo.FrameBuf.type)
        {
        case GL_UNSIGNED_SHORT_4_4_4_4:
        case GL_UNSIGNED_SHORT_5_5_5_1:
            mColorBufInfo.internalFormat = GL_RGBA;
            Ret = true;
            break;
        case GL_UNSIGNED_BYTE:
            if (mColorBufInfo.Surf.colorSpace == EGL_GL_COLORSPACE_LINEAR ||
                mColorBufInfo.Surf.colorSpace == EGL_GL_COLORSPACE_LINEAR_KHR)
            {
                if (mColorBufInfo.Surf.ComponentSize.R == 8 &&
                    mColorBufInfo.Surf.ComponentSize.G == 8 &&
                    mColorBufInfo.Surf.ComponentSize.B == 8 &&
                    mColorBufInfo.Surf.ComponentSize.A == 8)
                {
                    mColorBufInfo.internalFormat = GL_RGBA8;
                    Ret = true;
                }
                else if (mColorBufInfo.Surf.ComponentSize.R == 5 &&
                         mColorBufInfo.Surf.ComponentSize.G == 5 &&
                         mColorBufInfo.Surf.ComponentSize.B == 5 &&
                         mColorBufInfo.Surf.ComponentSize.A == 1)
                {
                    mColorBufInfo.internalFormat = GL_RGB5_A1;
                    Ret = true;
                }
                else
                {
                    DBG_LOG("Unsupported RGBA internal format\n");
                }
            }
            else if (mColorBufInfo.Surf.colorSpace == EGL_GL_COLORSPACE_SRGB ||
                     mColorBufInfo.Surf.colorSpace == EGL_GL_COLORSPACE_SRGB_KHR)
            {
                if (mColorBufInfo.Surf.ComponentSize.R == 8 &&
                    mColorBufInfo.Surf.ComponentSize.G == 8 &&
                    mColorBufInfo.Surf.ComponentSize.B == 8 &&
                    mColorBufInfo.Surf.ComponentSize.A == 8)
                {
                    mColorBufInfo.internalFormat = GL_SRGB8_ALPHA8;
                    Ret = true;
                }
                else
                {
                    DBG_LOG("Unsupported RGBA internal format\n");
                }
            }
            else
            {
                DBG_LOG("Unsupported color space 0x%x\n", mColorBufInfo.Surf.colorSpace);
            }
            break;
        default:
            DBG_LOG("Unsupported RGBA internal format\n");
            break;
        }

        return Ret;
    }
    bool getRGBInternalFormat()
    {
        bool Ret = false;
        switch (mColorBufInfo.FrameBuf.type)
        {
        case GL_UNSIGNED_SHORT_5_6_5:
            mColorBufInfo.internalFormat = GL_RGB;
            Ret = true;
            break;
        case GL_UNSIGNED_BYTE:
            if (mColorBufInfo.Surf.colorSpace == EGL_GL_COLORSPACE_LINEAR ||
                mColorBufInfo.Surf.colorSpace == EGL_GL_COLORSPACE_LINEAR_KHR)
            {
                if (mColorBufInfo.Surf.ComponentSize.R == 8 &&
                    mColorBufInfo.Surf.ComponentSize.G == 8 &&
                    mColorBufInfo.Surf.ComponentSize.B == 8)
                {
                    mColorBufInfo.internalFormat = GL_RGB8;
                    Ret = true;
                }
                else if (mColorBufInfo.Surf.ComponentSize.R == 5 &&
                         mColorBufInfo.Surf.ComponentSize.G == 6 &&
                         mColorBufInfo.Surf.ComponentSize.B == 5)
                {
                    mColorBufInfo.internalFormat = GL_RGB565;
                    Ret = true;
                }
                else
                {
                    DBG_LOG("Unsupported RGB internal format\n");
                }
            }
            else if (mColorBufInfo.Surf.colorSpace == EGL_GL_COLORSPACE_SRGB ||
                     mColorBufInfo.Surf.colorSpace == EGL_GL_COLORSPACE_SRGB_KHR)
            {
                if (mColorBufInfo.Surf.ComponentSize.R == 8 &&
                    mColorBufInfo.Surf.ComponentSize.G == 8 &&
                    mColorBufInfo.Surf.ComponentSize.B == 8)
                {
                    mColorBufInfo.internalFormat = GL_SRGB8;
                    Ret = true;
                }
                else
                {
                    DBG_LOG("Unsupported RGB internal format\n");
                }
            }
            else
            {
                DBG_LOG("Unsupported color space 0x%x\n", mColorBufInfo.Surf.colorSpace);
            }
            break;
        default:
            DBG_LOG("Unsupported RGBA internal format\n");
            break;
        }

        return Ret;
    }
    bool getInternalFormat()
    {
        bool Ret = false;
        switch (mColorBufInfo.FrameBuf.format)
        {
        case GL_RGBA:
            Ret = getRGBAInternalFormat();
            break;
        case GL_RGB:
            Ret = getRGBInternalFormat();
            break;
        default:
            DBG_LOG("Frame buffer format is not supported yet\n");
        }
        return Ret;
    }

    bool readFbo0()
    {
        bool success = false;
        mColorBufInfo.size = _gl_image_size(mColorBufInfo.FrameBuf.format,
                                            mColorBufInfo.FrameBuf.type,
                                            mColorBufInfo.Surf.width,
                                            mColorBufInfo.Surf.height,
                                            1,
                                            false);
        assert(mColorBufInfo.size > 0);
        mScratchBuff.resizeToFit(mColorBufInfo.size);

        _glReadPixels(0,
                      0,
                      mColorBufInfo.Surf.width,
                      mColorBufInfo.Surf.height,
                      mColorBufInfo.FrameBuf.format,
                      mColorBufInfo.FrameBuf.type,
                      mScratchBuff.bufferPtr());
        success = !checkError("_glReadPixels end");
        if (success != true)
        {
            DBG_LOG("Fail to read default FBO content\n");
            return false;
        }

        return true;
    }

    void emitProgram()
    {
        GLint vs, fs;
        const GLchar *VsCode =
            "#version 310 es\n\
            precision highp float;\n\
            layout(location = 0) in highp vec2 a_Position;\n\
            out highp vec2 v_TexCoordinate;\n\
            void main() {\n\
            v_TexCoordinate = a_Position * 0.5 + vec2(0.5, 0.5);\n\
            gl_Position = vec4(a_Position, 0.0f, 1.0f);\n\
            }";
        auto shaders = mRetracerContext.getShaderMap().GetCopy();
        vs = getObjectMaxId(shaders) + 1;
        mCmdEmitter.emitCreateShader(GL_VERTEX_SHADER, vs);
        mCmdEmitter.emitShaderSource(vs, 1, &VsCode, 0);
        mCmdEmitter.emitCompileShader(vs);

        const GLchar *FSCode =
            "#version 310 es\n\
            precision highp float;\n\
            layout(location = 0) uniform highp sampler2D u_Texture;\n\
            in highp vec2 v_TexCoordinate;\n\
            out highp vec4 fragColor;\n\
            void main() {\n\
            fragColor = texture(u_Texture, v_TexCoordinate).xyzw;\n\
            }";

        fs = vs + 1;
        mCmdEmitter.emitCreateShader(GL_FRAGMENT_SHADER, fs);
        mCmdEmitter.emitShaderSource(fs, 1, &FSCode, 0);
        mCmdEmitter.emitCompileShader(fs);

        _glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&mProgram);
        auto programs = mRetracerContext.getProgramMap().GetCopy();
        mProgram = mRetracerContext.getProgramRevMap().RValue(mProgram);

        mRenderProgram = getObjectMaxId(programs) + 1;
        mCmdEmitter.emitCreateProgram(mRenderProgram);
        mCmdEmitter.emitAttachShader(mRenderProgram, vs);
        mCmdEmitter.emitAttachShader(mRenderProgram, fs);
        mCmdEmitter.emitLinkProgram(mRenderProgram);
        mCmdEmitter.emitDeleteShader(vs);
        mCmdEmitter.emitDeleteShader(fs);
        mCmdEmitter.emitUseProgram(mRenderProgram);
    }

    void emitTexture()
    {
        _glGetIntegerv(GL_ACTIVE_TEXTURE, &mActiveTex);
        _glActiveTexture(GL_TEXTURE0);
        _glGetIntegerv(GL_TEXTURE_BINDING_2D, &mRetraceTexture2D);
        mTraceTexure2D = mRetracerContext.getTextureRevMap().RValue(mRetraceTexture2D);

        _glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &mTex2DWrapS);
        _glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &mTex2DWrapT);
        _glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &mTex2DMinFilter);
        _glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &mTex2DMaxFilter);
        _glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, &mTex2DCompareMode);

        auto textures = mRetracerContext.getTextureMap().GetCopy();
        mFbo0Tex = getObjectMaxId(textures)+1;
        mCmdEmitter.emitActiveTexture(GL_TEXTURE0);
        mCmdEmitter.emitGenTextures(1, &mFbo0Tex);
        mCmdEmitter.emitBindTexture(GL_TEXTURE_2D, mFbo0Tex);
        mCmdEmitter.emitTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        mCmdEmitter.emitTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        mCmdEmitter.emitTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        mCmdEmitter.emitTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        mCmdEmitter.emitTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        mCmdEmitter.emitTexImage2D(GL_TEXTURE_2D,
                                   0,
                                   mColorBufInfo.internalFormat,
                                   mColorBufInfo.Surf.width,
                                   mColorBufInfo.Surf.height,
                                   0,
                                   mColorBufInfo.FrameBuf.format,
                                   mColorBufInfo.FrameBuf.type,
                                   mScratchBuff.bufferPtr(),
                                   mColorBufInfo.size);
    }

    void emitVBO()
    {
        // vertex buffer
        _glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &mArrayBuffer);
        mArrayBuffer = mRetracerContext.getBufferRevMap().RValue(mArrayBuffer);

        GLfloat v[] = {1.0, 1.0, -1.0, 1.0, -1.0, -1.0, 1.0, -1.0};
        auto buffers = mRetracerContext.getBufferMap().GetCopy();
        mVB = getObjectMaxId(buffers) + 1;

        mCmdEmitter.emitGenBuffers(1, &mVB);
        mCmdEmitter.emitBindBuffer(GL_ARRAY_BUFFER, mVB);
        mCmdEmitter.emitBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);

        // element buffer
        _glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &mElementBuffer);
        mElementBuffer = mRetracerContext.getBufferRevMap().RValue(mElementBuffer);

        GLuint index[] = {0, 1, 2, 0, 2, 3};
        mIB = mVB + 1;
        mCmdEmitter.emitGenBuffers(1, &mIB);
        mCmdEmitter.emitBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIB);
        mCmdEmitter.emitBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index), index, GL_STATIC_DRAW);

        // vao
        _glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &mVAO);
        mCmdEmitter.emitBindVertexArray(0);

        // vertex attribute
        _glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &mVA0En);
        _glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &mVA0VertexBuffer);
        _glGetVertexAttribIiv(0, GL_VERTEX_ATTRIB_ARRAY_INTEGER, &misVA0Integer);
        _glGetVertexAttribIiv(0, GL_VERTEX_ATTRIB_ARRAY_SIZE, &mVA0Size);
        _glGetVertexAttribIiv(0, GL_VERTEX_ATTRIB_ARRAY_TYPE, &mVA0Type);
        _glGetVertexAttribIiv(0, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &mVA0Normalized);
        _glGetVertexAttribIiv(0, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &mVA0Stride);
        _glGetVertexAttribPointerv(0, GL_VERTEX_ATTRIB_ARRAY_POINTER, &mVA0Pointer);
        mCmdEmitter.emitVertexAttibPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        mCmdEmitter.emitEnableVertexAttribArray(0);
    }

    void emitDraw()
    {
        _glGetIntegerv(GL_VIEWPORT, mViewport);
        _glGetBooleanv(GL_COLOR_WRITEMASK, mColorMask);
        _glGetBooleanv(GL_SCISSOR_TEST, &mScissorEn);
        _glGetBooleanv(GL_BLEND, &mBlendEn);
        _glGetBooleanv(GL_RASTERIZER_DISCARD, &mRastDiscardEn);
        _glGetBooleanv(GL_CULL_FACE, &mCullFaceEn);
        _glGetIntegerv(GL_FRONT_FACE, &mFrontFace);
        _glGetFloatv(GL_COLOR_CLEAR_VALUE, mClearColor);
        _glGetIntegerv(GL_SAMPLER_BINDING, &mSampler);

        mCmdEmitter.emitViewport(0, 0, mColorBufInfo.Surf.width, mColorBufInfo.Surf.height);
        mCmdEmitter.emitColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        mCmdEmitter.emitDisable(GL_SCISSOR_TEST);
        mCmdEmitter.emitDisable(GL_BLEND);
        mCmdEmitter.emitDisable(GL_RASTERIZER_DISCARD);
        mCmdEmitter.emitDisable(GL_CULL_FACE);
        mCmdEmitter.emitFrontFace(GL_CCW);
        mCmdEmitter.emitBindSampler(0, 0);
        mCmdEmitter.emitClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    }

    void emitSwap()
    {
        mCmdEmitter.emitSwapBuffers(mDisplay, mSurface);
    }

    void emitBooleanbState(GLenum state, GLboolean value)
    {
        if (value)
        {
            mCmdEmitter.emitEnable(state);
        }
        else
        {
            mCmdEmitter.emitDisable(state);
        }
    }

    void emitCleanUp()
    {
        // restore program
        mCmdEmitter.emitUseProgram(mProgram);
        mCmdEmitter.emitDeleteProgram(mRenderProgram);

        // restore texture pipe
        mCmdEmitter.emitTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mTex2DWrapS);
        mCmdEmitter.emitTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mTex2DWrapT);
        mCmdEmitter.emitTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mTex2DMinFilter);
        mCmdEmitter.emitTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mTex2DMaxFilter);
        mCmdEmitter.emitTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, mTex2DCompareMode);
        mCmdEmitter.emitBindTexture(GL_TEXTURE_2D, mTraceTexure2D);
        mCmdEmitter.emitActiveTexture(mActiveTex);
        mCmdEmitter.emitDeleteTextures(1, &mFbo0Tex);

        _glBindTexture(GL_TEXTURE_2D, mRetraceTexture2D);
        _glActiveTexture(mActiveTex);
        GLenum error = _glGetError();
        if (error != GL_NO_ERROR)
        {
            DBG_LOG("Fail to restore texture with error: 0x%x\n", error);
        }

        // restore VBO VAO
        if (misVA0Integer)
        {
            mCmdEmitter.emitVertexAttribIPointer(0, mVA0Size, mVA0Type, mVA0Stride, mVA0Pointer);
        }
        else
        {
            mCmdEmitter.emitVertexAttibPointer(0, mVA0Size, mVA0Type, mVA0Normalized, mVA0Size, mVA0Pointer);
            if (!mVA0En)
            {
                mCmdEmitter.emitGlDisableVertexAttribArray(0);
            }
        }
        mCmdEmitter.emitBindVertexArray(mVAO);
        mCmdEmitter.emitBindBuffer(GL_ARRAY_BUFFER, mArrayBuffer);
        mCmdEmitter.emitBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElementBuffer);
        mCmdEmitter.emitDeleteBuffers(1, &mVB);
        mCmdEmitter.emitDeleteBuffers(1, &mIB);

        // restore states
        emitBooleanbState(GL_BLEND, mBlendEn);
        emitBooleanbState(GL_RASTERIZER_DISCARD, mRastDiscardEn);
        emitBooleanbState(GL_CULL_FACE, mCullFaceEn);
        emitBooleanbState(GL_SCISSOR_TEST, mScissorEn);

        mCmdEmitter.emitBindSampler(0, mSampler);
        mCmdEmitter.emitFrontFace(mFrontFace);
        mCmdEmitter.emitViewport(mViewport[0], mViewport[1], mViewport[2], mViewport[3]);
        mCmdEmitter.emitColorMask(mColorMask[0], mColorMask[1], mColorMask[2], mColorMask[3]);
        mCmdEmitter.emitClearColor(mClearColor[0], mClearColor[1], mClearColor[2], mClearColor[3]);
    }

    void emitRenderFbo0()
    {
        emitProgram();
        emitTexture();
        emitVBO();
        emitDraw();

        // repeat to inject draw and swap. minus 1 because the last injected swap was the one in trace file
        for (unsigned int i = 0; i < mRepeat-1; i++)
        {
            mCmdEmitter.emitClear(GL_COLOR_BUFFER_BIT);
            mCmdEmitter.emitDrawElements(GL_TRIANGLES, 2 * 3, GL_UNSIGNED_INT, 0);
            mCmdEmitter.emitSwapBuffers(mDisplay, mSurface);
        }

        mCmdEmitter.emitClear(GL_COLOR_BUFFER_BIT);
        mCmdEmitter.emitDrawElements(GL_TRIANGLES, 2 * 3, GL_UNSIGNED_INT, 0);

        emitCleanUp();
    }
private:
    struct ColorBufInfo
    {
        struct
        {
            struct
            {
                EGLint R;
                EGLint G;
                EGLint B;
                EGLint A;
                EGLint L;
            } ComponentSize;
            EGLint width;
            EGLint height;
            EGLint colorSpace;
            EGLint samples;
            EGLint sampleBuffers;
            EGLint colorBufferType;
        } Surf;
        struct
        {
            GLint format;
            GLint type;
            GLint ComponentType;
            GLint ColorEncoding;
        } FrameBuf;
        GLenum internalFormat;
        size_t size;
    } mColorBufInfo;

    struct PixelStoreCommState
    {
        GLint row_lenth;
        GLint skip_pixels;
        GLint skip_rows;
        GLint alignment;
    };

    struct PixelStoreStates
    {
        PixelStoreCommState pack;
        struct : PixelStoreCommState
        {
            GLint image_height;
            GLint skip_images;
        } unpack;
    } mPixelStore;

    // The original info to save and restore
    GLint mReadFbo;
    GLint mDrawFbo;
    GLint mPackBuffer;
    GLint mReadBuffer;

    GLuint mProgram;
    GLuint mRenderProgram;

    // The original texture pipe info to save and restore
    GLint mActiveTex;
    GLint mRetraceTexture2D; // The retrace texture
    GLint mTraceTexure2D;    // The trace texture
    GLint mTex2DWrapS;
    GLint mTex2DWrapT;
    GLint mTex2DMinFilter;
    GLint mTex2DMaxFilter;
    GLint mTex2DCompareMode;

    // The temp texture to readback FBO0
    GLuint mFbo0Tex;

    // The original VBO and VAO info to save and restore
    GLint mArrayBuffer;
    GLint mElementBuffer;
    GLint mVAO;
    GLint mVA0En;
    GLint mVA0VertexBuffer;
    GLint misVA0Integer;
    GLint mVA0Size;
    GLint mVA0Type;
    GLint mVA0Normalized;
    GLint mVA0Stride;
    GLvoid *mVA0Pointer;

    // The temp buffers to readback FBO0
    GLuint mVB;
    GLuint mIB;

    // The original pipe states to save and restore
    GLint mViewport[4];
    GLint mFrontFace;
    GLfloat mClearColor[4];
    GLboolean mScissorEn;
    GLboolean mColorMask[4];
    GLboolean mBlendEn;
    GLboolean mCullFaceEn;
    GLboolean mRastDiscardEn;
    GLint mSampler;

    retracer::Context &mRetracerContext;
    common::OutFile &mOutFile;
    int mThreadId;
    int mDisplay;
    int mSurface;
    unsigned int mRepeat;
    ScratchBuffer mScratchBuff;
    TraceCommandEmitter mCmdEmitter;
};

} // End RetraceAndTrim namespace

static void injectClear(retracer::Context& retracerContext, common::OutFile& out, retracer::Retracer& retracer)
{
    // Save state
    GLint oldBoundFramebuffer = 0;
    _glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldBoundFramebuffer);
    const auto revFramebuffers = retracerContext.getFramebufferRevMap().GetCopy();
    GLint oldBoundFramebufferTrace = 0;
    auto search = revFramebuffers.find(oldBoundFramebuffer);
    if (search != revFramebuffers.end())
        oldBoundFramebufferTrace = search->second;
    GLint oldColors[4];
    GLboolean oldColorMask[4];
    GLboolean stencil;
    GLboolean scissor;
    _glGetBooleanv(GL_STENCIL_TEST, &stencil);
    _glGetBooleanv(GL_SCISSOR_TEST, &scissor);
    _glGetBooleanv(GL_COLOR_WRITEMASK, oldColorMask);
    _glGetIntegerv(GL_COLOR_CLEAR_VALUE, oldColors);

    if (!oldColorMask[0] || !oldColorMask[1] || !oldColorMask[2] || !oldColorMask[3])
    {
        DBG_LOG("WARNING: Color mask will prevent initial glClear\n");
    }

    // Inject glClear
    RetraceAndTrim::TraceCommandEmitter traceCommandEmitter(out, retracer.getCurTid());
    traceCommandEmitter.emitBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    traceCommandEmitter.emitClearColor(1.0, 0.0, 0.0, 1.0); // use ugly red color to highlight errors
    traceCommandEmitter.emitClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    // Restore states
    traceCommandEmitter.emitClearColor(oldColors[0], oldColors[1], oldColors[2], oldColors[3]);
    traceCommandEmitter.emitBindFramebuffer(GL_DRAW_FRAMEBUFFER, oldBoundFramebufferTrace);
    if (stencil)
    {
        traceCommandEmitter.emitEnable(GL_STENCIL_TEST);
    }
    else
    {
        traceCommandEmitter.emitDisable(GL_STENCIL_TEST);
    }
    if (scissor)
    {
        traceCommandEmitter.emitEnable(GL_SCISSOR_TEST);
    }
    else
    {
        traceCommandEmitter.emitDisable(GL_SCISSOR_TEST);
    }
}

using namespace retracer;

static void saveData(common::OutFile &out, unsigned int flags, unsigned int repeat, Json::Value& ffJson, GLint dpy, GLint surface)
{
    retracer::Retracer& retracer = gRetracer;

    RetraceAndTrim::checkError("RetraceAndTrim state-saving begin");

    // Should have a valid GL state at this point
    char* version = (char*) _glGetString(GL_VERSION);
    ffJson["GL_VERSION"]  = version;
    ffJson["GL_VENDOR"]   = (char*) _glGetString(GL_VENDOR);
    ffJson["GL_RENDERER"] = (char*) _glGetString(GL_RENDERER);

    // Sync
    _glMemoryBarrier(GL_ALL_BARRIER_BITS);
    _glFinish();

    // Save buffers
    {
    RetraceAndTrim::BufferSaver::run(retracer.getCurrentContext(), out, retracer.getCurTid());
    }

    // Save texture
    if (flags & FASTFORWARD_RESTORE_TEXTURES)
    {
        RetraceAndTrim::TextureSaver ts(retracer.getCurrentContext(), out, retracer.getCurTid());
        ts.run();
    }

    if (flags & FASTFORWARD_RESTORE_DEFAUTL_FBO)
    {
        RetraceAndTrim::DefaultFboSaver fbo0(retracer.getCurrentContext(), out, retracer.getCurTid(), repeat, dpy, surface);
        fbo0.run();
    }
    else
    {
        injectClear(retracer.getCurrentContext(), out, retracer);
    }

    RetraceAndTrim::checkError("RetraceAndTrim state-saving end");
}

static void replay_thread(common::OutFile &out, const int threadidx, const int our_tid, const FastForwardOptions& ffOptions, Json::Value& ffJson)
{
    std::unique_lock<std::mutex> lk(gRetracer.mConditionMutex);
    RetraceAndTrim::ScratchBuffer buffer;
    retracer::Retracer& retracer = gRetracer;

    if (retracer.getFileFormatVersion() <= common::HEADER_VERSION_3)
    {
        // Because they have different binary format for e.g. the pixel-array portion
        // of glTexSubImage2D, and we don't want to write emitter-functions for both the old
        // and new version.
        DBG_LOG("ERROR: Can't retrace traces with header-version <= HEADER_VERSION_3\n");
        os::abort();
    }

    ff_glGetTexLevelParameteriv = (PFN_GLGETTEXLEVELPARAMETERIV) eglGetProcAddress("glGetTexLevelParameteriv");

    if ((ffOptions.mFlags & FASTFORWARD_RESTORE_TEXTURES) && (ff_glGetTexLevelParameteriv == NULL))
    {
        DBG_LOG("ERROR: Couldn't fetch pointer to GLES31 function glGetTexLevelParameteriv, which is needed when creating a fastforward-trace doing texture-restoration.\n");
        os::abort();
    }

    unsigned int curDrawCallNo = 0;
    const bool isFrameTarget = (0 != ffOptions.mTargetFrame);
    bool arriveTarget = false;

    while (!retracer.mFinish.load(std::memory_order_consume))
    {
        const char *funcName = retracer.mFile.ExIdToName(retracer.mCurCall.funcId);
        const bool isSwapBuffers = (retracer.mCurCall.funcId == retracer.mFile.NameToExId("eglSwapBuffers") ||
                                    retracer.mCurCall.funcId == retracer.mFile.NameToExId("eglSwapBuffersWithDamageKHR"));
        const bool isDrawCall = (common::FREQUENCY_RENDER == common::GetCallFlags(funcName));

        const bool shouldSaveData = isFrameTarget ? (retracer.GetCurFrameId() == ffOptions.mTargetFrame - 1) && isSwapBuffers : curDrawCallNo == ffOptions.mTargetDrawCallNo;
        if (retracer.mCurCall.tid == retracer.mOptions.mRetraceTid && shouldSaveData)
        {
            DBG_LOG("Started saving GL state\n");
            GLint dpy = 0;
            GLint surface = 0;

            if (isSwapBuffers)
            {
                char* src = retracer.src;
                src = common::ReadFixed(src, dpy);
                src = common::ReadFixed(src, surface);
            }
            saveData(out, ffOptions.mFlags, ffOptions.mFbo0Repeat, ffJson, dpy, surface);
            DBG_LOG("Done saving GL state\n");
        }

        // Save calls.
        // Calling the function might modify what's pointed to by src (e.g. ReadStringArray does this),
        // so it's important that we copy the call before actually calling the function.
        if (true)
        {
            bool shouldSkip = (strstr(funcName, "SwapBuffers"))
                || (strstr(funcName, "glDraw") && strcmp(funcName, "glDrawBuffers") != 0) // By excluding glDrawBuffers, glDraw* matches all drawing funcs.
                || (strstr(funcName, "glDispatchCompute")) // Matches glDispatchCompute*
                || (strstr(funcName, "glClearBuffer")) // Matches glClearBuffer*
                || (strcmp(funcName, "glBlitFramebuffer") == 0) // NOTE: strCMP == 0
                || (strcmp(funcName, "eglSetDamageRegionKHR") == 0)  // NOTE: strCMP == 0
                || (strcmp(funcName, "glClear") == 0); // NOTE: strCMP == 0

            if (isFrameTarget)
            {
                arriveTarget = (retracer.GetCurFrameId() >= ffOptions.mTargetFrame);
                if (strstr(funcName, "SwapBuffers") && (retracer.GetCurFrameId() + 1 == ffOptions.mTargetFrame))
                {
                    // We save the call before the call is executed, and GetCurFrameId() isn't
                    // updated until the call (SwapBuffers) is made. This handles the case where this
                    // is the last swap before the target frame, so that it isn't wrongly skipped.
                    // (I.e., this swap marks the start of the target frame -- or equivalently, the end
                    // of frame 0.)
                    arriveTarget = true;
                }
            }
            else
            {
                arriveTarget = (curDrawCallNo >= ffOptions.mTargetDrawCallNo);
            }

            // Until the target frame, output everything but skipped calls. After that, output everything.
            if (arriveTarget || !shouldSkip)
            {
                // Translate funcId for call to id in current sigbook.
                unsigned short newId = common::gApiInfo.NameToId(funcName);

                common::BCall_vlen outBCall = retracer.mCurCall;
                outBCall.funcId = newId;

                if (outBCall.toNext == 0)
                {
                    // It's really a BCall-struct, so only copy the BCall part of it
                    buffer.resizeToFit(sizeof(common::BCall) + common::gApiInfo.IdToLenArr[newId]);

                    char* curScratch = buffer.bufferPtr();

                    memcpy(curScratch, &outBCall, sizeof(common::BCall));
                    curScratch += sizeof(common::BCall);

                    memcpy(curScratch, retracer.src, common::gApiInfo.IdToLenArr[newId] - sizeof(common::BCall));
                    curScratch += common::gApiInfo.IdToLenArr[newId] - sizeof(common::BCall);

                    out.Write(buffer.bufferPtr(), curScratch - buffer.bufferPtr());
                }
                else
                {
                    // It's a BCall_vlen
                    buffer.resizeToFit(sizeof(outBCall) + outBCall.toNext);

                    char* curScratch = buffer.bufferPtr();
                    memcpy(curScratch, &outBCall, sizeof(outBCall));
                    curScratch += sizeof(outBCall);

                    memcpy(curScratch, retracer.src, outBCall.toNext - sizeof(outBCall));
                    curScratch += outBCall.toNext - sizeof(outBCall);

                    out.Write(buffer.bufferPtr(), curScratch - buffer.bufferPtr());
                }
            }
        }

        // Call function
        if (retracer.fptr)
        {
            (*(RetraceFunc)retracer.fptr)(retracer.src); // increments src to point to end of parameter block

            if (isDrawCall)
            {
                curDrawCallNo++;
            }
        }

        if (retracer.GetCurFrameId() > ffOptions.mEndFrame)
        {
            retracer.mFinish.store(true);
        }

        // ---------------------------------------------------------------------------
        // Get next packet
skip_call:
        retracer.curCallNo++;
        if (!retracer.mFile.GetNextCall(retracer.fptr, retracer.mCurCall, retracer.src))
        {
            retracer.mFinish.store(true);
            for (auto &cv : retracer.conditions) cv.notify_one(); // Wake up all other threads
            break;
        }
        // Skip call because it is on an ignored thread?
        if (!retracer.mOptions.mMultiThread && retracer.mCurCall.tid != retracer.mOptions.mRetraceTid)
        {
            goto skip_call;
        }
        // Need to switch active thread?
        if (our_tid != retracer.mCurCall.tid)
        {
            retracer.latest_call_tid = retracer.mCurCall.tid; // need to use an atomic member copy of this here
            // Do we need to make this thread?
            if (retracer.thread_remapping.count(retracer.mCurCall.tid) == 0)
            {
                retracer.thread_remapping[retracer.mCurCall.tid] = retracer.threads.size();
                int newthreadidx = retracer.threads.size();
                retracer.conditions.emplace_back();
                retracer.threads.emplace_back(&replay_thread, std::ref(out), newthreadidx, (int)retracer.mCurCall.tid, std::ref(ffOptions), std::ref(ffJson));
            }
            else // Wake up existing thread
            {
                const int otheridx = retracer.thread_remapping.at(retracer.mCurCall.tid);
                retracer.conditions.at(otheridx).notify_one();
            }
            // Now we go to sleep. However, there is a race condition, since the thread we woke up above might already have told us
            // to wake up again, so keep waking up to check if that is indeed the case.
            bool success = false;
            do {
                success = retracer.conditions.at(threadidx).wait_for(lk, std::chrono::milliseconds(50), [&]{ return our_tid == retracer.latest_call_tid || retracer.mFinish.load(std::memory_order_consume); });
            } while (!success);
        }
    }
}

static bool retraceAndTrim(common::OutFile &out, const FastForwardOptions& ffOptions, Json::Value& ffJson)
{
    retracer::Retracer& retracer = gRetracer;

    if (retracer.getFileFormatVersion() <= common::HEADER_VERSION_3)
    {
        // Because they have different binary format for e.g. the pixel-array portion
        // of glTexSubImage2D, and we don't want to write emitter-functions for both the old
        // and new version.
        DBG_LOG("ERROR: Can't retrace traces with header-version <= HEADER_VERSION_3\n");
        os::abort();
    }

    ff_glGetTexLevelParameteriv = (PFN_GLGETTEXLEVELPARAMETERIV) eglGetProcAddress("glGetTexLevelParameteriv");

    if ((ffOptions.mFlags & FASTFORWARD_RESTORE_TEXTURES) && (ff_glGetTexLevelParameteriv == NULL))
    {
        DBG_LOG("ERROR: Couldn't fetch pointer to GLES31 function glGetTexLevelParameteriv, which is needed when creating a fastforward-trace doing texture-restoration.\n");
        os::abort();
    }

    retracer.mFile.GetNextCall(retracer.fptr, retracer.mCurCall, retracer.src);
    gRetracer.threads.resize(1);
    gRetracer.conditions.resize(1);
    gRetracer.thread_remapping[retracer.mCurCall.tid] = 0;
    replay_thread(out, 0, gRetracer.mCurCall.tid, ffOptions, ffJson);

    for (std::thread &t : gRetracer.threads)
    {
        if (t.joinable()) t.join();
    }
    GLWS::instance().Cleanup();
    gRetracer.CloseTraceFile();
    return true;
}

static void
usage(const char *argv0)
{
    fprintf(stderr,
        "Usage: %s [OPTIONS] \n"
        "Fastforward tracefile.\n"
        "\n"
        "  --input <input_trace> Target frame to fastforward [REQUIRED]\n"
        "  --output <output_file_name> Where to write fastforwarded trace file [REQUIRED]\n"
        "  --targetFrame <target> The frame number that should be fastforwarded to [REQUIRED]\n"
        "  --targetDrawCallNo <target> The draw call number that should be fastforwarded to [REQUIRED]\n"
        "  --endFrame <end> The frame number that should be ended (by default fastforward to the last frame)\n"
        "  --multithread Run in multithread mode\n"
        "  --offscreen Run in offscreen mode\n"
        "  --noscreen Run in pbuffer output mode\n"
        "  --norestoretex When generating a fastforward trace, don't inject commands to restore the contents of textures to what the would've been when retracing the original. (NOTE: NOT RECOMMEND)\n"
        "  --version Output the version of this program\n"
        "  --restorefbo0 <repeat_times> Repeat to inject a draw call commands and swapbuffer the given number of times to restore the last default FBO. Suggest repeating 3~4 times if setDamageRegionKHR, else repeating 1 time.\n"
        "\n"
        , argv0);
}

static int readValidValue(const char* v)
{
    char* endptr;
    errno = 0;

    int val = strtol(v, &endptr, 10);

    if(errno)
    {
        perror("strtol");
        exit(1);
    }

    if(endptr == v || *endptr != '\0')
    {
        fprintf(stderr, "Invalid parameter value: %s\n", v);
        exit(1);
    }

    return val;
}

static bool ParseCommandLine(int argc, char** argv, FastForwardOptions& ffOptions, RetraceOptions& cmdOpts)
{
    bool gotOutput = false;
    bool gotInput = false;
    bool gotTargetFrame = false;
    bool gotTargetDrawCallNo = false;

    // Parse all except first (executable name)
    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];

        if (!strcmp(arg, "--offscreen"))
        {
            cmdOpts.mForceOffscreen = true;
        }
        else if (!strcmp(arg, "--multithread"))
        {   cmdOpts.mMultiThread = true;
        }
        else if (!strcmp(arg, "--input"))
        {
            cmdOpts.mFileName = argv[++i];
            gotInput = true;
        }
        else if (!strcmp(arg, "--noscreen"))
        {
            cmdOpts.mPbufferRendering = true;
        }
        else if (!strcmp(arg, "--output"))
        {
            ffOptions.mOutputFileName = argv[++i];
            gotOutput = true;
        }
        else if (!strcmp(arg, "--comment"))
        {
            ffOptions.mComment = argv[++i];
        }
        else if (!strcmp(arg, "--targetFrame"))
        {
            ffOptions.mTargetFrame = readValidValue(argv[++i]);
            gotTargetFrame = true;
        }
        else if (!strcmp(arg, "--targetDrawCallNo"))
        {
            ffOptions.mTargetDrawCallNo = readValidValue(argv[++i]);
            gotTargetDrawCallNo = true;
            ffOptions.mEndFrame = UINT32_MAX;
        }
        else if (!strcmp(arg, "--endFrame"))
        {
            ffOptions.mEndFrame = readValidValue(argv[++i]);
            if (ffOptions.mEndFrame < ffOptions.mTargetFrame)
            {
                DBG_LOG("WARNING: the endFrame is less than targetFrame! Set endFrame to default to fastforward to the last frame.\n");
                ffOptions.mEndFrame = UINT32_MAX;
            }
        }
        else if (!strcmp(arg, "--norestoretex"))
        {
            ffOptions.mFlags &= ~FASTFORWARD_RESTORE_TEXTURES;
        }
        else if (!strcmp(arg, "--restorefbo0"))
        {
            ffOptions.mFlags |= FASTFORWARD_RESTORE_DEFAUTL_FBO;
            ffOptions.mFbo0Repeat = readValidValue(argv[++i]);
        }
        else if (!strcmp(arg, "--version"))
        {
            std::cout << "Version:" << std::endl;
            std::cout << "- Retracer: " PATRACE_VERSION << std::endl;
            std::cout << "- Fastforwarder: " << PATRACE_VERSION << std::endl;
            exit(0);
        }
        else
        {
            DBG_LOG("error: unknown option %s\n", arg);
            usage(argv[0]);
            return false;
        }
    }

    if (!gotOutput)
    {
        DBG_LOG("error: missing output file name\n");
    }

    if (!gotInput)
    {
        DBG_LOG("error: missing input file name\n");
    }

    if (!gotTargetFrame && !gotTargetDrawCallNo)
    {
        DBG_LOG("error: missing target frame number / draw call number\n");
    }

    if (false == (gotTargetFrame ^ gotTargetDrawCallNo))
    {
        DBG_LOG("error: please either indicate target frame number or draw call number\n");
    }

    bool success = gotOutput && gotInput && (gotTargetFrame ^ gotTargetDrawCallNo);
    if (!success)
    {
        usage(argv[0]);
    }
    return success;
}

extern "C"
int main(int argc, char** argv)
{
    FastForwardOptions ffOptions;
    if (!ParseCommandLine(argc, argv, /*out*/ ffOptions, gRetracer.mOptions))
    {
        return 1;
    }

    if (gRetracer.mOptions.mFileName.empty())
    {
        std::cerr << "No trace file name specified.\n";
        usage(argv[0]);
        return 1;
    }

    // Register Entries before opening tracefile as sigbook is read there
    common::gApiInfo.RegisterEntries(gles_callbacks);
    common::gApiInfo.RegisterEntries(egl_callbacks);

    // 1. Load defaults from file
    if (!gRetracer.OpenTraceFile(gRetracer.mOptions.mFileName.c_str()))
    {
        DBG_LOG("Failed to open %s\n", gRetracer.mOptions.mFileName.c_str());
        return 1;
    }

    // 3. init egl and gles, using final combination of settings (header + override)
    GLWS::instance().Init(gRetracer.mOptions.mApiVersion);

    // Create fastforward-part of trace-file JSON header
    Json::Value ffJson(Json::objectValue);

    // Prepare ffJson
    {
        // Target frame
        ffJson["originalFrame"] = ffOptions.mTargetFrame;
        // Target draw call no
        ffJson["targetDrawCallNo"] = ffOptions.mTargetDrawCallNo;
        // End Frame if not 0
        if (ffOptions.mEndFrame != UINT32_MAX)
            ffJson["endFrame"] = ffOptions.mEndFrame;

        // Misc.
        std::stringstream cmdlineSS;
        cmdlineSS << argv[0];
        for (int i = 1; i < argc; i++)
        {
            cmdlineSS << " " << argv[i];
        }
        ffJson["generationCommand"] = cmdlineSS.str();
        ffJson["comment"]           = ffOptions.mComment;

        // Restoration info
        Json::Value ffRestoreInfoJson(Json::objectValue);
        ffRestoreInfoJson["textures"] = (ffOptions.mFlags & FASTFORWARD_RESTORE_TEXTURES) ? true : false;
        ffRestoreInfoJson["buffers"]  = true;
        ffRestoreInfoJson["fbo0Repeat"] = ffOptions.mFbo0Repeat;
        ffJson["restoreOptions"] = ffRestoreInfoJson;

        // Version of fastforwarder and retracer
        Json::Value ffVersions(Json::objectValue);
        ffVersions["retracer"]      = PATRACE_VERSION;
        ffVersions["fastforwarder"] = PATRACE_VERSION;
        ffJson["versions"] = ffVersions;
    }

    // Open output file
    common::OutFile out(ffOptions.mOutputFileName.c_str());

    // Get existing header
    Json::Value jsonRoot = gRetracer.mFile.getJSONHeader();
    gRetracer.mOptions.mMultiThread = jsonRoot.get("multiThread", gRetracer.mOptions.mMultiThread).asBool();
    DBG_LOG("Multi-threading is %s\n", gRetracer.mOptions.mMultiThread ? "ON" : "OFF");

    // Do fastforwarding: pass ffJson in case we want to add anything
    retraceAndTrim(out, ffOptions, ffJson);

    // Add our conversion to the list
    addConversionEntry(jsonRoot, "fastforward", gRetracer.mOptions.mFileName, ffJson);

    // Serialize header
    Json::FastWriter writer;
    std::string jsonData = writer.write(jsonRoot);

    // Write header to file
    out.WriteHeader(jsonData.c_str(), jsonData.length());

    // Close and cleanup
    out.Close();
    GLWS::instance().Cleanup();

    return 0;
}

int ff_main(int argc, char** argv)
{
    return main(argc, argv);
}