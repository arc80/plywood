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

inline Float3 notCollinear(const Float3& unitVec) {
    return square(unitVec.z) < 0.9f ? Float3{0, 0, 1} : Float3{0, -1, 0};
}

inline Float3 anyPerp(const Float3& unitVec) {
    return cross(unitVec, notCollinear(unitVec));
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

inline Float3x3 makeBasis(const Float3& unitFwd, Axis3 fwdFrom) {
    u32 upFrom = u32(fwdFrom) + 2;
    upFrom -= (upFrom >= 6) * 6;
    return makeBasis(unitFwd, notCollinear(unitFwd), fwdFrom, Axis3(upFrom));
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

} // namespace ply
