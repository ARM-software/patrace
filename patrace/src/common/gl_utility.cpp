#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <unistd.h>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>

#include "common/gl_utility.hpp"
#include "common/os.hpp"
#include "dispatch/eglproc_auto.hpp"

#include <stdint.h>
#include <string.h>

bool getIndirectBuffer(void *params, size_t size, const void *indirect)
{
    GLint bufferId = 0;
    _glGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, &bufferId);
    if (bufferId == 0) // indirect buffer is in client memory (ugh)
    {
        memcpy(params, indirect, size);
    }
    else
    {
        GLint bufSize = 0;
        _glGetBufferParameteriv(GL_DRAW_INDIRECT_BUFFER, GL_BUFFER_SIZE, &bufSize);
        if ((size_t)bufSize < size + (GLintptr)indirect)
        {
            DBG_LOG("Buffer overrun reading indirect draw parameters!\n");
            return false;
        }
        void* mapPtr = _glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, (GLintptr)indirect, size, GL_MAP_READ_BIT);
        if (mapPtr)
        {
            memcpy(params, mapPtr, size);
            _glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);
        }
        else
        {
            DBG_LOG("Error reading out indirect draw parameters!\n");
            return false;
        }
    }
    return true;
}

void unmapBuffer()
{
    _glUnmapBuffer(GL_COPY_READ_BUFFER);
}

const char *mapBuffer(const std::string& rootname, unsigned bufferOffset, unsigned bufferSize, GLuint bufferId, const std::string& marker)
{
    _glBindBuffer(GL_COPY_READ_BUFFER, bufferId);
    const char *ptr = (const char *)_glMapBufferRange(GL_COPY_READ_BUFFER, bufferOffset, bufferSize, GL_MAP_READ_BIT);
    if (!ptr)
    {
        DBG_LOG("Failed to bind buffer %d\n", bufferId);
        abort();
    }
    std::string filename = rootname + "_" + marker + ".bin";
    FILE *fp = fopen(filename.c_str(), "w");
    if (!fp)
    {
        DBG_LOG("Failed to open %s: %s\n", filename.c_str(), strerror(errno));
        abort();
    }
    fwrite(ptr, bufferSize, 1, fp);
    fclose(fp);
    DBG_LOG("Successfully wrote %s\n", filename.c_str());
    return ptr;
}
