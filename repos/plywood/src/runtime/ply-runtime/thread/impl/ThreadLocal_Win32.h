/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

template <typename T>
class ThreadLocalScope_Win32;

template <typename T>
class ThreadLocal_Win32 {
private:
    PLY_STATIC_ASSERT(sizeof(T) <= PLY_PTR_SIZE);
    DWORD m_tlsIndex;

    ThreadLocal_Win32(const ThreadLocal_Win32&) {
    }

public:
    typedef ThreadLocalScope_Win32<T> Scope;

    ThreadLocal_Win32() {
        m_tlsIndex = TlsAlloc();
        PLY_ASSERT(m_tlsIndex != TLS_OUT_OF_INDEXES);
    }

    ~ThreadLocal_Win32() {
        BOOL rc = TlsFree(m_tlsIndex);
        PLY_ASSERT(rc != 0);
        PLY_UNUSED(rc);
    }

    template <typename U = T, std::enable_if_t<std::is_pointer<U>::value, int> = 0>
    U load() const {
        LPVOID value = TlsGetValue(m_tlsIndex);
        PLY_ASSERT(value != 0 || GetLastError() == ERROR_SUCCESS);
        return (T) value;
    }

    template <typename U = T,
              std::enable_if_t<std::is_enum<U>::value || std::is_integral<U>::value, int> = 0>
    U load() const {
        LPVOID value = TlsGetValue(m_tlsIndex);
        PLY_ASSERT(value != 0 || GetLastError() == ERROR_SUCCESS);
        return (T)(uptr) value;
    }

    void store(T value) {
        BOOL rc = TlsSetValue(m_tlsIndex, (LPVOID) value);
        PLY_ASSERT(rc != 0);
        PLY_UNUSED(rc);
    }

    // In C++11, you can write auto scope = myTLvar.setInScope(value);
    ThreadLocalScope_Win32<T> setInScope(T value);
};

template <typename T>
class ThreadLocalScope_Win32 {
private:
    ThreadLocal_Win32<T>* m_var;
    T m_oldValue;

    ThreadLocalScope_Win32(const ThreadLocalScope_Win32&) {
    }

public:
    ThreadLocalScope_Win32(ThreadLocal_Win32<T>& ptr, T value) : m_var(&ptr) {
        m_oldValue = *m_var;
        *m_var = value;
    }

    ThreadLocalScope_Win32(ThreadLocalScope_Win32&& other) {
        m_var = other.m_var;
        m_oldValue = other.m_oldValue;
        other.m_var = nullptr;
    }

    ~ThreadLocalScope_Win32() {
        if (m_var) {
            *m_var = m_oldValue;
        }
    }
};

template <typename T>
ThreadLocalScope_Win32<T> ThreadLocal_Win32<T>::setInScope(T value) {
    return ThreadLocalScope_Win32<T>(*this, value);
}

} // namespace ply
