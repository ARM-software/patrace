#include "memory.hpp"
#include "os.hpp"

#include <cstdio>
#include <cassert>

namespace common
{

void PrintMemory(const void *mem, size_t len)
{
    printf("\nMEMORY PRINT BEGIN : %d >>>>>>>>>>>> {\n", (int)len);

    const unsigned char *begin = (const unsigned char*)mem;
    for (size_t i = 0; i < len; ++i)
    {
        printf("0x%X ", *(begin++));
        if ((i+1) % 8 == 0)
            printf("\n");
    }

    printf("\nMEMORY PRINT END : %d <<<<<<<<<<<<< }\n", (int)len);
}

void * ClientSideBufferObject::extend(const void *p, ptrdiff_t s)
{
    const void *new_base_address = PTR_DIFF(base_address, p) > (ptrdiff_t)(0) ? p : base_address;
    const ptrdiff_t size1 = PTR_DIFF(PTR_MOVE(p, s), new_base_address);
    const ptrdiff_t size2 = PTR_DIFF(PTR_MOVE(base_address, size), new_base_address);
    base_address = const_cast<void*>(new_base_address);
    size = size1 > size2 ? size1 : size2;
    return base_address;
}

VertexAttributeMemoryMerger::~VertexAttributeMemoryMerger()
{
    for (unsigned int i = 0; i < _memory_ranges.size(); ++i)
    {
        delete _memory_ranges[i];
    }
    _memory_ranges.clear();

    for (unsigned int i = 0; i < _attributes.size(); ++i)
    {
        delete _attributes[i];
    }
    _attributes.clear();
}

void VertexAttributeMemoryMerger::add_attribute(
    unsigned int    index,
    const void *    ptr,
    ptrdiff_t       data_size,
    int             size,
    unsigned int    type,
    bool            normalized,
    size_t          stride)
{
#if 0
    DBG_LOG("VertexAttributeMemoryMerger::add_attribute()\n");
    DBG_LOG("    index=%d\n", index);
    DBG_LOG("    ptr=%p\n", ptr);
    DBG_LOG("    data_size=%d\n", data_size);
    DBG_LOG("    size=%d\n", size);
    DBG_LOG("    type=%d\n", type);
    DBG_LOG("    normalized=%s\n", normalized?"true":"false");
    DBG_LOG("    stride=%d\n", stride);
#endif

    // Loop over available memory blocks, checking for interleaved attributes
    for (unsigned int i = 0; i < _memory_ranges.size(); ++i)
    {
        if (_memory_ranges[i]->overlap(ptr, data_size))
        {
            const void * orig_base_address = _memory_ranges[i]->base_address;
            const void * new_base_address = _memory_ranges[i]->extend(ptr, data_size);
            for (unsigned int j = 0; j < _attributes.size(); ++j)
            {
                if (_attributes[j]->base_address == orig_base_address)
                    _attributes[j]->update(new_base_address);
            }

            _attributes.push_back(new AttributeInfo(index, new_base_address, PTR_DIFF(ptr, new_base_address), size, type, normalized, stride));
            return;
        }
    }
    _memory_ranges.push_back(new ClientSideBufferObject(ptr, data_size));
    _attributes.push_back(new AttributeInfo(index, ptr, 0, size, type, normalized, stride));
}

} // namespace common
