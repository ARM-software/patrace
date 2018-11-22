#ifndef _STATE_VBO_HPP_
#define _STATE_VBO_HPP_

#include "common.hpp"

namespace pat
{

class BufferObject
{
public:
    BufferObject();
    ~BufferObject();

    std::string IDString() const;

    unsigned int GetUsage() const;
    unsigned int GetSize() const;
    const void * GetData() const;
    unsigned int GetType() const;
    
    void SetType(unsigned int type);
    void SetUsage(unsigned int usage);
    void SetData(unsigned int size, const void *data);
    void SetSubData(unsigned int offset, unsigned int size, const void *data);

private:
    static unsigned int NextID;

    unsigned int _id;
    unsigned char *_data;
    unsigned int _size;
    unsigned int _usage;
    unsigned int _type;
};

} // namespace state

#endif // _STATE_VBO_HPP_
