/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/string/StringView.h>
#if PLY_TARGET_WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace ply {

class Logger_Stdout {
public:
    static void log(StringView str_with_optional_null_term) {
        StringView str_without_null_term =
            str_with_optional_null_term.without_null_terminator();
#if PLY_TARGET_WIN32
        _write(1, str_without_null_term.bytes, str_without_null_term.num_bytes);
#else
        ssize_t rc = ::write(STDOUT_FILENO, str_without_null_term.bytes,
                             str_without_null_term.num_bytes);
        PLY_UNUSED(rc);
#endif
    }
};

} // namespace ply
