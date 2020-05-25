/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <pthread.h>

namespace ply {

template <typename T>
class ThreadLocalScope_POSIX;

template <typename T>
class ThreadLocal_POSIX {
private:
    PLY_STATIC_ASSERT(sizeof(T) <= PLY_PTR_SIZE);
    pthread_key_t m_tlsKey;

    ThreadLocal_POSIX(const ThreadLocal_POSIX&) {
    }

public:
    typedef ThreadLocalScope_POSIX<T> Scope;

    ThreadLocal_POSIX() {
        int rc = pthread_key_create(&m_tlsKey, NULL);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
    }

    ~ThreadLocal_POSIX() {
        int rc = pthread_key_delete(m_tlsKey);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
    }

    template <typename U = T, std::enable_if_t<std::is_pointer<U>::value, int> = 0>
    U load() const {
        void* value = pthread_getspecific(m_tlsKey);
        return (T) value;
    }

    template <typename U = T,
              std::enable_if_t<std::is_enum<U>::value || std::is_integral<U>::value, int> = 0>
    U load() const {
        void* value = pthread_getspecific(m_tlsKey);
        return (T)(uptr) value;
    }

    template <typename U = T,
              std::enable_if_t<std::is_enum<U>::value || std::is_integral<U>::value, int> = 0>
    void store(U value) {
        int rc = pthread_setspecific(m_tlsKey, (void*) (uptr) value);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
    }

    // In C++11, you can write auto scope = myTLvar.setInScope(value);
    ThreadLocalScope_POSIX<T> setInScope(T value);
};

template <typename T>
class ThreadLocalScope_POSIX {
private:
    ThreadLocal_POSIX<T>* m_var;
    T m_oldValue;

    ThreadLocalScope_POSIX(const ThreadLocalScope_POSIX&) {
    }

public:
    ThreadLocalScope_POSIX(ThreadLocal_POSIX<T>& ptr, T value) : m_var(&ptr) {
        m_oldValue = *m_var;
        *m_var = value;
    }

    ThreadLocalScope_POSIX(ThreadLocalScope_POSIX&& other) {
        m_var = other.m_var;
        m_oldValue = other.m_oldValue;
        other.m_var = nullptr;
    }

    ~ThreadLocalScope_POSIX() {
        if (m_var) {
            *m_var = m_oldValue;
        }
    }
};

template <typename T>
ThreadLocalScope_POSIX<T> ThreadLocal_POSIX<T>::setInScope(T value) {
    return ThreadLocalScope_POSIX<T>(*this, value);
}

} // namespace ply
