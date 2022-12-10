/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/Base.h>

namespace ply {

struct Error_ {
    void log(StringView message);
};

extern Error_ Error;

} // namespace ply
