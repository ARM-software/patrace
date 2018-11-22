#include <cstdio>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "vbo.hpp"

namespace pat
{

unsigned int BufferObject::NextID = 0;

BufferObject::BufferObject()
: _id(NextID++), _data(NULL), _size(0), _usage(GL_NONE), _type(GL_NONE)
{
}

BufferObject::~BufferObject()
{
    delete []_data;
    _data = NULL;
    _size = 0;
}

std::string BufferObject::IDString() const
{
    char buffer[128];
    sprintf(buffer, "BufferObject[%d]", _id);
    return buffer;
}

unsigned int BufferObject::GetUsage() const
{
    return _usage;
}

unsigned int BufferObject::GetSize() const
{
    return _size;
}

const void * BufferObject::GetData() const
{
    return _data;
}

unsigned int BufferObject::GetType() const
{
    return _type;
}

void BufferObject::SetUsage(unsigned int usage)
{
    _usage = usage;
}

void BufferObject::SetType(unsigned int type)
{
    _type = type;
}

void BufferObject::SetData(unsigned int size, const void *data)
{
    if (_data)
    {
        delete []_data;
        _data = NULL;
        _size = 0;
    }

    if (size)
    {
        _data = new unsigned char[size];
        _size = size;
        if (data)
            memcpy(_data, data, size);
        else
            memset(_data, 0, size);
    }
}

void BufferObject::SetSubData(unsigned int offset, unsigned int size, const void *data)
{
    assert(offset + size <= _size);
    if (data == NULL || size == 0)
        return;

    memcpy(_data + offset, data, size);
}

} // namespace state
