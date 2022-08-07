/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

// clang-format off

// Choose default implementation if not already configured by ply_userconfig.h:
#if !defined(PLY_IMPL_MANUALRESETEVENT_PATH)
    #if PLY_TARGET_WIN32
        #define PLY_IMPL_MANUALRESETEVENT_PATH "impl/ManualResetEvent_Win32.h"
        #define PLY_IMPL_MANUALRESETEVENT_TYPE ply::ManualResetEvent_Win32
    #else
        #define PLY_IMPL_MANUALRESETEVENT_PATH "impl/ManualResetEvent_CondVar.h"
        #define PLY_IMPL_MANUALRESETEVENT_TYPE ply::ManualResetEvent_CondVar
    #endif
#endif

// Include the implementation:
#include PLY_IMPL_MANUALRESETEVENT_PATH

// Alias it:
namespace ply {
typedef PLY_IMPL_MANUALRESETEVENT_TYPE ManualResetEvent;
}
