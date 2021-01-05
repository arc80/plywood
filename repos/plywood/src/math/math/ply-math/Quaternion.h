/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-math/Core.h>
#include <ply-math/Vector.h>

namespace ply {

struct Float3x3;
struct Float3x4;
struct Float4x4;

struct Quaternion {
    float x;
    float y;
    float z;
    float w;

    Quaternion() {
    }

    Quaternion(const Float3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {
    }

    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {
    }

    Float3& asFloat3() {
        return reinterpret_cast<Float3&>(*this);
    }

    const Float3& asFloat3() const {
        return reinterpret_cast<const Float3&>(*this);
    }

    Float4& asFloat4() {
        return reinterpret_cast<Float4&>(*this);
    }

    const Float4& asFloat4() const {
        return reinterpret_cast<const Float4&>(*this);
    }

    static Quaternion identity() {
        return {0, 0, 0, 1};
    }

    Quaternion operator-() const {
        return {-x, -y, -z, -w};
    }

    static Quaternion fromAxisAngle(const Float3& unitAxis, float radians) {
        float c = cosf(radians / 2);
        float s = sinf(radians / 2);
        return {s * unitAxis.x, s * unitAxis.y, s * unitAxis.z, c};
    }

    static Quaternion fromUnitVectors(const Float3& start, const Float3& end);

    friend Quaternion operator*(const Quaternion& a, const Quaternion& b) {
        return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
                a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
                a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
    }

    friend Float3 operator*(const Quaternion& quat, const Float3& v) {
        // From https://gist.github.com/rygorous/8da6651b597f3d825862
        Float3 t = cross(quat.asFloat3(), v) * 2.f;
        return v + t * quat.w + cross(quat.asFloat3(), t);
    }

    Float3 rotateUnitX() const {
        return {1 - 2 * y * y - 2 * z * z, 2 * x * y + 2 * z * w, 2 * x * z - 2 * y * w};
    }

    Float3 rotateUnitY() const {
        return {2 * x * y - 2 * z * w, 1 - 2 * x * x - 2 * z * z, 2 * y * z + 2 * x * w};
    }

    Float3 rotateUnitZ() const {
        return {2 * x * z + 2 * y * w, 2 * y * z - 2 * x * w, 1 - 2 * x * x - 2 * y * y};
    }

    Quaternion renormalized() const {
        return asFloat4().normalized().asQuaternion();
    }

    Quaternion inverted() const {
        // Small rotations have large w component, so prefer to keep the same sign of w.
        // Better for interpolation.
        return {-x, -y, -z, w};
    }

    Quaternion negatedIfCloserTo(const Quaternion& other) const {
        const Float4& v0 = asFloat4();
        const Float4& v1 = other.asFloat4();
        return (v0 - v1).length2() < (-v0 - v1).length2() ? v0.asQuaternion()
                                                          : (-v0).asQuaternion();
    }

    static Quaternion fromOrtho(const Float3x3& m);
    static Quaternion fromOrtho(const Float4x4& m);
};

PLY_INLINE const Quaternion& Float4::asQuaternion() const {
    PLY_COMPILER_BARRIER();
    return reinterpret_cast<const Quaternion&>(*this);
}

inline Quaternion mix(const Quaternion& lo, const Quaternion& hi, float f) {
    return mix(lo.negatedIfCloserTo(hi).asFloat4(), hi.asFloat4(), f).normalized().asQuaternion();
}

} // namespace ply
