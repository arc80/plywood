/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/io/InStream.h>
#include <ply-runtime/io/OutStream.h>

namespace ply {

InPipe* get_console_in_pipe();
OutPipe* get_console_out_pipe();
OutPipe* get_console_error_pipe();
    
enum ConsoleMode {
    CM_Text,
    CM_Binary,
};

struct Console_t {
    InStream in(ConsoleMode mode = CM_Text);
    OutStream out(ConsoleMode mode = CM_Text);
    OutStream error(ConsoleMode mode = CM_Text);
};

extern Console_t Console;

} // namespace ply
