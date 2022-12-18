/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Error.h>

namespace ply {

Error_t Error;

void Error_t::log_internal(StringView fmt, ArrayView<const FormatArg> args) {
    OutStream outs = StdErr::text();
    outs.formatInternal(fmt, args);
    if (!fmt.endsWith('\n')) {
        outs << '\n';
    }
}

} // namespace ply
