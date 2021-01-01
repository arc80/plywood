/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/io/text/StringReader.h>
#include <ply-runtime/io/text/StringWriter.h>
#include <ply-runtime/container/Functor.h>

namespace ply {

void extractLiquidTags(StringWriter* outs, StringViewReader* ins,
                       Functor<void(const StringView, const StringView)> tagHandler);

} // namespace ply
