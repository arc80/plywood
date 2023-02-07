/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-math/Core.h>
#include <ply-math/QuatPos.h>

namespace ply {

QuatPos QuatPos::inverted() const {
    Quaternion qi = quat.inverted();
    return {qi, qi * -pos};
}

QuatPos QuatPos::identity() {
    return {{0, 0, 0, 1}, {0, 0, 0}};
}

QuatPos QuatPos::make_translation(const Float3& pos) {
    return {{0, 0, 0, 1}, pos};
}

QuatPos QuatPos::make_rotation(const Float3& unit_axis, float radians) {
    return {Quaternion::from_axis_angle(unit_axis, radians), {0, 0, 0}};
}

QuatPos QuatPos::from_ortho(const Float3x4& m) {
    return {Quaternion::from_ortho(m.as_float3x3()), m[3]};
}

QuatPos QuatPos::from_ortho(const Float4x4& m) {
    return {Quaternion::from_ortho(m), m[3].as_float3()};
}

} // namespace ply
