#include <GLES2/gl2.h>
#include <set>

#include "retracer/state.hpp"
#include "retracer/retracer.hpp"
#include "retracer/forceoffscreen/offscrmgr.h"

namespace retracer {

template <typename K, typename V>
void deleteUnique(std::unordered_map<K, V>& map)
{
    // Build up a set, containing the distinct elements that will be deleted.
    std::set<V> toBeDeleted;

    for (const auto it : map)
    {
        toBeDeleted.insert(it.second);
    }
    map.clear();

    // Delete the elements in the set
    for (auto it : toBeDeleted)
    {
        if (it != NULL) it->release();
    }
    toBeDeleted.clear();
}

StateMgr::StateMgr()
 : mThreadArr(PATRACE_THREAD_LIMIT)
 , mSingleSurface(0)
 , mForceSingleWindow(false)
 , mDrawableMap()
 , mContextMap()
 , mEGLImageKHRMap()
{}


void StateMgr::Reset()
{
    mForceSingleWindow = false;
    mSingleSurface = 0;
    mEGLImageKHRMap.clear();

    // Drawable and Context can occur multiple times in the mDrawableMap and mContextMap.
    // To prevent double deletion, which would have occured if we deleted all the values
    // in these maps, we need to call the deleteUnique function.
    deleteUnique(mDrawableMap);
    deleteUnique(mContextMap);

    // These can be references by the above, so they are handled last.
    for (auto& thread : mThreadArr)
    {
        thread.Reset();
    }
}

void StateMgr::InsertContextMap(int oldVal, Context* ctx)
{
    mContextMap[oldVal] = ctx;
    ctx->retain();
}

Context* StateMgr::GetContext(int oldVal) const
{
    if (oldVal == 0)
    {
        return NULL;
    }

    const auto it = mContextMap.find(oldVal);
    Context* ctx = (it != mContextMap.end()) ? it->second : NULL;
    return ctx;
}

void StateMgr::RemoveContextMap(int oldVal)
{
    const auto it = mContextMap.find(oldVal);

    if (it != mContextMap.end())
    {
        Context* ctx = it->second;
        if (ctx != NULL) ctx->release();
        mContextMap.erase(it);
    }
}

int StateMgr::GetCtx(Context* context) const
{
    for (const auto& it : mContextMap)
    {
        if (it.second == context)
        {
            return it.first;
        }
    }

    return -1;
}

void StateMgr::InsertDrawableMap(int oldVal, Drawable* drawable)
{
    mDrawableMap[oldVal] = drawable;
    if (drawable)
        drawable->retain();
}

void StateMgr::InsertDrawableToWinMap(int drawableVal, int winVal)
{
    mDrawableToWinMap[drawableVal] = winVal;
}

int StateMgr::GetWin(int drawableVal) const
{
    if (drawableVal == 0)
    {
        return 0;
    }
    const auto it = mDrawableToWinMap.find(drawableVal);
    int win = (it != mDrawableToWinMap.end()) ? it->second : 0;
    return win;
}

Drawable* StateMgr::GetDrawable(int oldVal) const
{
    if (oldVal == 0) // EGL_NO_SURFACE
    {
        return 0;
    }

    const auto it = mDrawableMap.find(oldVal);
    Drawable* handler = (it != mDrawableMap.end()) ? it->second : NULL;
    return handler;
}

void StateMgr::RemoveDrawableMap(int oldVal)
{
    if (oldVal == 0) // EGL_NO_SURFACE
    {
        return;
    }

    const auto found = mDrawableMap.find(oldVal);
    if (found != mDrawableMap.end())
    {
        Drawable* handler = found->second;
        if (handler) handler->release();
        mDrawableMap.erase(found);
    }
}

bool StateMgr::IsInDrawableMap(Drawable* drawable) const
{
    for (const auto& it : mDrawableMap)
    {
        if (it.second == drawable)
        {
            return true;
        }
    }

    return false;
}

int StateMgr::GetDraw(Drawable* drawable) const
{
    for (const auto& it : mDrawableMap)
    {
        if (it.second == drawable)
        {
            return it.first;
        }
    }

    return -1;
}

void StateMgr::InsertEGLImageMap(int oldVal, EGLImageKHR image)
{
    mEGLImageKHRMap[oldVal] = image;
}

EGLImageKHR StateMgr::GetEGLImage(int oldVal, bool &found) const
{
    if (oldVal == 0) // EGL_NO_SURFACE
    {
        return NULL;
    }

    const auto it = mEGLImageKHRMap.find(oldVal);
    EGLImageKHR handler;
    if (it != mEGLImageKHRMap.end())
    {
        handler = it->second;
        found = true;
    }
    else
    {
        handler = NULL;
        found = false;
    }
    return handler;
}

void StateMgr::RemoveEGLImageMap(int oldVal)
{
    if (oldVal == 0) // EGL_NO_SURFACE
    {
        return;
    }

    const auto found = mEGLImageKHRMap.find(oldVal);
    if (found != mEGLImageKHRMap.end())
    {
        mEGLImageKHRMap.erase(found);
    }
}

bool StateMgr::IsInEGLImageMap(EGLImageKHR image) const
{
    for (const auto& it : mEGLImageKHRMap)
    {
        if (it.second == image)
        {
            return true;
        }
    }

    return false;
}

} // namespace retracer

