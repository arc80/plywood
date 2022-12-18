/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/Base.h>

namespace ply {

struct Error_t {
    void log_internal(StringView fmt, ArrayView<const FormatArg> args);
    template <typename... Args>
    PLY_INLINE void log(StringView fmt, const Args&... args) {
        auto argList = FormatArg::collect(args...);
        this->log_internal(fmt, argList);
    }
};

extern Error_t Error;

} // namespace ply
