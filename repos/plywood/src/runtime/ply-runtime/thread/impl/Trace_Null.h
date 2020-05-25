/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

struct Trace_Null {
    void dumpStats() {
    }

    PLY_DLL_ENTRY static Trace_Null Instance;
};

} // namespace ply

// clang-format off
#define PLY_TRACE_DECLARE(group, count)
#define PLY_TRACE_DEFINE_BEGIN(group, count)
#define PLY_TRACE_DEFINE(desc)
#define PLY_TRACE_DEFINE_END(group, count)
#define PLY_TRACE(group, id, str, param1, param2) do {} while (0)
// clang-format on
