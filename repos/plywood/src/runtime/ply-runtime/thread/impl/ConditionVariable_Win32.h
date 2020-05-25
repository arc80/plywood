/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/thread/impl/Mutex_Win32.h>

namespace ply {

class ConditionVariable_Win32 {
private:
    CONDITION_VARIABLE m_condVar;

public:
    ConditionVariable_Win32() {
        InitializeConditionVariable(&m_condVar);
    }

    void wait(LockGuard<Mutex_Win32>& guard) {
        SleepConditionVariableCS(&m_condVar, &guard.getMutex().m_mutex, INFINITE);
    }

    void timedWait(LockGuard<Mutex_Win32>& guard, ureg waitMillis) {
        if (waitMillis > 0)
            SleepConditionVariableCS(&m_condVar, &guard.getMutex().m_mutex, (DWORD) waitMillis);
    }

    void wakeOne() {
        WakeConditionVariable(&m_condVar);
    }

    void wakeAll() {
        WakeAllConditionVariable(&m_condVar);
    }
};

} // namespace ply
