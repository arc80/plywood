/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

namespace ply {

#define PLY_THREAD_STARTCALL

class Thread_POSIX {
private:
    pthread_t m_handle;
    bool m_attached = false;

    template <typename Callable>
    struct Wrapper {
        Callable callable;

        // POSIX thread entry point
        static PLY_NO_INLINE void* start(void* param) {
            Wrapper* wrapper = (Wrapper*) param;
            wrapper->callable();
            delete wrapper;
            return nullptr;
        }
    };

public:
    PLY_INLINE Thread_POSIX() {
        memset(&m_handle, 0, sizeof(m_handle));
    }

    template <typename Callable>
    PLY_INLINE void run(Callable&& callable) {
        PLY_ASSERT(!m_attached);
        Wrapper<Callable>* wrapper = new Wrapper<Callable>{std::forward<Callable>(callable)};
        pthread_create(&m_handle, NULL, Wrapper<Callable>::start, wrapper);
        m_attached = true;
    }

    template <typename Callable>
    PLY_INLINE Thread_POSIX(Callable&& callable) {
        run(std::forward<Callable>(callable));
    }

    PLY_INLINE ~Thread_POSIX() {
        if (m_attached)
            pthread_detach(m_handle);
    }

    PLY_INLINE bool isValid() const {
        return m_attached;
    }

    PLY_INLINE void join() {
        PLY_ASSERT(m_attached);
        void* retVal;
        pthread_join(m_handle, &retVal);
        memset(&m_handle, 0, sizeof(m_handle));
        m_attached = false;
    }

#if !PLY_TARGET_MINGW
    static PLY_DLL_ENTRY void sleepMillis(ureg millis);
#endif
};

} // namespace ply
