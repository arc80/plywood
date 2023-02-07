/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-runtime.h>

namespace ply {

// Returns an InPipe that converts (CR*)LF to just LF.
// Takes ownership of the InStream.
// Works only with 8-bit encodings.
Owned<InPipe> create_in_new_line_filter(InStream&& ins);

// Returns an OutPipe that converts (CR*)LF to either LF or CRLF depending on argument.
// Takes ownership of the OutStream.
// Works only with 8-bit encodings.
Owned<OutPipe> create_out_new_line_filter(OutStream&& out, bool write_crlf);

} // namespace ply
