/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-math/Core.h>
#include <ply-math/Matrix.h>
#include <ply-math/Complex.h>
#include <ply-math/AxisVector.h>

namespace ply {
namespace extra {

// FIXME: This is coordinate system-specific, maybe should re-organize:
inline Float4x4 lookAt(const Float3& direction) {
    // FIXME: Check for epsilons
    Float4x4 cameraToWorld = Float4x4::identity();
    Float3 fwd = direction.normalized();
    Float3 up{0, 0, 1};
    Float3 right = cross(fwd, up).normalized();
    up = cross(right, fwd);
    cameraToWorld[0] = {right, 0};
    cameraToWorld[1] = {up, 0};
    cameraToWorld[2] = {-fwd, 0};
    return cameraToWorld.transposed(); // returns worldToCamera
}

inline Float3x3 makeBasis(const Float3& unitFwdTo, const Float3& upTo, Axis3 fwdFrom,
                          Axis3 upFrom) {
    Float3 rightXPos = cross(unitFwdTo, upTo);
    float L2 = rightXPos.length2();
    if (L2 < 1e-6f) {
        Float3 notCollinear =
            (unitFwdTo.z * unitFwdTo.z < 0.9f) ? Float3{0, 0, 1} : Float3{0, -1, 0};
        rightXPos = cross(unitFwdTo, notCollinear);
        L2 = rightXPos.length2();
    }
    rightXPos /= sqrtf(L2);
    Float3 fixedUpZPos = cross(rightXPos, unitFwdTo);
    PLY_ASSERT(fabsf(fixedUpZPos.length() - 1) < 0.001f); // Should have unit length
    return Float3x3{rightXPos, unitFwdTo, fixedUpZPos} *
           AxisRot{cross(fwdFrom, upFrom), fwdFrom, upFrom}.inverted().toFloat3x3();
}

// FIXME: Move approach() out of extra namespace?
template <class V>
V approach(const V& from, const V& to, typename V::T step) {
    V delta = to - from;
    typename V::T length = delta.length();
    return (length < step) ? to : from + delta * (step / length);
}

template <class V>
V clampLength(const V& vec, float maxLen) {
    typename V::T length = vec.length();
    return (length <= maxLen) ? vec : vec * (maxLen / length);
}

//---------------------------------------------------------------------------
inline u32 packNormal(const Float3& normal) {
    PLY_ASSERT(fabsf(1 - normal.length()) < 0.001f);
    u32 x = clamp<u32>(u32(roundf(normal.x * 511 + 512)), 0, 1023);
    u32 y = clamp<u32>(u32(roundf(normal.y * 511 + 512)), 0, 1023);
    u32 z = clamp<u32>(u32(roundf(normal.z * 511 + 512)), 0, 1023);
    return (x << 20) | (y << 10) | z;
}

inline u8 packDirection(const Float2& dir) {
    float angleRad = atan2f(dir.y, dir.x);
    float angleFrac = wrapOne(angleRad / (2 * Pi));
    return (u8) roundf(angleFrac * 256);
}

/*! Result is NOT normalized!
 */
inline Float3 unpackFloat3_101010(u32 packed) {
    static const float ood = 1.f / 511;
    return {(s32((packed >> 20) & 0x3ff) - 512) * ood, (s32((packed >> 10) & 0x3ff) - 512) * ood,
            (s32(packed & 0x3ff) - 512) * ood};
}

inline Float2 unpackDirection_8(u8 packed) {
    float radians = packed * (2 * Pi / 256.f);
    return Complex::fromAngle(radians);
}

} // namespace extra
} // namespace ply
