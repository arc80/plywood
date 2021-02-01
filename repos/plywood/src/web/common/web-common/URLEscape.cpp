/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <web-common/Core.h>
#include <web-common/URLEscape.h>

namespace ply {

PLY_NO_INLINE void fmt::TypePrinter<web::URLEscape>::print(OutStream* outs,
                                                           const web::URLEscape& value) {
    for (u32 i = 0; i < value.view.numBytes; i++) {
        u8 c = value.view[i];
        if (isAsciiLetter(c) || isDecimalDigit(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            *outs << (char) c;
        } else {
            // FIXME: Improve format strings to make this simpler
            *outs << '%' << String::format("0{}", fmt::Hex{c}).upperAsc().right(2);
        }
    }
}

} // namespace ply
