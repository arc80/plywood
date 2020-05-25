/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

class ManualResetEvent_Win32 {
private:
    HANDLE m_evt;

public:
    ManualResetEvent_Win32(bool initialState = false) {
        m_evt = CreateEvent(NULL, TRUE, initialState ? TRUE : FALSE, NULL);
    }

    ~ManualResetEvent_Win32() {
        CloseHandle(m_evt);
    }

    void signal() {
        SetEvent(m_evt);
    }

    void reset() {
        ResetEvent(m_evt);
    }

    void wait() {
        WaitForSingleObject(m_evt, INFINITE);
    }
};

} // namespace ply
