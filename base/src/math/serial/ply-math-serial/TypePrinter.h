/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-math.h>
#include <ply-runtime/io/OutStream.h>

namespace ply {
namespace fmt {

template <>
struct TypePrinter<vec2> {
    static void print(OutStream* outs, const vec2& v);
};

template <>
struct TypePrinter<vec3> {
    static void print(OutStream* outs, const vec3& v);
};

template <>
struct TypePrinter<vec4> {
    static void print(OutStream* outs, const vec4& v);
};

} // namespace fmt
} // namespace ply
