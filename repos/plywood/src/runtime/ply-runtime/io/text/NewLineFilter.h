/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/io/Pipe.h>
#include <ply-runtime/io/InStream.h>
#include <ply-runtime/io/OutStream.h>

namespace ply {

// Returns an InPipe that converts (CR*)LF to just LF.
// Takes ownership of the InStream.
// Works only with 8-bit encodings.
PLY_DLL_ENTRY Owned<InPipe> createInNewLineFilter(OptionallyOwned<InStream>&& ins);

// Returns an OutPipe that converts (CR*)LF to either LF or CRLF depending on argument.
// Takes ownership of the OutStream.
// Works only with 8-bit encodings.
PLY_DLL_ENTRY Owned<OutPipe> createOutNewLineFilter(OptionallyOwned<OutStream>&& outs,
                                                    bool writeCRLF);

} // namespace ply
