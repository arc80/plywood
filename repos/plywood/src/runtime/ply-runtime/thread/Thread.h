/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

// clang-format off

// Choose default implementation if not already configured by ply_userconfig.h:
#if !defined(PLY_IMPL_THREAD_PATH)
    #if PLY_PREFER_CPP11
        #define PLY_IMPL_THREAD_PATH "impl/Thread_CPP11.h"
        #define PLY_IMPL_THREAD_TYPE ply::Thread_CPP11
    #elif PLY_TARGET_WIN32
        #define PLY_IMPL_THREAD_PATH "impl/Thread_Win32.h"
        #define PLY_IMPL_THREAD_TYPE ply::Thread_Win32
    #elif PLY_TARGET_POSIX
        #define PLY_IMPL_THREAD_PATH "impl/Thread_POSIX.h"
        #define PLY_IMPL_THREAD_TYPE ply::Thread_POSIX
    #else
        #define PLY_IMPL_THREAD_PATH "*** Unable to select a default Thread implementation ***"
    #endif
#endif

// Include the implementation:
#include PLY_IMPL_THREAD_PATH

// Alias it:
namespace ply {
typedef PLY_IMPL_THREAD_TYPE Thread;
}
