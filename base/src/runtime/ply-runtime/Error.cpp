/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime.h>

namespace ply {

Error_t Error;

void Error_t::log_internal(StringView fmt, ArrayView<const FormatArg> args) {
    OutStream out = Console.error();
    out.format_args(fmt, args);
    if (!fmt.ends_with('\n')) {
        out << '\n';
    }
}

} // namespace ply
