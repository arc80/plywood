/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/io/OutStream.h>
#include <ply-runtime/io/StdIO.h>

namespace ply {

Error_t Error;

void Error_t::log_internal(StringView fmt, ArrayView<const FormatArg> args) {
    OutStream out = Console.error();
    out.format_args(fmt, args);
    if (!fmt.endsWith('\n')) {
        out << '\n';
    }
}

} // namespace ply
