/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-math/Core.h>
#include <ply-math/Quaternion.h>
#include <ply-math/Matrix.h>

namespace ply {

PLY_NO_INLINE Quaternion Quaternion::fromAxisAngle(const Float3& unitAxis, float radians) {
    PLY_ASSERT(unitAxis.isUnit());
    float c = cosf(radians / 2);
    float s = sinf(radians / 2);
    return {s * unitAxis.x, s * unitAxis.y, s * unitAxis.z, c};
}

PLY_NO_INLINE Quaternion Quaternion::fromUnitVectors(const Float3& start, const Float3& end) {
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

template <typename M>
PLY_INLINE Quaternion quaternionFromOrtho(M m) {
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

PLY_NO_INLINE Quaternion Quaternion::fromOrtho(const Float3x3& m) {
    PLY_PUN_SCOPE
    return quaternionFromOrtho(reinterpret_cast<const float(*)[3]>(&m));
}

PLY_NO_INLINE Quaternion Quaternion::fromOrtho(const Float4x4& m) {
    PLY_PUN_SCOPE
    return quaternionFromOrtho(reinterpret_cast<const float(*)[4]>(&m));
}

PLY_NO_INLINE Float3 Quaternion::rotateUnitX() const {
    return {1 - 2 * y * y - 2 * z * z, 2 * x * y + 2 * z * w, 2 * x * z - 2 * y * w};
}

PLY_NO_INLINE Float3 Quaternion::rotateUnitY() const {
    return {2 * x * y - 2 * z * w, 1 - 2 * x * x - 2 * z * z, 2 * y * z + 2 * x * w};
}

PLY_NO_INLINE Float3 Quaternion::rotateUnitZ() const {
    return {2 * x * z + 2 * y * w, 2 * y * z - 2 * x * w, 1 - 2 * x * x - 2 * y * y};
}

PLY_NO_INLINE Quaternion Quaternion::negatedIfCloserTo(const Quaternion& other) const {
    const Float4& v0 = asFloat4();
    const Float4& v1 = other.asFloat4();
    return (v0 - v1).length2() < (-v0 - v1).length2() ? v0.asQuaternion() : (-v0).asQuaternion();
}

PLY_NO_INLINE Float3 operator*(const Quaternion& q, const Float3& v) {
    // From https://gist.github.com/rygorous/8da6651b597f3d825862
    Float3 t = cross(q.asFloat3(), v) * 2.f;
    return v + t * q.w + cross(q.asFloat3(), t);
}

PLY_NO_INLINE Quaternion operator*(const Quaternion& a, const Quaternion& b) {
    return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
            a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}

PLY_NO_INLINE Quaternion mix(const Quaternion& a, const Quaternion& b, float f) {
    return mix(a.negatedIfCloserTo(b).asFloat4(), b.asFloat4(), f).normalized().asQuaternion();
}

} // namespace ply
