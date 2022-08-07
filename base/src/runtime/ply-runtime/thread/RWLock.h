/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

// clang-format off

// Choose default implementation if not already configured by ply_userconfig.h:
#if !defined(PLY_IMPL_RWLOCK_PATH)
    #if PLY_TARGET_WIN32
        #define PLY_IMPL_RWLOCK_PATH "impl/RWLock_Win32.h"
        #define PLY_IMPL_RWLOCK_TYPE ply::RWLock_Win32
    #elif PLY_TARGET_POSIX
        #define PLY_IMPL_RWLOCK_PATH "impl/RWLock_POSIX.h"
        #define PLY_IMPL_RWLOCK_TYPE ply::RWLock_POSIX
    #else
        #define PLY_IMPL_RWLOCK_PATH "*** Unable to select a default RWLock implementation ***"
    #endif
#endif

// Include the implementation:
#include PLY_IMPL_RWLOCK_PATH

// Alias it:
namespace ply {

typedef PLY_IMPL_RWLOCK_TYPE RWLock;

//---------------------------------------------------------
// SharedLockGuard
//---------------------------------------------------------
template <typename LockType> class SharedLockGuard {
private:
    LockType& m_lock;

public:
    SharedLockGuard(LockType& lock) : m_lock(lock) {
        m_lock.lockShared();
    }

    ~SharedLockGuard() {
        m_lock.unlockShared();
    }
};

//---------------------------------------------------------
// ExclusiveLockGuard
//---------------------------------------------------------
template <typename LockType> class ExclusiveLockGuard {
private:
    LockType& m_lock;

public:
    ExclusiveLockGuard(LockType& lock) : m_lock(lock) {
        m_lock.lockExclusive();
    }

    ~ExclusiveLockGuard() {
        m_lock.unlockExclusive();
    }
};

} // namespace ply
