/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/io/InStream.h>
#include <ply-runtime/io/OutStream.h>
#include <ply-runtime/container/Func.h>

namespace ply {

void extractLiquidTags(OutStream& out, ViewInStream* ins,
                       Func<void(StringView, StringView)> tagHandler);

} // namespace ply
