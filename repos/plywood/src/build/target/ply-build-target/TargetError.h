/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>

namespace ply {
namespace build {

struct TargetError {
    static thread_local Functor<void(StringView)> callback_;

    static PLY_INLINE void log(StringView msg) {
        if (callback_) {
            callback_.call(msg);
        }
    }
};

} // namespace build
} // namespace ply
