/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

// clang-format off

// Choose default implementation if not already configured by ply_userconfig.h:
#if !defined(PLY_IMPL_THREADLOCAL_PATH)
    #if PLY_TARGET_WIN32
        #define PLY_IMPL_THREADLOCAL_PATH "impl/ThreadLocal_Win32.h"
        #define PLY_IMPL_THREADLOCAL_TYPE ply::ThreadLocal_Win32
        #define PLY_IMPL_THREADLOCALSCOPE_TYPE ply::ThreadLocalScope_Win32
    #elif PLY_TARGET_POSIX
        #define PLY_IMPL_THREADLOCAL_PATH "impl/ThreadLocal_POSIX.h"
        #define PLY_IMPL_THREADLOCAL_TYPE ply::ThreadLocal_POSIX
        #define PLY_IMPL_THREADLOCALSCOPE_TYPE ply::ThreadLocalScope_POSIX
    #else
        #define PLY_IMPL_THREADLOCAL_PATH "*** Unable to select a default ThreadLocal implementation ***"
    #endif
#endif

// Include the implementation:
#include PLY_IMPL_THREADLOCAL_PATH

// Alias it:
namespace ply {

template<typename T>
class ThreadLocal : public PLY_IMPL_THREADLOCAL_TYPE<T> {
public:
    void operator=(T value) {
        PLY_IMPL_THREADLOCAL_TYPE<T>::operator=(value);
    }
};

template<typename T>
class ThreadLocalScope : public PLY_IMPL_THREADLOCALSCOPE_TYPE<T> {
public:
    ThreadLocalScope(ThreadLocal<T>& ptr, T value) : PLY_IMPL_THREADLOCALSCOPE_TYPE<T>(ptr, value) {
    }
};

} // namespace ply
