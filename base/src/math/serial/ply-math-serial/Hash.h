/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-math/Core.h>
#include <ply-runtime/container/Hash.h>

namespace ply {

// These functions should be made part of the math library, but enabled as an optional feature that
// implies a dependency on ply-runtime. However, that'll require changes to the build system first.

PLY_INLINE Hasher& operator<<(Hasher& hasher, const Float2& v) {
    return hasher << v.x << v.y;
}

PLY_INLINE Hasher& operator<<(Hasher& hasher, const Float3& v) {
    return hasher << v.x << v.y << v.z;
}

PLY_INLINE Hasher& operator<<(Hasher& hasher, const Float4& v) {
    return hasher << v.x << v.y << v.z << v.w;
}

PLY_INLINE Hasher& operator<<(Hasher& hasher, const AxisRot& rot) {
    u32 value = u32(rot.cols[0]) | (u32(rot.cols[1]) << 8) | (u32(rot.cols[2]) << 16);
    return hasher << value;
}

PLY_INLINE Hasher& operator<<(Hasher& hasher, const AxisRotPos& rotPos) {
    return hasher << rotPos.rot << rotPos.pos;
}

} // namespace ply
