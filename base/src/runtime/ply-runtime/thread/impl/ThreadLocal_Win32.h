/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

template <typename T>
class ThreadLocal_Win32 {
private:
    PLY_STATIC_ASSERT(sizeof(T) <= PLY_PTR_SIZE);
    DWORD m_tlsIndex;

public:
    PLY_INLINE ThreadLocal_Win32() {
        m_tlsIndex = TlsAlloc();
        PLY_ASSERT(m_tlsIndex != TLS_OUT_OF_INDEXES);
    }

    ThreadLocal_Win32(const ThreadLocal_Win32&) = delete;

    PLY_INLINE ~ThreadLocal_Win32() {
        BOOL rc = TlsFree(m_tlsIndex);
        PLY_ASSERT(rc != 0);
        PLY_UNUSED(rc);
    }

    template <typename U = T, std::enable_if_t<std::is_pointer<U>::value, int> = 0>
    PLY_INLINE U load() const {
        LPVOID value = TlsGetValue(m_tlsIndex);
        PLY_ASSERT(value != 0 || GetLastError() == ERROR_SUCCESS);
        return (T) value;
    }

    template <typename U = T,
              std::enable_if_t<std::is_enum<U>::value || std::is_integral<U>::value, int> = 0>
    PLY_INLINE U load() const {
        LPVOID value = TlsGetValue(m_tlsIndex);
        PLY_ASSERT(value != 0 || GetLastError() == ERROR_SUCCESS);
        return (T)(uptr) value;
    }

    PLY_INLINE void store(T value) {
        BOOL rc = TlsSetValue(m_tlsIndex, (LPVOID) value);
        PLY_ASSERT(rc != 0);
        PLY_UNUSED(rc);
    }

    // In C++11, you can write auto scope = myTLvar.setInScope(value);
    using Scope = ThreadLocalScope<ThreadLocal_Win32, T>;
    PLY_INLINE Scope setInScope(T value) {
        return {this, value};
    }
};

} // namespace ply
