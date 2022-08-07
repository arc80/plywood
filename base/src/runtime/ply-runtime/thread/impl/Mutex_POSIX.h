/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <pthread.h>

namespace ply {

class Mutex_POSIX {
private:
    friend class ConditionVariable_POSIX;
    pthread_mutex_t m_mutex;

public:
    Mutex_POSIX() {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
#if defined(PTHREAD_MUTEX_RECURSIVE) || defined(__FreeBSD__)
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#else
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
#endif
        pthread_mutex_init(&m_mutex, &attr);
    }

    ~Mutex_POSIX() {
        pthread_mutex_destroy(&m_mutex);
    }

    void lock() {
        pthread_mutex_lock(&m_mutex);
    }

    bool tryLock() {
        return !pthread_mutex_trylock(&m_mutex);
    }

    void unlock() {
        pthread_mutex_unlock(&m_mutex);
    }
};

} // namespace ply
