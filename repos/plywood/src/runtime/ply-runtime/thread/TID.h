/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

// clang-format off

// Choose default implementation if not already configured by ply_userconfig.h:
#if !defined(PLY_IMPL_TID_PATH)
    #if PLY_TARGET_WIN32
        #define PLY_IMPL_TID_PATH "impl/TID_Win32.h"
        #define PLY_IMPL_TID_TYPE ply::TID_Win32
    #elif PLY_KERNEL_MACH
        #define PLY_IMPL_TID_PATH "impl/TID_Mach.h"
        #define PLY_IMPL_TID_TYPE ply::TID_Mach
    #elif PLY_TARGET_POSIX
        #define PLY_IMPL_TID_PATH "impl/TID_POSIX.h"
        #define PLY_IMPL_TID_TYPE ply::TID_POSIX
    #else
        #define PLY_IMPL_TID_PATH "*** Unable to select a default TID implementation ***"
    #endif
#endif

// Include the implementation:
#include PLY_IMPL_TID_PATH

// Alias it:
namespace ply {
typedef PLY_IMPL_TID_TYPE TID;
}
