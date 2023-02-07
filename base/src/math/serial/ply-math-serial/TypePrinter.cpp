/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-math/Core.h>
#include <ply-math-serial/TypePrinter.h>

namespace ply {
namespace fmt {

void TypePrinter<Float2>::print(OutStream* outs, const Float2& v) {
    outs->format("{{{}, {}}}", v.x, v.y);
}

void TypePrinter<Float3>::print(OutStream* outs, const Float3& v) {
    outs->format("{{{}, {}, {}}}", v.x, v.y, v.z);
}

void TypePrinter<Float4>::print(OutStream* outs, const Float4& v) {
    outs->format("{{{}, {}, {}, {}}}", v.x, v.y, v.z, v.w);
}

} // namespace fmt
} // namespace ply
