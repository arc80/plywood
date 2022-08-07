/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

// clang-format off

// Choose default implementation if not already configured by ply_userconfig.h:
#if !defined(PLY_IMPL_AFFINITY_PATH)
    #if PLY_TARGET_WIN32
        #define PLY_IMPL_AFFINITY_PATH "impl/Affinity_Win32.h"
        #define PLY_IMPL_AFFINITY_TYPE ply::Affinity_Win32
    #elif PLY_KERNEL_LINUX
        #define PLY_IMPL_AFFINITY_PATH "impl/Affinity_Linux.h"
        #define PLY_IMPL_AFFINITY_TYPE ply::Affinity_Linux
    #elif PLY_KERNEL_FREEBSD
        #define PLY_IMPL_AFFINITY_PATH "impl/Affinity_FreeBSD.h"
        #define PLY_IMPL_AFFINITY_TYPE ply::Affinity_FreeBSD
    #elif PLY_KERNEL_MACH
        #define PLY_IMPL_AFFINITY_PATH "impl/Affinity_Mach.h"
        #define PLY_IMPL_AFFINITY_TYPE ply::Affinity_Mach
    #elif PLY_TARGET_MINGW
        // FIXME: Is there an API to detect CPU topology on MinGW?
        #define PLY_IMPL_AFFINITY_PATH "impl/Affinity_Null.h"
        #define PLY_IMPL_AFFINITY_TYPE ply::Affinity_Null
    #else
        #define PLY_IMPL_AFFINITY_PATH "*** Unable to select a default Affinity implementation ***"
    #endif
#endif

// Include the implementation:
#include PLY_IMPL_AFFINITY_PATH

// Alias it:
namespace ply {
typedef PLY_IMPL_AFFINITY_TYPE Affinity;
}
