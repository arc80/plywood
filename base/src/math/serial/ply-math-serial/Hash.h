/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-math.h>
#include <ply-runtime/container/Hash.h>

namespace ply {

// These functions should be made part of the math library, but enabled as an optional
// feature that implies a dependency on ply-runtime. However, that'll require changes to
// the build system first.

inline Hasher& operator<<(Hasher& hasher, const vec2& v) {
    return hasher << v.x << v.y;
}

inline Hasher& operator<<(Hasher& hasher, const vec3& v) {
    return hasher << v.x << v.y << v.z;
}

inline Hasher& operator<<(Hasher& hasher, const vec4& v) {
    return hasher << v.x << v.y << v.z << v.w;
}

inline Hasher& operator<<(Hasher& hasher, const AxisRot& rot) {
    u32 value = u32(rot.cols[0]) | (u32(rot.cols[1]) << 8) | (u32(rot.cols[2]) << 16);
    return hasher << value;
}

inline Hasher& operator<<(Hasher& hasher, const AxisRotPos& rot_pos) {
    return hasher << rot_pos.rot << rot_pos.pos;
}

} // namespace ply
