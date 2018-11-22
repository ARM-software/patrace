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
    _access();
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
    _exit();
}

void StateMgr::InsertContextMap(int oldVal, Context* ctx)
{
    _access();
    mContextMap[oldVal] = ctx;
    ctx->retain();
    _exit();
}

Context* StateMgr::GetContext(int oldVal)
{
    _access();
    if (oldVal == 0)
    {
        _exit();
        return NULL;
    }

    const auto it = mContextMap.find(oldVal);
    Context* ctx = (it != mContextMap.end()) ? it->second : NULL;
    _exit();
    return ctx;
}

void StateMgr::RemoveContextMap(int oldVal)
{
    _access();
    const auto it = mContextMap.find(oldVal);

    if (it != mContextMap.end())
    {
        Context* ctx = it->second;
        if (ctx != NULL) ctx->release();
        mContextMap.erase(it);
    }
    _exit();
}

int StateMgr::GetCtx(Context* context)
{
    _access();
    for (std::unordered_map<int, Context*>::iterator it = mContextMap.begin(); it != mContextMap.end(); ++it)
    {
        if (it->second == context)
        {
            _exit();
            return it->first;
        }
    }
    _exit();

    return -1;
}

void StateMgr::InsertDrawableMap(int oldVal, Drawable* drawable)
{
    _access();
    mDrawableMap[oldVal] = drawable;
    if (drawable)
        drawable->retain();
    _exit();
}

void StateMgr::InsertDrawableToWinMap(int drawableVal, int winVal)
{
    _access();
    mDrawableToWinMap[drawableVal] = winVal;
    _exit();
}

int StateMgr::GetWin(int drawableVal)
{
    _access();
    if (drawableVal == 0)
    {
        _exit();
        return 0;
    }
    const auto it = mDrawableToWinMap.find(drawableVal);
    int win = (it != mDrawableToWinMap.end()) ? it->second : 0;
    _exit();
    return win;
}

Drawable* StateMgr::GetDrawable(int oldVal)
{
    _access();
    if (oldVal == 0) // EGL_NO_SURFACE
    {
        _exit();
        return 0;
    }

    const auto it = mDrawableMap.find(oldVal);
    Drawable* handler = (it != mDrawableMap.end()) ? it->second : NULL;
    _exit();
    return handler;
}

void StateMgr::RemoveDrawableMap(int oldVal)
{
    if (oldVal == 0) // EGL_NO_SURFACE
    {
        return;
    }

    _access();
    const auto found = mDrawableMap.find(oldVal);
    if (found != mDrawableMap.end())
    {
        Drawable* handler = found->second;
        if (handler) handler->release();
        mDrawableMap.erase(found);
    }
    _exit();
}

bool StateMgr::IsInDrawableMap(Drawable* drawable)
{
    _access();
    for (std::unordered_map<int, Drawable*>::iterator it = mDrawableMap.begin(); it != mDrawableMap.end(); ++it)
    {
        if (it->second == drawable)
        {
            _exit();
            return true;
        }
    }
    _exit();

    return false;
}

int StateMgr::GetDraw(Drawable* drawable)
{
    _access();
    for (std::unordered_map<int, Drawable*>::iterator it = mDrawableMap.begin(); it != mDrawableMap.end(); ++it)
    {
        if (it->second == drawable)
        {
            _exit();
            return it->first;
        }
    }
    _exit();

    return -1;
}

void StateMgr::InsertEGLImageMap(int oldVal, EGLImageKHR image)
{
    _access();
    mEGLImageKHRMap[oldVal] = image;
    _exit();
}

EGLImageKHR StateMgr::GetEGLImage(int oldVal, bool &found)
{
    _access();
    if (oldVal == 0) // EGL_NO_SURFACE
    {
        _exit();
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
    _exit();
    return handler;
}

void StateMgr::RemoveEGLImageMap(int oldVal)
{
    if (oldVal == 0) // EGL_NO_SURFACE
    {
        return;
    }

    _access();
    const auto found = mEGLImageKHRMap.find(oldVal);
    if (found != mEGLImageKHRMap.end())
    {
        mEGLImageKHRMap.erase(found);
    }
    _exit();
}

bool StateMgr::IsInEGLImageMap(EGLImageKHR image)
{
    _access();
    for (std::unordered_map<int, EGLImageKHR>::iterator it = mEGLImageKHRMap.begin(); it != mEGLImageKHRMap.end(); ++it)
    {
        if (it->second == image)
        {
            _exit();
            return true;
        }
    }
    _exit();

    return false;
}

} // namespace retracer

