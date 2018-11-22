#pragma once

#include <unordered_map>
#include <string>
#include <list>

#include <GLES3/gl32.h>

// This file contains helper that do not call any GL functions. Used for example for static analysis tools.

// Very simple LRU cache implementation, for simulating and testing cache behaviours.
template<typename K, typename V>
class LRU_Cache
{
public:
    typedef typename std::pair<K, V> pair_t;
    typedef typename std::list<pair_t> list_t;
    typedef typename list_t::iterator list_iterator_t;

    LRU_Cache(int size) : mSize(size) {}

    void put(const K& key, const V& value)
    {
        auto it = mCacheMap.find(key);
        mCacheList.push_front(pair_t(key, value));
        if (it != mCacheMap.end()) // already exists, remove duplicate
        {
            mCacheList.erase(it->second);
            mCacheMap.erase(it);
        }
        mCacheMap[key] = mCacheList.begin();
        if (mCacheMap.size() > mSize) // evict something from cache
        {
            mCacheMap.erase(mCacheList.back().first);
            mCacheList.pop_back();
        }
    }

    const V& get(const K& key)
    {
        auto it = mCacheMap.find(key);
        mCacheList.splice(mCacheList.begin(), mCacheList, it->second);
        return it->second->second;
    }

    bool exists(const K& key) const
    {
        return mCacheMap.find(key) != mCacheMap.end();
    }

    int age(const K& key) const // returns age of entry; if not found, returns size of cache
    {
        int i = 0;
        for (const auto& v : mCacheList)
        {
            if (v.first == key)
            {
                return i;
            }
            i++;
        }
        return mSize;
    }

private:
    unsigned mSize = 0;
    list_t mCacheList;
    std::unordered_map<K, list_iterator_t> mCacheMap;
};

const std::string shader_extension(GLenum type);
long calculate_primitives(GLenum mode, long vertices, GLuint patchSize);
GLenum interpret_texture_target(GLenum target);
bool isUniformSamplerType(GLenum type);
