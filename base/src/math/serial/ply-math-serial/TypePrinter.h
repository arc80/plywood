/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-math/Core.h>
#include <ply-math/Vector.h>
#include <ply-runtime/io/OutStream.h>

namespace ply {
namespace fmt {

template <>
struct TypePrinter<Float2> {
    static void print(OutStream* outs, const Float2& v);
};

template <>
struct TypePrinter<Float3> {
    static void print(OutStream* outs, const Float3& v);
};

template <>
struct TypePrinter<Float4> {
    static void print(OutStream* outs, const Float4& v);
};

} // namespace fmt
} // namespace ply
