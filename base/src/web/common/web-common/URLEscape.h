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
    u32 max_points = 0;
    URLEscape(StringView view, u32 max_points = 0)
        : view{view}, max_points{max_points} {
    }
};
} // namespace web

template <>
struct fmt::TypePrinter<web::URLEscape> {
    static void print(OutStream* outs, const web::URLEscape& value);
};
} // namespace ply
