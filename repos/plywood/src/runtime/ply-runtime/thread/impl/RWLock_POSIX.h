/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <pthread.h>

namespace ply {

class RWLock_POSIX {
private:
    pthread_rwlock_t m_rwLock;

public:
    RWLock_POSIX() {
        pthread_rwlock_init(&m_rwLock, NULL);
    }

    ~RWLock_POSIX() {
        pthread_rwlock_destroy(&m_rwLock);
    }

    void lockExclusive() {
        pthread_rwlock_wrlock(&m_rwLock);
    }

    void unlockExclusive() {
        pthread_rwlock_unlock(&m_rwLock);
    }

    void lockShared() {
        pthread_rwlock_rdlock(&m_rwLock);
    }

    void unlockShared() {
        pthread_rwlock_unlock(&m_rwLock);
    }
};

} // namespace ply
