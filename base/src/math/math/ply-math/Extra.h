/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-math/Core.h>
#include <ply-math/Matrix.h>
#include <ply-math/Complex.h>
#include <ply-math/AxisVector.h>

namespace ply {

// FIXME: This is coordinate system-specific, maybe should re-organize:
inline Float4x4 look_at(const Float3& direction) {
    // FIXME: Check for epsilons
    Float4x4 camera_to_world = Float4x4::identity();
    Float3 fwd = direction.normalized();
    Float3 up{0, 0, 1};
    Float3 right = cross(fwd, up).normalized();
    up = cross(right, fwd);
    camera_to_world[0] = {right, 0};
    camera_to_world[1] = {up, 0};
    camera_to_world[2] = {-fwd, 0};
    return camera_to_world.transposed(); // returns world_to_camera
}

inline Float3 not_collinear(const Float3& unit_vec) {
    return square(unit_vec.z) < 0.9f ? Float3{0, 0, 1} : Float3{0, -1, 0};
}

inline Float3 any_perp(const Float3& unit_vec) {
    return cross(unit_vec, not_collinear(unit_vec));
}

inline Float3x3 make_basis(const Float3& unit_fwd_to, const Float3& up_to,
                           Axis3 fwd_from, Axis3 up_from) {
    Float3 right_xpos = cross(unit_fwd_to, up_to);
    float L2 = right_xpos.length2();
    if (L2 < 1e-6f) {
        Float3 not_collinear =
            (unit_fwd_to.z * unit_fwd_to.z < 0.9f) ? Float3{0, 0, 1} : Float3{0, -1, 0};
        right_xpos = cross(unit_fwd_to, not_collinear);
        L2 = right_xpos.length2();
    }
    right_xpos /= sqrtf(L2);
    Float3 fixed_up_zpos = cross(right_xpos, unit_fwd_to);
    PLY_ASSERT(fabsf(fixed_up_zpos.length() - 1) < 0.001f); // Should have unit length
    return Float3x3{right_xpos, unit_fwd_to, fixed_up_zpos} *
           AxisRot{cross(fwd_from, up_from), fwd_from, up_from}
               .inverted()
               .to_float3x3();
}

inline Float3x3 make_basis(const Float3& unit_fwd, Axis3 fwd_from) {
    u32 up_from = u32(fwd_from) + 2;
    up_from -= (up_from >= 6) * 6;
    return make_basis(unit_fwd, not_collinear(unit_fwd), fwd_from, Axis3(up_from));
}

// FIXME: Move approach() out of extra namespace?
template <typename T, typename = std::enable_if_t<!std::is_arithmetic<T>::value>>
T approach(const T& from, const T& to, decltype(T::x) step) {
    T delta = to - from;
    decltype(T::x) length = delta.length();
    return (length < step) ? to : from + delta * (step / length);
}

template <class V>
V clamp_length(const V& vec, float max_len) {
    typename V::T length = vec.length();
    return (length <= max_len) ? vec : vec * (max_len / length);
}

} // namespace ply
