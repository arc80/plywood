/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

class Mutex_Win32 {
private:
    friend class ConditionVariable_Win32;
    CRITICAL_SECTION m_mutex;

public:
    Mutex_Win32() {
        InitializeCriticalSection(&m_mutex);
    }

    ~Mutex_Win32() {
        DeleteCriticalSection(&m_mutex);
    }

    void lock() {
        EnterCriticalSection(&m_mutex);
    }

    bool tryLock() {
        return !!TryEnterCriticalSection(&m_mutex);
    }

    void unlock() {
        LeaveCriticalSection(&m_mutex);
    }
};

} // namespace ply
