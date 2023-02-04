/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <web-common/Core.h>
#include <ply-runtime/io/OutStream.h>

namespace ply {
namespace web {
struct URLEscape {
    StringView view;
    u32 maxPoints = 0;
    PLY_INLINE URLEscape(StringView view, u32 maxPoints = 0) : view{view}, maxPoints{maxPoints} {
    }
};
} // namespace web

template <>
struct fmt::TypePrinter<web::URLEscape> {
    static PLY_NO_INLINE void print(OutStream* outs, const web::URLEscape& value);
};
} // namespace ply
