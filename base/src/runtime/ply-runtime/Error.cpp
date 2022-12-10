/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Error.h>

namespace ply {

Error_ Error;

void Error_::log(StringView message) {
    OutStream outs = StdErr::text();
    outs << message;
    if (!message.endsWith('\n')) {
        outs << '\n';
    }
}

} // namespace ply
