/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <pthread.h>

namespace ply {

template <typename T>
class ThreadLocal_POSIX {
private:
    PLY_STATIC_ASSERT(sizeof(T) <= PLY_PTR_SIZE);
    pthread_key_t m_tlsKey;

public:
    PLY_INLINE ThreadLocal_POSIX() {
        int rc = pthread_key_create(&m_tlsKey, NULL);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
    }

    ThreadLocal_POSIX(const ThreadLocal_POSIX&) = delete;

    PLY_INLINE ~ThreadLocal_POSIX() {
        int rc = pthread_key_delete(m_tlsKey);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
    }

    template <typename U = T, std::enable_if_t<std::is_pointer<U>::value, int> = 0>
    PLY_INLINE U load() const {
        void* value = pthread_getspecific(m_tlsKey);
        return (T) value;
    }

    template <typename U = T,
              std::enable_if_t<std::is_enum<U>::value || std::is_integral<U>::value, int> = 0>
    PLY_INLINE U load() const {
        void* value = pthread_getspecific(m_tlsKey);
        return (T)(uptr) value;
    }

    template <typename U = T,
              std::enable_if_t<std::is_enum<U>::value || std::is_integral<U>::value, int> = 0>
    PLY_INLINE void store(U value) {
        int rc = pthread_setspecific(m_tlsKey, (void*) (uptr) value);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
    }

    // In C++11, you can write auto scope = myTLvar.setInScope(value);
    using Scope = ThreadLocalScope<ThreadLocal_POSIX, T>;
    PLY_INLINE Scope setInScope(T value) {
        return {this, value};
    }
};

} // namespace ply
