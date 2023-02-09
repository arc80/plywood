/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-math.h>
#include <ply-math-serial/TypePrinter.h>

namespace ply {
namespace fmt {

void TypePrinter<vec2>::print(OutStream* outs, const vec2& v) {
    outs->format("{{{}, {}}}", v.x, v.y);
}

void TypePrinter<vec3>::print(OutStream* outs, const vec3& v) {
    outs->format("{{{}, {}, {}}}", v.x, v.y, v.z);
}

void TypePrinter<vec4>::print(OutStream* outs, const vec4& v) {
    outs->format("{{{}, {}, {}, {}}}", v.x, v.y, v.z, v.w);
}

} // namespace fmt
} // namespace ply
