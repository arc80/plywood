/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

// clang-format off

// Choose default implementation if not already configured by ply_userconfig.h:
#if !defined(PLY_IMPL_MEMPAGE_PATH)
    #if PLY_TARGET_WIN32
        #define PLY_IMPL_MEMPAGE_PATH "impl/MemPage_Win32.h"
        #define PLY_IMPL_MEMPAGE_TYPE ply::MemPage_Win32
    #elif PLY_TARGET_POSIX
        #define PLY_IMPL_MEMPAGE_PATH "impl/MemPage_POSIX.h"
        #define PLY_IMPL_MEMPAGE_TYPE ply::MemPage_POSIX
    #else
        #define PLY_IMPL_MEMPAGE_PATH "*** Unable to select a default MemPage implementation ***"
    #endif
#endif

// Include the implementation:
#include PLY_IMPL_MEMPAGE_PATH

// Alias it:
namespace ply {
typedef PLY_IMPL_MEMPAGE_TYPE MemPage;
}
