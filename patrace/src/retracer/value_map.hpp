#ifndef _RETRACER_HANDLE_MAP_HPP_
#define _RETRACER_HANDLE_MAP_HPP_

#include <cstring>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <unordered_map>

namespace retracer {

template <class K, class V>
class stdmap {
public:
    stdmap()
        : mNull(0)
    {}

    inline V& LValue(const K& key)
    {
        const auto it = mMap.find(key);
        if (it == mMap.end())
        {
            mMap.insert(std::pair<K, V>(key, mNull));
        }
        return mMap.at(key);
    }

    const inline V& RValue(const K& key) const
    {
        const auto it = mMap.find(key);
        if (it == mMap.end())
        {
            return mNull;
        }
        else
        {
            return it->second;
        }
    }

private:
    std::unordered_map<K, V> mMap;
    V mNull;
};

template <class T>
class hmap {
private:
    static const unsigned int KEY_LIMIT = 10*1024;
    // map for small keys (< KEY_LIMIT)
    T *mpData;
    // map for large keys (>= KEY_LIMIT)
    std::unordered_map<T, T> mMap;

    unsigned int mSize;
    T mNull;

public:
    hmap():mpData(NULL), mSize(0)
    {
        resize(128);
        mNull = 0;
    }

    ~hmap()
    {
        delete [] mpData;
    }

    std::unordered_map<T, T> GetCopy()
    {
        std::unordered_map<T, T> newMap = mMap;

        for(size_t i = 0; i < mSize; i++)
        {
            if (mpData[i] != 0)
            {
                newMap[i] = mpData[i];
            }
        }

        return newMap;
    }

    inline T& LValue(const T& key)
    {
        if (key < KEY_LIMIT) {
            if (((unsigned int)key) >= mSize)
                resize(key);
            return mpData[key];
        }
        const auto it = mMap.find(key);
        if (it == mMap.end())
        {
            mMap.insert(std::pair<T, T>(key, mNull));
        }
        return mMap.at(key);
    }

    inline const T& RValue(const T& key) const
    {
        if (key < KEY_LIMIT) {
            if (((unsigned int)key) >= mSize)
                return mNull;
            return mpData[key];
        }
        //DBG_LOG("Map index (%u) larger than KEY_LIMIT, using map instead of array.\n", (unsigned)key);
        const auto it = mMap.find(key);
        if (it == mMap.end())
        {
            return mNull;
        }
        else
        {
            return it->second;
        }
    }

    void resize(unsigned int sz)
    {
        if (sz < mSize)
            return;

        int newSz = 1;
        for (; sz ; sz>>=1, newSz<<=1)
            ;

        newSz <<= 2;

        T *newData = new T[newSz];
        if (mpData)
            memcpy(newData, mpData, mSize*sizeof(T));
        memset(newData+mSize, 0, (newSz-mSize)*sizeof(T));
        delete [] mpData;
        mpData = newData;
        mSize = newSz;
    }
};

// The logic in this map is specific for <attribute location> and <uniform location>.
class locationmap {
private:
    int *mpData;
    unsigned int mSize;

public:
    locationmap():mpData(NULL), mSize(0)
    {
        resize(128);
    }

    locationmap(const locationmap& b)
    {
        mSize = b.mSize;
        mpData = new int[mSize];
        for (unsigned int i = 0; i < mSize; ++i)
            mpData[i] = b.mpData[i];
    }

    ~locationmap()
    {
        delete [] mpData;
    }

    locationmap& operator=(const locationmap& b)
    {
        delete [] mpData;

        mSize = b.mSize;
        mpData = new int[mSize];
        for (unsigned int i = 0; i < mSize; ++i)
            mpData[i] = b.mpData[i];

        return *this;
    }

    inline int& LValue(const int& key)
    {
        if (((unsigned int)key) >= mSize)
            resize((unsigned int)key);
        return mpData[key];
    }

    inline const int& RValue(const int& key) const
    {
        if (((unsigned int)key) >= mSize)
            return key;
        return mpData[key];
    }

    void resize(unsigned int sz)
    {
        if (sz < mSize)
            return;

        int newSz = 1;
        for (; sz ; sz>>=1, newSz<<=1)
            ;

        newSz <<= 2;

        int *newData = new int[newSz];
        if (mpData)
            memcpy(newData, mpData, mSize*sizeof(int));
        for (int i = mSize; i < newSz; ++i)
            newData[i] = i;
        delete [] mpData;
        mpData = newData;
        mSize = newSz;
    }
};

}

#endif
