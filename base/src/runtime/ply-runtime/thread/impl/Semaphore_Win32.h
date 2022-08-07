/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

class Semaphore_Win32 {
private:
    HANDLE m_sem;

public:
    Semaphore_Win32() {
        m_sem = CreateSemaphore(NULL, 0, INT32_MAX, NULL);
    }

    ~Semaphore_Win32() {
        CloseHandle(m_sem);
    }

    void wait() {
        WaitForSingleObject(m_sem, INFINITE);
    }

    void signal(ureg count = 1) {
        ReleaseSemaphore(m_sem, (DWORD) count, NULL);
    }
};

} // namespace ply
