/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

class Thread_Win32 {
private:
    HANDLE handle = INVALID_HANDLE_VALUE;

    template <typename Callable>
    struct Wrapper {
        Callable callable;

        // Win32 thread entry point
        static PLY_NO_INLINE DWORD WINAPI start(LPVOID param) {
            Wrapper* wrapper = (Wrapper*) param;
            wrapper->callable();
            delete wrapper;
            return 0;
        }
    };

public:
    PLY_INLINE Thread_Win32() = default;

    template <typename Callable>
    PLY_INLINE void run(Callable&& callable) {
        PLY_ASSERT(this->handle == INVALID_HANDLE_VALUE);
        Wrapper<Callable>* wrapper = new Wrapper<Callable>{std::forward<Callable>(callable)};
        this->handle = CreateThread(NULL, 0, Wrapper<Callable>::start, wrapper, 0, NULL);
    }

    template <typename Callable>
    PLY_INLINE Thread_Win32(Callable&& callable) {
        run(std::forward<Callable>(callable));
    }

    PLY_INLINE ~Thread_Win32() {
        if (this->handle != INVALID_HANDLE_VALUE) {
            CloseHandle(this->handle);
        }
    }

    PLY_INLINE bool isValid() const {
        return this->handle != INVALID_HANDLE_VALUE;
    }

    PLY_INLINE void join() {
        PLY_ASSERT(this->handle != INVALID_HANDLE_VALUE);
        WaitForSingleObject(this->handle, INFINITE);
        CloseHandle(this->handle);
        this->handle = INVALID_HANDLE_VALUE;
    }

    static PLY_INLINE void sleepMillis(ureg millis) {
        Sleep((DWORD) millis);
    }
};

} // namespace ply
