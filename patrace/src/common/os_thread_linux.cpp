#include "os_thread.hpp"

namespace os {

Thread::Thread()
{
    mStarted = false;
    while(pthread_attr_init(&attr) != 0);
}

Thread::~Thread()
{
    end();
    waitUntilExit();
    while(pthread_attr_destroy(&attr) != 0);
}

pthread_t Thread::getCurrentThreadID()
{
    pthread_t cur_id = pthread_self();
    return cur_id;
}

bool Thread::IsSameThread(THREAD_HANDLE tid1, THREAD_HANDLE tid2)
{
    if( pthread_equal(tid1, tid2) != 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

pthread_t Thread::getTid()
{
    return tid;
}

bool Thread::start()
{
    if(pthread_create(&tid, &attr, start_thread, (void *)this) != 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

#ifdef ANDROID
void Thread::handle_quit(int signo)
{
    pthread_exit(NULL);
}
#endif

void* Thread::start_thread(void* param)
{
    Thread *ptr = (Thread *)param;
#ifdef ANDROID
    signal(SIGQUIT, handle_quit);
#endif
    pthread_cleanup_push(thread_cleanup, param);
    ptr->OnStart();
    ptr->run();
    pthread_cleanup_pop(1);
    pthread_exit(NULL);
}

void Thread::thread_cleanup(void* param)
{
    Thread *ptr = (Thread *)param;
    ptr->OnExit();
}

bool Thread::end()
{
    if (mStarted)
    {
#ifdef ANDROID
        pthread_kill(tid, SIGQUIT);
#else
        pthread_cancel(tid);
#endif
    }
    return true;
}

void* Thread::waitUntilExit()
{
    void* returnValue = nullptr;
    if (mStarted)
    {
        pthread_join(tid, &returnValue);
    }
    return returnValue;
}

Mutex::Mutex()
{
    mutex = PTHREAD_MUTEX_INITIALIZER;
    init();
}

Mutex::~Mutex()
{
    unlock();
    destroy();
}

bool Mutex::lock()
{
    int ret = pthread_mutex_lock(&mutex);
    return ret == 0;
}

bool Mutex::trylock()
{
    int ret = pthread_mutex_trylock(&mutex);
    return ret == 0;
}

bool Mutex::unlock()
{
    int ret = pthread_mutex_unlock(&mutex);
    return ret == 0;
}

void Mutex::init()
{
    pthread_mutex_init(&mutex, NULL);
}

void Mutex::destroy()
{
    pthread_mutex_destroy(&mutex);
}

Condition::Condition(int initValue)
{
    mutex = PTHREAD_MUTEX_INITIALIZER;
    cond  = PTHREAD_COND_INITIALIZER;
    ref.setValue(initValue);
    init();
}

Condition::~Condition()
{
    destroy();
}

void Condition::setValue(int value)
{
    ref.setValue(value);
}

int Condition::getValue()
{
    return ref.Value();
}

void Condition::signal()
{
    pthread_mutex_lock(&mutex);
    while(pthread_cond_signal(&cond) != 0);
    pthread_mutex_unlock(&mutex);
}

void Condition::inc()
{
    ref.Add();
}

void Condition::dec()
{
    ref.Minus();
}

void Condition::broadcast()
{
    pthread_mutex_lock(&mutex);
    while(pthread_cond_broadcast(&cond) != 0);
    pthread_mutex_unlock(&mutex);
}

void Condition::wait()
{
    pthread_mutex_lock(&mutex);
    if ( ref < 1 )
    {
        pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_unlock(&mutex);
}

void Condition::init()
{
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
}

void Condition::destroy()
{
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
}

Atomic::Atomic()
{
    value = 0;
    init();
}

Atomic::~Atomic()
{
    value = 0;
    destroy();
}

void Atomic::setValue(int v)
{
    pthread_rwlock_wrlock(&lock);
    value = v;
    pthread_rwlock_unlock(&lock);
}

int  Atomic::Value()
{
    int ret = 0;
    pthread_rwlock_rdlock(&lock);
    ret = value;
    pthread_rwlock_unlock(&lock);
    return ret;
}

void Atomic::Add()
{
    pthread_rwlock_wrlock(&lock);
    value++;
    pthread_rwlock_unlock(&lock);
}

void Atomic::Minus()
{
    pthread_rwlock_wrlock(&lock);
    value--;
    pthread_rwlock_unlock(&lock);
}

bool Atomic::IsEqual(int v)
{
    int ret = Value();
    return ret == v;
}

bool Atomic::operator == (int v)
{
    return IsEqual(v);
}

bool Atomic::operator > (int v)
{
    int ret = Value();
    return ret > v;
}

bool Atomic::operator < (int v)
{
    int ret = Value();
    return ret < v;
}

bool Atomic::operator >= (int v)
{
    int ret = Value();
    return ret >= v;
}

bool Atomic::operator <= (int v)
{
    int ret = Value();
    return ret <= v;
}

void Atomic::init()
{
    pthread_rwlock_init(&lock, NULL);
}

void Atomic::destroy()
{
    pthread_rwlock_destroy(&lock);
}

}
