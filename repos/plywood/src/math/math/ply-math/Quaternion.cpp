/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-math/Core.h>
#include <ply-math/Quaternion.h>
#include <ply-math/Matrix.h>

namespace ply {

Quaternion Quaternion::fromUnitVectors(const Float3& start, const Float3& end) {
    // Float4{cross(start, end), dot(start, end)} gives you double the desired rotation.
    // To get the desired rotation, "average" (really just sum) that with Float4{0, 0, 0, 1},
    // then normalize.
    float w = dot(start, end) + 1;
    if (w < 1e-6f) {
        // Exceptional case: Vectors point in opposite directions.
        // Choose a perpendicular axis and make a 180 degree rotation.
        Float3 notCollinear = (fabsf(start.x) < 0.9f) ? Float3{1, 0, 0} : Float3{0, 1, 0};
        Float3 axis = cross(start, notCollinear);
        return Float4{axis, 0}.normalized().asQuaternion();
    }
    Float3 v = cross(start, end);
    return Float4{v, w}.normalized().asQuaternion();
}

template <class M>
Quaternion quaternionFromOrtho(const M& m) {
    float t; // This will be set to 4*c*c for some quaternion component c.
    // At least one component's square must be >= 1/4. (Otherwise, it isn't a unit quaternion.)
    // Let's require t >= 1/2. This will accept any component whose square is >= 1/8.
    if ((t = 1.f + m[0][0] + m[1][1] + m[2][2]) >= 0.5f) { // 4*w*w
        float w = sqrtf(t) * 0.5f;
        float f = 0.25f / w;
        return {(m[1][2] - m[2][1]) * f, (m[2][0] - m[0][2]) * f, (m[0][1] - m[1][0]) * f, w};
    } else if ((t = 1.f + m[0][0] - m[1][1] - m[2][2]) >= 0.5f) { // 4*x*x
        // Prefer positive w component in result
        float wco = m[1][2] - m[2][1];
        float x = sqrtf(t) * ((wco >= 0) - 0.5f); // equivalent to sqrtf(t) * 0.5f * sgn(wco)
        float f = 0.25f / x;
        return {x, (m[0][1] + m[1][0]) * f, (m[2][0] + m[0][2]) * f, wco * f};
    } else if ((t = 1.f - m[0][0] + m[1][1] - m[2][2]) >= 0.5f) { // 4*y*y
        float wco = m[2][0] - m[0][2];
        float y = sqrtf(t) * ((wco >= 0) - 0.5f); // equivalent to sqrtf(t) * 0.5f * sgn(wco)
        float f = 0.25f / y;
        return {(m[0][1] + m[1][0]) * f, y, (m[1][2] + m[2][1]) * f, wco * f};
    } else if ((t = 1.f - m[0][0] - m[1][1] + m[2][2]) >= 0.5f) { // 4*z*z
        float wco = m[0][1] - m[1][0];
        float z = sqrtf(t) * ((wco >= 0) - 0.5f); // equivalent to sqrtf(t) * 0.5f * sgn(wco)
        float f = 0.25f / z;
        return {(m[2][0] + m[0][2]) * f, (m[1][2] + m[2][1]) * f, z, wco * f};
    }
    PLY_ASSERT(0); // The matrix is not even close to being orthonormal
    return {0, 0, 0, 1};
}

Quaternion Quaternion::fromOrtho(const Float3x3& m) {
    return quaternionFromOrtho(m);
}

Quaternion Quaternion::fromOrtho(const Float4x4& m) {
    return quaternionFromOrtho(m);
}

} // namespace ply
