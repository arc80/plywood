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

    template <typename Invocable>
    struct EntryPoint {
        Invocable inv;

        // POSIX thread entry point
        static PLY_NO_INLINE void* start(void* param) {
            EntryPoint* te = (EntryPoint*) param;
            te->inv();
            delete te;
            return nullptr;
        }
    };

public:
    typedef void* ReturnType;
    typedef void* (*StartRoutine)(void*);

    PLY_INLINE Thread_POSIX() {
        memset(&m_handle, 0, sizeof(m_handle));
    }

    template <typename Invocable>
    PLY_INLINE void run(Invocable&& inv) {
        PLY_ASSERT(!m_attached);
        EntryPoint<Invocable>* te = new EntryPoint<Invocable>{std::forward<Invocable>(inv)};
        pthread_create(&m_handle, NULL, EntryPoint<Invocable>::start, te);
        m_attached = true;
    }

    template <typename Invocable>
    PLY_INLINE Thread_POSIX(Invocable&& inv) {
        run(std::forward<Invocable>(inv));
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
