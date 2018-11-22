#ifndef _COMMON_EGLSTATE_COMMON_
#define _COMMON_EGLSTATE_COMMON_

#include <string>
#include "base/base.hpp"

#define CLEAR_POINTER_ARRAY(array) \
    for (unsigned int i = 0; i < (array).size(); ++i) \
    { \
        delete (array)[i]; \
    } \
    (array).clear();

#define MAKE_ROOM_FOR_INDEX(array, index) \
    if ((array).size() <= (index)) \
        (array).resize((index) + 1, NULL);

#define ASSERT_ROOM_FOR_INDEX(array, index) \
    assert((array).size() > (index));

#define StateManagerInstance (state::StateManager::Instance())
#define FrameRecorderInstance (record::FrameRecorder::Instance())

// The meaning of enum is depend on the function where it's used in;
// For example, GL_POINTS and GL_NONE have same value. In glDrawArrays
// or glDrawElements, it means GL_POINTS. In glDrawBuffer and glGetError,
// it means GL_NONE.
const char * EnumString(unsigned int e, const std::string &funName = std::string());

struct Cube2D
{
    Cube2D()
    : x(0), y(0), width(0), height(0)
    {
    }
    Cube2D(unsigned int xx, unsigned int yy, unsigned int w, unsigned int h)
    : x(xx), y(yy), width(w), height(h)
    {
    }

    bool operator==(const Cube2D &other) const
    {
        return x == other.x && y == other.y &&
            width == other.width && height == other.height;
    }

    const std::string asString() const;
    unsigned int x, y, width, height;
};

namespace pat
{

struct ObjectID
{
    ObjectID(UInt32 h, UInt32 c)
    : handle(h), callNumber(c)
    {
    }

    bool operator<(const ObjectID & other) const
    {
        if (handle < other.handle)
            return true;
        else if (handle == other.handle && callNumber < other.callNumber)
            return true;
        else
            return false;
    }

    UInt32 handle;
    UInt32 callNumber; // The call number of creation API
};

#define RESOURCE_MANAGEMENT_DECLARATION(TYPE, type) \
public: \
    unsigned int TYPE##Count() const; \
    const TYPE *Get##TYPE(unsigned int index) const; \
    TYPE *Get##TYPE(unsigned int index); \
    const TYPE *Get##TYPE##Slot(unsigned int slot) const; \
    TYPE *Get##TYPE##Slot(unsigned int slot); \
    const TYPE *Bound##TYPE(unsigned int target) const; \
    TYPE *Bound##TYPE(unsigned int target); \
    void Create##TYPE(unsigned int slot); \
    void Destroy##TYPE(unsigned int slot); \
    void Bind##TYPE(unsigned int target, unsigned int slot); \
private: \
    std::vector<TYPE *> _##type##s; \
    std::vector<TYPE *> _##type##Slots; \
    std::map<unsigned int, unsigned int> _bound##TYPE##s; \
public: 

#define RESOURCE_MANAGEMENT_DEFINITION(TYPE, type) \
unsigned int StateManager::TYPE##Count() const \
{ \
    return _##type##s.size(); \
} \
 \
const TYPE * StateManager::Get##TYPE(unsigned int index) const \
{ \
    ASSERT_ROOM_FOR_INDEX(_##type##s, index); \
    return _##type##s[index]; \
} \
 \
TYPE * StateManager::Get##TYPE(unsigned int index) \
{ \
    ASSERT_ROOM_FOR_INDEX(_##type##s, index); \
    return _##type##s[index]; \
} \
 \
const TYPE * StateManager::Get##TYPE##Slot(unsigned int slot) const \
{ \
    ASSERT_ROOM_FOR_INDEX(_##type##Slots, slot); \
    return _##type##Slots[slot]; \
} \
 \
TYPE * StateManager::Get##TYPE##Slot(unsigned int slot) \
{ \
    ASSERT_ROOM_FOR_INDEX(_##type##Slots, slot); \
    return _##type##Slots[slot]; \
} \
 \
const TYPE * StateManager::Bound##TYPE(unsigned int target) const \
{ \
    std::map<unsigned int, unsigned int>::const_iterator found = _bound##TYPE##s.find(target); \
    if (found != _bound##TYPE##s.end()) \
    { \
        return Get##TYPE##Slot(found->second); \
    } \
    else \
    { \
        PAT_DEBUG_LOG("Invalid target for " #TYPE " binding : 0x%X\n", target); \
        return NULL; \
    } \
} \
 \
TYPE * StateManager::Bound##TYPE(unsigned int target) \
{ \
    std::map<unsigned int, unsigned int>::iterator found = _bound##TYPE##s.find(target); \
    if (found != _bound##TYPE##s.end()) \
    { \
        return Get##TYPE##Slot(found->second); \
    } \
    else \
    { \
        PAT_DEBUG_LOG("Invalid target for " #TYPE " binding : 0x%X\n", target); \
        return NULL; \
    } \
} \
 \
void StateManager::Create##TYPE(unsigned int slot) \
{ \
    MAKE_ROOM_FOR_INDEX(_##type##Slots, slot); \
    if (_##type##Slots[slot]) \
        Destroy##TYPE(slot); \
 \
    TYPE *newObj = new TYPE; \
    _##type##s.push_back(newObj); \
    _##type##Slots[slot] = newObj; \
} \
 \
void StateManager::Destroy##TYPE(unsigned int slot) \
{ \
    ASSERT_ROOM_FOR_INDEX(_##type##Slots, slot); \
    _##type##Slots[slot] = NULL; \
} \
 \
void StateManager::Bind##TYPE(unsigned int target, unsigned int slot) \
{ \
    if (slot >= _##type##Slots.size() || _##type##Slots[slot] == NULL) \
        Create##TYPE(slot); \
 \
    _bound##TYPE##s[target] = slot; \
    Bound##TYPE(target)->SetType(target); \
}

#define RESOURCE_MANAGEMENT_DUMP(TYPE, type) \
    if (StateManagerInstance->TYPE##Count()) \
    { \
        json.beginMember(#TYPE "s"); \
        json.beginArray(); \
        for (unsigned int i = 0; i < StateManagerInstance->TYPE##Count(); ++i) \
        { \
            const TYPE * obj = StateManagerInstance->Get##TYPE(i); \
            if (obj) \
                Dump##TYPE(json, obj); \
        } \
        json.endArray(); \
        json.endMember(); \
    }

};

#endif // _COMMON_EGLSTATE_COMMON_

