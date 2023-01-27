/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                   ┃
┃    ╱   ╱╲    Plywood Multimedia Toolkit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/          ┃
┃    └──┴┴┴┘                                 ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/StdIO.h>
#include <ply-runtime/io/text/NewLineFilter.h>

namespace ply {

Console_t Console;

#if PLY_TARGET_WIN32

// ┏━━━━━━━━━━━┓
// ┃  Windows  ┃
// ┗━━━━━━━━━━━┛
InPipe* get_console_in_pipe() {
    static InPipe_Handle in_pipe{GetStdHandle(STD_INPUT_HANDLE)};
    return &in_pipe;
}

OutPipe* get_console_out_pipe() {
    static OutPipe_Handle out_pipe{GetStdHandle(STD_OUTPUT_HANDLE)};
    return &out_pipe;
}

OutPipe* get_console_error_pipe() {
    static OutPipe_Handle error_pipe{GetStdHandle(STD_ERROR_HANDLE)};
    return &error_pipe;
}

#elif PLY_TARGET_POSIX

// ┏━━━━━━━━━┓
// ┃  POSIX  ┃
// ┗━━━━━━━━━┛
InPipe* get_console_in_pipe() {
    static InPipe_FD in_pipe{STDIN_FILENO};
    return &in_pipe;
}

InPipe* get_console_out_pipe() {
    static OutPipe_FD out_pipe{STDOUT_FILENO};
    return &out_pipe;
}

InPipe* get_console_error_pipe() {
    static OutPipe_FD error_pipe{STDERR_FILENO};
    return &error_pipe;
}

#endif

InStream Console_t::in(ConsoleMode mode) {
    if (mode == CM_Text) {
        InStream in{get_console_in_pipe(), false};
        // Always create a filter to make newlines consistent.
        return createInNewLineFilter(std::move(in));
    } else {
        return {get_console_in_pipe(), false};
    }
}

OutStream Console_t::out(ConsoleMode mode) {
    OutStream out{get_console_out_pipe(), false};
    bool writeCRLF = false;
#if PLY_TARGET_WIN32
    writeCRLF = true;
#endif
    // Always create a filter to make newlines consistent.
    return createOutNewLineFilter(std::move(out), writeCRLF);
}

OutStream Console_t::error(ConsoleMode mode) {
    OutStream out{get_console_error_pipe(), false};
    bool writeCRLF = false;
#if PLY_TARGET_WIN32
    writeCRLF = true;
#endif
    // Always create a filter to make newlines consistent.
    return createOutNewLineFilter(std::move(out), writeCRLF);
}

} // namespace ply
