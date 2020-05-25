/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {
template <typename LockType>
class LockGuard;
} // namespace ply

// clang-format off

// Choose default implementation if not already configured by ply_userconfig.h:
#if !defined(PLY_IMPL_MUTEX_PATH)
    #if PLY_PREFER_CPP11
        #define PLY_IMPL_MUTEX_PATH "impl/Mutex_CPP11.h"
        #define PLY_IMPL_MUTEX_TYPE ply::Mutex_CPP11
    #elif PLY_TARGET_WIN32
        #define PLY_IMPL_MUTEX_PATH "impl/Mutex_Win32.h"
        #define PLY_IMPL_MUTEX_TYPE ply::Mutex_Win32
    #elif PLY_TARGET_POSIX
        #define PLY_IMPL_MUTEX_PATH "impl/Mutex_POSIX.h"
        #define PLY_IMPL_MUTEX_TYPE ply::Mutex_POSIX
    #else
        #define PLY_IMPL_MUTEX_PATH "*** Unable to select a default Mutex implementation ***"
    #endif
#endif

// Include the implementation:
#include PLY_IMPL_MUTEX_PATH

// Alias it:
namespace ply {

typedef PLY_IMPL_MUTEX_TYPE Mutex;

//---------------------------------------------------------
// Generic LockGuard
//---------------------------------------------------------
template <typename LockType> class LockGuard {
private:
    LockType& m_lock;

public:
    LockGuard(LockType& lock) : m_lock(lock) {
        m_lock.lock();
    }
    ~LockGuard() {
        m_lock.unlock();
    }
    LockType& getMutex() {
        return m_lock;
    }
};

} // namespace ply
