/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

class RWLock_Win32 {
private:
    SRWLOCK m_rwLock;

public:
    RWLock_Win32() {
        InitializeSRWLock(&m_rwLock);
    }

    ~RWLock_Win32() {
        // SRW locks do not need to be explicitly destroyed.
    }

    void lockExclusive() {
        AcquireSRWLockExclusive(&m_rwLock);
    }

    void unlockExclusive() {
        ReleaseSRWLockExclusive(&m_rwLock);
    }

    void lockShared() {
        AcquireSRWLockShared(&m_rwLock);
    }

    void unlockShared() {
        ReleaseSRWLockShared(&m_rwLock);
    }
};

} // namespace ply
