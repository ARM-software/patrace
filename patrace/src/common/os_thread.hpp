/**************************************************************************
 *
 * Copyright 2011 Jose Fonseca
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/

/*
 * Simple OS abstraction.
 *
 * Mimics C++11 / boost threads.
 */

#ifndef _OS_THREAD_HPP_
#define _OS_THREAD_HPP_

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

#ifdef ANDROID
#define OWN_SPINLOCK
#endif

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#define OWN_SPINLOCK
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "os.hpp"
#include <vector>

namespace os {
    class Thread;
    class Mutex;
    class Condition;
    class Atomic;
    template <typename T> class MTQueue;
    template <typename T> class thread_specific_ptr;

    template <typename T>
    class thread_specific_ptr
    {
    private:
#ifdef _WIN32
        DWORD dwTlsIndex;
#else
        pthread_key_t key;

        static void destructor(void *ptr) {
            delete static_cast<T *>(ptr);
        }
#endif

    public:
        thread_specific_ptr(void) {
#ifdef _WIN32
            dwTlsIndex = TlsAlloc();
#else
            pthread_key_create(&key, &destructor);
#endif
        }

        ~thread_specific_ptr() {
#ifdef _WIN32
            TlsFree(dwTlsIndex);
#else
            pthread_key_delete(key);
#endif
        }

        T* get(void) const {
            void *ptr;
#ifdef _WIN32
            ptr = TlsGetValue(dwTlsIndex);
#else
            ptr = pthread_getspecific(key);
#endif
            return static_cast<T*>(ptr);
        }

        T* operator -> (void) const
        {
            return get();
        }

        T& operator * (void) const
        {
            return *get();
        }

        void reset(T* new_value=0) {
            T * old_value = get();
#ifdef _WIN32
            TlsSetValue(dwTlsIndex, new_value);
#else
            pthread_setspecific(key, new_value);
#endif
            if (old_value) {
                delete old_value;
            }
        }
    };

    class Atomic
    {
    public:
        Atomic();
        ~Atomic();
        void setValue(int);
        int  Value();
        void Add();
        void Minus();
        bool IsEqual(int v);
        bool operator == (int v);
        bool operator > (int v);
        bool operator < (int v);
        bool operator >= (int v);
        bool operator <= (int v);

    private:
        void init();
        void destroy();

        int  value;

#ifdef _WIN32
#else
        pthread_rwlock_t lock;
#endif
    };

    class Mutex
    {
    public:
        Mutex();
        ~Mutex();
        bool lock();
        bool trylock();
        bool unlock();

    private:
        void init();
        void destroy();

#ifdef _WIN32
#else
        pthread_mutex_t mutex;
#endif
    };

    class Condition
    {
    public:
        Condition(int initValue = 0);
        ~Condition();
        void setValue(int value);
        int  getValue();
        void signal();
        void broadcast();
        void wait();
        void inc();
        void dec();

    private:
        void init();
        void destroy();

#ifdef _WIN32
#else
        pthread_mutex_t mutex;
        pthread_cond_t  cond;
        Atomic ref;
#endif
    };

#ifdef _WIN32
#define THREAD_HANDLE HANDLE
#else
#define THREAD_HANDLE pthread_t
#endif
    class Thread
    {
    public:
        Thread();
        virtual  ~Thread();
        THREAD_HANDLE        getTid();
        static THREAD_HANDLE getCurrentThreadID();
        static bool IsSameThread(THREAD_HANDLE tid1, THREAD_HANDLE tid2);

        virtual bool start();
        virtual bool end();
        virtual void *waitUntilExit();
        virtual void _access()
        {
            mMutex.lock();
        }
        virtual void _exit()
        {
            mMutex.unlock();
        }

        virtual void OnStart()
        {
            mStarted = true;
        }

        virtual void OnExit()
        {
            mStarted = false;
        }

    private:
        static  void* start_thread(void* param);
        static  void thread_cleanup(void* param);
#ifdef ANDROID
        static  void handle_quit(int signo);
#endif
        virtual void run() = 0;

        Mutex   mMutex;
        THREAD_HANDLE tid;
        bool    mStarted;
#ifdef _WIN32
#else
        pthread_attr_t attr;
#endif
    };

    template<typename T>
    class MTQueue
    {
    public:
        enum { MAX_QUEUE_SIZE = 100 };

        MTQueue(int size = MAX_QUEUE_SIZE) :
            maxSize(size),
            usedCond(0),
            leftCond(size)
        {
            mQueue.resize(size);
            mQueue.clear();
        }

        ~MTQueue()
        {
            _exit();
        }

        T trypop()
        {
            T item = 0;
            if (!isEmpty())
            {
                _access();
                item = mQueue.front();
                mQueue.erase(mQueue.begin());
                usedCond.dec();
                leftCond.inc();
                leftCond.broadcast();
                _exit();
            }
            return item;
        }

        T pop()
        {
            waitNewItem();
            return trypop();
        }

        bool trypush(T item)
        {
            if (isFull())
                return false;
            _access();
            mQueue.push_back(item);
            usedCond.inc();
            usedCond.broadcast();
            leftCond.dec();
            _exit();
            return true;
        }

        bool push(T item)
        {
            waitEmptyPos();
            return trypush(item);
        }

        void erase(T item)
        {
            _access();
            int i = 0;
            for (auto it : mQueue)
            {
                if (item == it)
                {
                    mQueue.erase(mQueue.begin() + i);
                    usedCond.dec();
                    leftCond.inc();
                    leftCond.broadcast();
                    break;
                }
                i++;
            }
            _exit();
        }

        void resize(int size)
        {
            _access();
            mQueue.clear();
            mQueue.resize(size);
            mQueue.clear();
            maxSize = size;
            usedCond.setValue(0);
            usedCond.broadcast();
            leftCond.setValue(size);
            leftCond.broadcast();
            _exit();
        }

        int  size()
        {
            int size = 0;
            _access();
            size = mQueue.size();
            if (size != usedCond.getValue())
            {
                DBG_LOG("Unmatched size queue(%d) vs cond(%d)!\n", size, usedCond.getValue());
            }
            _exit();
            return size;
        }

        bool isFull()
        {
            return size() == maxSize;
        }

        bool isEmpty()
        {
            return size() == 0;
        }

        void waitNewItem()
        {
            if (isEmpty())
            {
                usedCond.wait();
            }
        }

        void waitEmptyPos()
        {
            if (isFull())
            {
                leftCond.wait();
            }
        }

        void wakeup()
        {
            usedCond.broadcast();
            leftCond.broadcast();
        }

        void setUsedCondWait()
        {
            usedCond.wait();
        }
    private:
        inline void _access()
        {
            mMutex.lock();
        }

        inline void _exit()
        {
            mMutex.unlock();
        }

        int       maxSize;
        Mutex     mMutex;
        Condition usedCond;
        Condition leftCond;
        std::vector<T> mQueue;
    };

} /* namespace os */

#endif /* _OS_THREAD_HPP_ */
