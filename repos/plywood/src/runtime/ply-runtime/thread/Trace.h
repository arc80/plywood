/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

// clang-format off

// Choose default implementation if not already configured by ply_userconfig.h:
#if !defined(PLY_IMPL_TRACE_PATH)
    #define PLY_IMPL_TRACE_PATH "impl/Trace_Null.h"
    #define PLY_IMPL_TRACE_TYPE ply::Trace_Null
#endif

// Include the implementation:
#include PLY_IMPL_TRACE_PATH

// Alias it:
namespace ply {
typedef PLY_IMPL_TRACE_TYPE Trace;
}
