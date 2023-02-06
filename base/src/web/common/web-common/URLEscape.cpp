/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <web-common/Core.h>
#include <web-common/URLEscape.h>

namespace ply {

PLY_NO_INLINE void
fmt::TypePrinter<web::URLEscape>::print(OutStream* outs, const web::URLEscape& value) {
    for (u32 i = 0; i < value.view.num_bytes; i++) {
        u8 c = value.view[i];
        if (is_ascii_letter(c) || is_decimal_digit(c) || c == '-' || c == '_' ||
            c == '.' || c == '~') {
            *outs << (char) c;
        } else {
            // FIXME: Improve format strings to make this simpler
            *outs << '%' << String::format("0{}", fmt::Hex{c}).upper_asc().right(2);
        }
    }
}

} // namespace ply
