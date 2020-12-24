#ifndef _INCLUDE_MEMORY_
#define _INCLUDE_MEMORY_

#include <cstddef>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <map>
#include <set>
#include "md5/md5.h"

#include <common/os.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace common
{

static char const hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B','C','D','E','F'};
static char const hex_lower[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b','c','d','e','f'};

#define PTR_DIFF(ptr, base) ((const char *)(ptr) - (const char *)(base))
#define DIFF_TO_PTR(i) ((const char *)(0) + (i))
#define PTR_TO_DIFF(p) ((const char *)(p) - (const char *)(0))
#define PTR_MOVE(ptr, diff) ((const char *)(ptr) + (diff))

void PrintMemory(const void *mem, size_t len);

typedef unsigned int ClientSideBufferObjectName;

struct MD5Digest
{
public:
    enum { DIGEST_LEN = 16 };

    MD5Digest()
    {
        memset(_digest, 0, DIGEST_LEN);
    }

    MD5Digest(const void* str, int length)
    {
        md5_state_t mdContext;
        md5_init(&mdContext);
        if (str)
            md5_append(&mdContext, static_cast<const unsigned char*>(str), length);
        md5_finish(&mdContext, _digest);
    }

    MD5Digest(const std::string& str) : MD5Digest(str.c_str(), str.size()) {}

    MD5Digest(const std::vector<std::string>& strlist)
    {
        md5_state_t mdContext;
        md5_init(&mdContext);
        for (unsigned i = 0; i < strlist.size(); ++i)
        {
             md5_append(&mdContext, (const unsigned char*)strlist[i].c_str(), strlist[i].size());
        }
        md5_finish(&mdContext, _digest);
    }

    MD5Digest(void* ptr, int stride, int sizePerElem, int count)
    {
        md5_state_t mdContext;
        md5_init(&mdContext);

        if (!stride)
        {
            stride = sizePerElem;
        }

        unsigned char* charPtr = static_cast<unsigned char*>(ptr);
        unsigned char* charPtrEnd = charPtr + stride * count;

        for (unsigned char* p = charPtr; p != charPtrEnd; p += stride)
        {
            md5_append(&mdContext, p, sizePerElem);
        }

        md5_finish(&mdContext, _digest);
    }

    const char *data() { return (const char *)_digest; }

    const std::string text() const
    {
        std::string str;
        str.reserve(DIGEST_LEN * 2);
        for (int i = 0; i < DIGEST_LEN; ++i) {
            const char ch = _digest[i];
            str.append(&hex[(ch  & 0xF0) >> 4], 1);
            str.append(&hex[ch & 0xF], 1);
        }
        return str;
    }

    const std::string text_lower() const
    {
        std::string str;
        str.reserve(DIGEST_LEN * 2);
        for (int i = 0; i < DIGEST_LEN; ++i) {
            const char ch = _digest[i];
            str.append(&hex_lower[(ch  & 0xF0) >> 4], 1);
            str.append(&hex_lower[ch & 0xF], 1);
        }
        return str;
    }

    operator unsigned char *() { return _digest; }
    operator const unsigned char *() const { return _digest; }

    bool operator==(const MD5Digest &other) const
    {
        return memcmp(_digest, other._digest, DIGEST_LEN) == 0;
    }
    bool operator!=(const MD5Digest &other) const
    {
        return memcmp(_digest, other._digest, DIGEST_LEN) != 0;
    }
    bool operator<(const MD5Digest &other) const
    {
        return memcmp(_digest, other._digest, DIGEST_LEN) < 0;
    }

private:
    unsigned char _digest[DIGEST_LEN];
};

inline std::ostream& operator<<(std::ostream& o, const MD5Digest& md)
{
    std::ios::fmtflags f = std::cout.flags();

    for (int i = 0; i < MD5Digest::DIGEST_LEN; ++i)
    {
        o << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(md[i]);
    }

    std::cout.flags(f);
    return o;
}

struct CSBPatch
{
    unsigned int offset;
    unsigned int length;    // in byte
};

struct CSBPatchList
{
    unsigned int count;
};

// Represents a contiguous memory range
class ClientSideBufferObject
{
public:
    ~ClientSideBufferObject()
    {
        if (_own_memory)
            delete [] static_cast<const char*>(base_address);
        base_address = NULL;
    }

    ClientSideBufferObject()
    : base_address(NULL), size(0), _own_memory(false)
    , _destinationAddress(0)
    {
    }

    ClientSideBufferObject(const void *p, ptrdiff_t s, bool copy = false)
    : base_address(NULL), size(0), _own_memory(false)
    , _destinationAddress(0)
    {
        set_data(p, s, copy);
    }

    ClientSideBufferObject(const ClientSideBufferObject &other)
    : base_address(NULL), size(0), _own_memory(false)
    , _destinationAddress(0)
    {
        *this = other;
    }

    ClientSideBufferObject & operator=(const ClientSideBufferObject &other)
    {
        if (this == &other)
            return *this;

        set_data(other.base_address, other.size, other._own_memory);
        return *this;
    }

    bool operator==(const ClientSideBufferObject &other) const
    {
        return size == other.size && md5_digest() == other.md5_digest();
    }

    void set_data(const void *p, ptrdiff_t s, bool copy = false)
    {
        if (base_address && _own_memory)
        {
            delete [](static_cast<char *>(base_address));
            base_address = NULL;
            _own_memory = false;
        }

        if (copy)
        {
            char* buf = static_cast<char*>(_destinationAddress);

            // If _destinationAddress is 0, we allocate our own destionation.
            if (!buf)
            {
                buf = new char[s];
                _own_memory = true;
            }

            if (p)
            {
                memcpy(buf, p, s);
            }

            base_address = buf;
        }
        else
        {
            base_address = const_cast<void *>(p);
        }
        size = s;
        _dirty_md5_digest = true;

        if (!_own_memory)
        {
            // If we don't own the memory referenced, meaning we also don't
            // control the lifetime of it, we calculate the MD5 digest now as
            // the referenced memory might be invalidated at any time.
            calculate_md5_digest();
        }
    }

    void set_subdata(const void *p, ptrdiff_t offset, ptrdiff_t s)
    {
        if (_own_memory == false)
        {
            DBG_LOG("Can not set the sub-data of a client-side buffer object which does not own its memory.\n");
            return;
        }

        if (offset + s > size)
        {
            DBG_LOG("The range of sub data (offset %.td size %.td) exceeds the whole range (full size %.td) of the client-side buffer object.\n", offset, s, size);
            return;
        }

        if (p)
        {
            memcpy(static_cast<char*>(base_address) + offset, p, s);
        }
        _dirty_md5_digest = true;
    }

    // Whether these two contiguous memory regions overlap
    bool overlap(const void *p, ptrdiff_t s) const { return (PTR_DIFF(p, base_address) < size) && (PTR_DIFF(base_address, p) < s); }

    // Extend this memory region to contain another contiguous memory region, and return the new base address
    void * extend(const void *p, ptrdiff_t size);

    const MD5Digest md5_digest() const
    {
        if (_dirty_md5_digest) calculate_md5_digest();
        return _md5_digest;
    }

    void * translate_address(ptrdiff_t offset) const
    {
        if (offset < size)
        {
            return const_cast<char*>(PTR_MOVE(base_address, offset));
        }
        else if (size == 0)
        {
            return NULL;
        }
        else
        {
            DBG_LOG("Invalid offset %.td for client-side buffer object with length %.td\n", offset, size);
            return NULL;
        }
    }

    void setDestinationAddress(void* address)
    {
        _destinationAddress = address;
    }

    void * base_address;
    ptrdiff_t size;

private:
    // If own its memory, should delete it in the destructor
    bool _own_memory;

    // Cached MD5 digest
    mutable bool _dirty_md5_digest = true;
    mutable MD5Digest _md5_digest;

    // If != 0, this will be used as destination by set_data
    // This is used by the glReadMapBufferRange, and glUnmapBuffer functiosn.
    void* _destinationAddress;

    void calculate_md5_digest() const
    {
        _md5_digest = MD5Digest(base_address, size);
        _dirty_md5_digest = false;
    }
};

// Try to merge memory range of multiple vertex attributes for one draw call into a contiguous memory region
class VertexAttributeMemoryMerger
{
public:
    struct AttributeInfo
    {
        AttributeInfo(unsigned int i, const void * ba, ptrdiff_t of,
            int s, unsigned int t, bool nor, size_t st)
            : index(i), base_address(ba), offset(of),
              size(s), type(t), normalized(nor), stride(st)
        {
        }

        void update(const void *new_base_address)
        {
            offset = PTR_DIFF(PTR_MOVE(base_address, offset), new_base_address);
            base_address = new_base_address;
        }

        unsigned int index;
        const void * base_address;
        ptrdiff_t offset;
        int size;
        unsigned int type;
        bool normalized;
        size_t stride;
        ClientSideBufferObjectName obj_name;
    };

    VertexAttributeMemoryMerger() {}
    ~VertexAttributeMemoryMerger();

    unsigned int memory_range_count() const { return _memory_ranges.size(); }
    const ClientSideBufferObject* memory_range(size_t i) const { return _memory_ranges[i]; }
    unsigned int attribute_count() const { return _attributes.size(); }
    const AttributeInfo *attribute(size_t i) const { return _attributes[i]; }

    void add_attribute(unsigned int index, const void *ptr, ptrdiff_t data_size, int size, unsigned int type, bool normalized, size_t stride);

private:
    std::vector<ClientSideBufferObject *> _memory_ranges;
    std::vector<AttributeInfo *> _attributes;
};

class ClientSideBufferObjectSetPerThread
{
public:
    ClientSideBufferObjectSetPerThread()
    {
        _objects.emplace(0, new ClientSideBufferObject);   // a sentinel for being compatible with old traces
    }

    ~ClientSideBufferObjectSetPerThread()
    {
        for (auto &iter : _objects)
        {
            delete iter.second;
        }
    }

#ifdef RETRACE
    void create_object(ClientSideBufferObjectName name)
    {
        _objects.emplace(name, new ClientSideBufferObject);
    }
#else
    ClientSideBufferObjectName create_object()
    {
        _objects.emplace(_objects.size() + 1, new ClientSideBufferObject);
        return _objects.size();
    }
#endif

    void delete_object(ClientSideBufferObjectName name)
    {
        ClientSideBufferObjectList::iterator iter = _objects.find(name);
        if (iter != _objects.end())
        {
            delete _objects.at(name);
            _objects.at(name) = NULL;
            return;
        }

        DBG_LOG("Invalid client-side buffer name to be deleted : %d\n", name);
    }

    void object_data(ClientSideBufferObjectName name, int size, const void *data, bool copy = false)
    {
        ClientSideBufferObjectList::iterator iter = _objects.find(name);
        if (iter == _objects.end())
        {
            _objects.emplace(name, new ClientSideBufferObject);
        }
        _objects[name]->set_data(data, size, copy);
    }

    void object_subdata(ClientSideBufferObjectName name, int offset, int size, const void* data)
    {
        ClientSideBufferObjectList::iterator iter = _objects.find(name);
        if (iter == _objects.end())
        {
            DBG_LOG("Invalid client-side buffer name to set sub-data : %d\n", name);
        }
        _objects[name]->set_subdata(data, offset, size);
    }

    ClientSideBufferObject *get_object(ClientSideBufferObjectName name) const
    {
        ClientSideBufferObjectList::const_iterator iter = _objects.find(name);
        if (iter != _objects.end())
        {
            return _objects.at(name);
        }
        return NULL;
    }

    bool find(const ClientSideBufferObject &obj, ClientSideBufferObjectName &name) const
    {
        ClientSideBufferObjectList::const_iterator iter;
        for (iter = _objects.begin(); iter != _objects.end(); iter++)
        {
            if (iter->second && *(iter->second) == obj)
            {
                name = iter->first;
                return true;
            }
        }
        return false;
    }

    size_t total_size() const
    {
        size_t sum = 0;
        ClientSideBufferObjectList::const_iterator iter;
        for (iter = _objects.begin(); iter != _objects.end(); iter++)
        {
            if (iter->second)
            {
                sum += iter->second->size;
            }
        }
        return sum;
    }

private:
    typedef std::unordered_map<unsigned int, ClientSideBufferObject*> ClientSideBufferObjectList;
    ClientSideBufferObjectList _objects;
};

class ClientSideBufferObjectSet
{
public:
    ClientSideBufferObjectSet() {}

#ifdef RETRACE
    void create_object(unsigned int tid, ClientSideBufferObjectName name)
    {
        _per_threads[tid].create_object(name);
    }
#else
    // For thread N, create a new object and return the name
    ClientSideBufferObjectName create_object(unsigned int tid)
    {
        return _per_threads[tid].create_object();
    }
#endif

    // For thread N, delete the object with the specific name
    void delete_object(unsigned int tid, ClientSideBufferObjectName name)
    {
        _per_threads[tid].delete_object(name);
    }

    // For thread N, set the data of the object with the specific name
    // If copy equals true, the content in the memory range will be copied
    void object_data(unsigned int tid, ClientSideBufferObjectName name,
        int size, const void *data, bool copy = false)
    {
        _per_threads[tid].object_data(name, size, data, copy);
    }

    // For thread N, set the sub-data of the object with the specific name
    void object_subdata(unsigned int tid, ClientSideBufferObjectName name,
        int offset, int size, const void* data)
    {
        _per_threads[tid].object_subdata(name, offset, size, data);
    }

    // For thread N, return the object with the specific name;
    // Return NULL if can not find.
    ClientSideBufferObject *get_object(unsigned int tid, ClientSideBufferObjectName name) const
    {
        PerThreadMap::const_iterator citer =
            _per_threads.find(tid);
        if (citer == _per_threads.end())
            return NULL;
        else
            return citer->second.get_object(name);
    }

    // For thread N, try to find a object with the same memory length and checksum and return its name;
    // Otherwise, return 0.
    bool find(unsigned int tid, const ClientSideBufferObject &obj, ClientSideBufferObjectName &name) const
    {
        PerThreadMap::const_iterator citer =
            _per_threads.find(tid);
        if (citer == _per_threads.end())
            return false;
        else
            return citer->second.find(obj, name);
    }

    void *translate_address(unsigned int tid, ClientSideBufferObjectName name, ptrdiff_t offset) const
    {
        const ClientSideBufferObject *obj = get_object(tid, name);
        if (obj == NULL)
        {
            //DBG_LOG("Invalid client-side buffer name to translate memory address : %d\n", name);
            return NULL;
        }
        else
        {
            return obj->translate_address(offset);
        }
    }

    size_t total_size(unsigned int tid) const
    {
        PerThreadMap::const_iterator citer =
            _per_threads.find(tid);
        if (citer == _per_threads.end())
            return 0;
        else
            return citer->second.total_size();
    }

    void clear()
    {
        _per_threads.clear();
    }

private:
    typedef std::map<unsigned int, ClientSideBufferObjectSetPerThread> PerThreadMap;
    PerThreadMap _per_threads;
};

} // namespace common

#endif // _INCLUDE_MEMORY_
