/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

void extractLiquidTags(OutStream& out, ViewInStream& in,
                       Func<void(StringView, StringView)> tagHandler);

} // namespace ply
