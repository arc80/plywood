/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

class Thread_Win32 {
private:
    HANDLE m_handle = INVALID_HANDLE_VALUE;

    template <typename Invocable>
    struct EntryPoint {
        Invocable inv;

        // Win32 thread entry point
        static PLY_NO_INLINE DWORD WINAPI start(LPVOID param) {
            EntryPoint* te = (EntryPoint*) param;
            te->inv();
            delete te;
            return 0;
        }
    };

public:
    PLY_INLINE Thread_Win32() = default;

    template <typename Invocable>
    PLY_INLINE void run(Invocable&& inv) {
        PLY_ASSERT(m_handle == INVALID_HANDLE_VALUE);
        EntryPoint<Invocable>* te = new EntryPoint<Invocable>{std::forward<Invocable>(inv)};
        m_handle = CreateThread(NULL, 0, EntryPoint<Invocable>::start, te, 0, NULL);
    }

    template <typename Invocable>
    PLY_INLINE Thread_Win32(Invocable&& inv) {
        run(std::forward<Invocable>(inv));
    }

    PLY_INLINE ~Thread_Win32() {
        if (m_handle != INVALID_HANDLE_VALUE) {
            CloseHandle(m_handle);
        }
    }

    PLY_INLINE bool isValid() const {
        return m_handle != INVALID_HANDLE_VALUE;
    }

    PLY_INLINE void join() {
        PLY_ASSERT(m_handle != INVALID_HANDLE_VALUE);
        WaitForSingleObject(m_handle, INFINITE);
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }

    static PLY_INLINE void sleepMillis(ureg millis) {
        Sleep((DWORD) millis);
    }
};

} // namespace ply
