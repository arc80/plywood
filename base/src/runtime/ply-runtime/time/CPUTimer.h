/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

// clang-format off

// Choose default implementation if not already configured by ply_userconfig.h:
#if !defined(PLY_IMPL_CPUTIMER_PATH)
    #if PLY_PREFER_CPP11
        #define PLY_IMPL_CPUTIMER_PATH "impl/CPUTimer_CPP11.h"
        #define PLY_IMPL_CPUTIMER_TYPE ply::CPUTimer_CPP11
    #elif PLY_TARGET_WIN32 || PLY_TARGET_MINGW
        #define PLY_IMPL_CPUTIMER_PATH "impl/CPUTimer_Win32.h"
        #define PLY_IMPL_CPUTIMER_TYPE ply::CPUTimer_Win32
    #elif PLY_KERNEL_MACH
        #define PLY_IMPL_CPUTIMER_PATH "impl/CPUTimer_Mach.h"
        #define PLY_IMPL_CPUTIMER_TYPE ply::CPUTimer_Mach
    #elif PLY_TARGET_POSIX
        #define PLY_IMPL_CPUTIMER_PATH "impl/CPUTimer_POSIX.h"
        #define PLY_IMPL_CPUTIMER_TYPE ply::CPUTimer_POSIX
    #else
        #define PLY_IMPL_CPUTIMER_PATH "*** Unable to select a default CPUTimer implementation ***"
    #endif
#endif

// Include the implementation:
#include PLY_IMPL_CPUTIMER_PATH

// Alias it:
namespace ply {
typedef PLY_IMPL_CPUTIMER_TYPE CPUTimer;
}
