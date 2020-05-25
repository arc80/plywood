/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>

namespace ply {
namespace build {

struct ErrorHandler {
    enum Level {
        Info,
        Error,
        Fatal,
    };

    static thread_local Functor<void(Level, HybridString&&)> current;

    static void log(Level errorLevel, HybridString&& error); // Does not return if Fatal
};

} // namespace build
} // namespace ply
