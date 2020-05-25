/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/thread/impl/Mutex_POSIX.h>
#include <time.h>

namespace ply {

class ConditionVariable_POSIX {
private:
    pthread_cond_t m_condVar;

public:
    ConditionVariable_POSIX() {
        pthread_condattr_t attr;
        pthread_condattr_init(&attr);
        pthread_cond_init(&m_condVar, &attr);
    }

    ~ConditionVariable_POSIX() {
        pthread_cond_destroy(&m_condVar);
    }

    void wait(LockGuard<Mutex_POSIX>& guard) {
        pthread_cond_wait(&m_condVar, &guard.getMutex().m_mutex);
    }

#if !PLY_TARGET_MINGW
    void timedWait(LockGuard<Mutex_POSIX>& guard, ureg waitMillis) {
        if (waitMillis > 0) {
#if PLY_TARGET_APPLE
            struct timespec ts;
            ts.tv_sec += waitMillis / 1000;
            ts.tv_nsec += (waitMillis % 1000) * 1000000;
            pthread_cond_timedwait_relative_np(&m_condVar, &guard.getMutex().m_mutex, &ts);
#else
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += waitMillis / 1000;
            ts.tv_nsec += (waitMillis % 1000) * 1000000;
            if (ts.tv_nsec >= 1000000000) {
                ts.tv_nsec -= 1000000000;
                ts.tv_sec++;
            }
            pthread_cond_timedwait(&m_condVar, &guard.getMutex().m_mutex, &ts);
#endif
        }
    }
#endif

    void wakeOne() {
        pthread_cond_signal(&m_condVar);
    }

    void wakeAll() {
        pthread_cond_broadcast(&m_condVar);
    }
};

} // namespace ply
