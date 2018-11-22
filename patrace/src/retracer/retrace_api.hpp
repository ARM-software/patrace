#ifndef _RETRACER_RETRACE_API_HPP_
#define _RETRACER_RETRACE_API_HPP_

#include <common/api_info.hpp>
#include <common/file_format.hpp>
#include <dispatch/eglimports.hpp>

namespace retracer {

typedef void (*RetraceFunc)(char*);

void ignore(char*);
void unsupported(char*);

extern const common::EntryMap gles_callbacks;
extern const common::EntryMap egl_callbacks;

}

void glCopyClientSideBuffer(GLenum target, unsigned int name);
void glPatchClientSideBuffer(GLenum target, int _size, const char* _data);
void glClientSideBufferData(unsigned int _name, int _size, const char* _data);
void glClientSideBufferSubData(unsigned int _name, int _offset, int _size, const char* _data);
void glCreateClientSideBuffer(unsigned int name);
void glDeleteClientSideBuffer(unsigned int _name);
unsigned int glGenGraphicBuffer_ARM(unsigned int _width, unsigned int _height, int _pix_format, unsigned int _usage);
void glGraphicBufferData_ARM(unsigned int _name, int _size, const char * _data);
void glDeleteGraphicBuffer_ARM(unsigned int _name);
void paMandatoryExtensions(int count, common::Array<const char*> string);

#endif
