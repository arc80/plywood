/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/io/text/StringReader.h>
#include <ply-runtime/io/OutStream.h>
#include <ply-runtime/container/Functor.h>

namespace ply {

void extractLiquidTags(OutStream* outs, StringViewReader* ins,
                       Functor<void(StringView, StringView)> tagHandler);

} // namespace ply
