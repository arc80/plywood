/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-math/Core.h>
#include <ply-math/Vector.h>
#include <ply-math/Quaternion.h>
#include <ply-math/Complex.h>
#include <ply-math/Box.h>
#include <initializer_list>

namespace ply {

struct Float2x2;
struct Float3x3;
struct Float3x4;
struct Float4x4;

//----------------------------------------------------
// Float2x2
//----------------------------------------------------

struct Float2x2 {
    typedef Float2 V;
    static const ureg Cols = 2;

    V col[Cols];

    Float2x2() = default;

    Float2x2(std::initializer_list<V> init) {
        PLY_ASSERT(init.size() == Cols);
        ureg i = 0;
        for (const V& c : init)
            col[i++] = c;
    }

    V& operator[](ureg i) {
        PLY_ASSERT(i < Cols);
        return col[i];
    }

    const V& operator[](ureg i) const {
        PLY_ASSERT(i < Cols);
        return col[i];
    }

    static Float2x2 zero() {
        return Float2x2{{0, 0}, {0, 0}};
    }

    static Float2x2 identity() {
        return Float2x2{{1, 0}, {0, 1}};
    }

    bool operator==(const Float2x2 arg) const {
        return (col[0] == arg.col[0]) && (col[1] == arg.col[1]);
    }

    Float2 operator*(const Float2& arg) const;

    Float2x2 operator*(const Float2x2& arg) const;

    Float2x2 transposed() const;

    static Float2x2 makeScale(const Float2& arg) {
        return Float2x2{{arg.x, 0}, {0, arg.y}};
    }

    static Float2x2 makeScale(float arg) {
        return Float2x2{{arg, 0}, {0, arg}};
    }

    static Float2x2 fromComplex(const Float2& r) {
        return Float2x2{{r.x, r.y}, {-r.y, r.x}};
    }

    static Float2x2 makeRotation(float angle) {
        return fromComplex(Complex::fromAngle(angle));
    }
};

//----------------------------------------------------
// Float3x3
//----------------------------------------------------

struct Float3x3 {
    typedef Float3 V;
    static const ureg Cols = 3;

    V col[Cols];

    Float3x3() = default;

    Float3x3(std::initializer_list<V> init) {
        PLY_ASSERT(init.size() == Cols);
        ureg i = 0;
        for (const V& c : init)
            col[i++] = c;
    }

    V& operator[](ureg i) {
        PLY_ASSERT(i < Cols);
        return col[i];
    }

    const V& operator[](ureg i) const {
        PLY_ASSERT(i < Cols);
        return col[i];
    }

    static Float3x3 zero() {
        return Float3x3{{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    }

    static Float3x3 identity() {
        return Float3x3{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    }

    bool operator==(const Float3x3& arg) const {
        return (col[0] == arg.col[0]) && (col[1] == arg.col[1]) && (col[2] == arg.col[2]);
    }

    Float3x4 toFloat3x4(const Float3& translate = {0, 0, 0}) const;
    Float4x4 toFloat4x4(const Float3& translate = {0, 0, 0}) const;

    Float3 operator*(const Float3& arg) const;

    Float3x3 operator*(const Float3x3& arg) const;

    Float3x3 transposed() const;

    bool hasScale(float thresh = 0.001f) const {
        return !col[0].isUnit() || !col[1].isUnit() || !col[2].isUnit();
    }

    static Float3x3 makeScale(const Float3& arg) {
        return Float3x3{{arg.x, 0, 0}, {0, arg.y, 0}, {0, 0, arg.z}};
    }

    static Float3x3 makeScale(float arg) {
        return Float3x3{{arg, 0, 0}, {0, arg, 0}, {0, 0, arg}};
    }

    static Float3x3 makeRotation(const Float3& unitAxis, float angle) {
        return Quaternion::fromAxisAngle(unitAxis, angle).toFloat3x3();
    }
};

inline Float3x3 Quaternion::toFloat3x3() const {
    return {
        Float3{1 - 2 * y * y - 2 * z * z, 2 * x * y + 2 * z * w, 2 * x * z - 2 * y * w},
        Float3{2 * x * y - 2 * z * w, 1 - 2 * x * x - 2 * z * z, 2 * y * z + 2 * x * w},
        Float3{2 * x * z + 2 * y * w, 2 * y * z - 2 * x * w, 1 - 2 * x * x - 2 * y * y},
    };
}

//----------------------------------------------------
// Float3x4
//----------------------------------------------------

struct Float3x4 {
    typedef Float3 V;
    static const ureg Cols = 4;

    V col[Cols];

    Float3x4() = default;

    Float3x4(const Float3x3 m, const Float3 t) : col{m.col[0], m.col[1], m.col[2], t} {
    }

    Float3x4(std::initializer_list<V> init) {
        PLY_ASSERT(init.size() == Cols);
        ureg i = 0;
        for (const V& c : init)
            col[i++] = c;
    }

    Float3x3& asFloat3x3() {
        return reinterpret_cast<Float3x3&>(*this);
    }

    const Float3x3& asFloat3x3() const {
        return reinterpret_cast<const Float3x3&>(*this);
    }

    V& operator[](ureg i) {
        PLY_ASSERT(i < Cols);
        return col[i];
    }

    const V& operator[](ureg i) const {
        PLY_ASSERT(i < Cols);
        return col[i];
    }

    bool hasScale(float thresh = 0.001f) const {
        return asFloat3x3().hasScale(thresh);
    }

    static Float3x4 zero() {
        return Float3x4{{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    }

    static Float3x4 identity() {
        return Float3x4{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}};
    }

    Float4x4 toFloat4x4() const;

    bool operator==(const Float3x4 arg) const {
        return (col[0] == arg.col[0]) && (col[1] == arg.col[1]) && (col[2] == arg.col[2]) &&
               (col[3] == arg.col[3]);
    }

    Float3 operator*(const Float3& arg) const;

    Float3 operator*(const Float4& arg) const;

    Float3x4 operator*(const Float3x4& b) const {
        Float3x4 result;
        for (ureg c = 0; c < 3; c++)
            result[c] = asFloat3x3() * b.col[c];
        result[3] = *this * b.col[3];
        return result;
    }

    Float3x4 invertedOrtho() const {
        Float3x4 result;
        reinterpret_cast<Float3x3&>(result) = reinterpret_cast<const Float3x3&>(*this).transposed();
        result.col[3] = reinterpret_cast<Float3x3&>(result) * -col[3];
        return result;
    }

    static Float3x4 makeScale(const Float3& arg) {
        return Float3x4{{arg.x, 0, 0}, {0, arg.y, 0}, {0, 0, arg.z}, {0, 0, 0}};
    }

    static Float3x4 makeScale(float arg) {
        return Float3x4{{arg, 0, 0}, {0, arg, 0}, {0, 0, arg}, {0, 0, 0}};
    }

    static Float3x4 makeRotation(const Float3& unitAxis, float angle) {
        return Quaternion::fromAxisAngle(unitAxis, angle).toFloat3x4();
    }

    static Float3x4 makeTranslation(const Float3& arg) {
        return Float3x4{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, arg};
    }
};

inline Float3x4 Quaternion::toFloat3x4(const Float3& xlate) const {
    return {{1 - 2 * y * y - 2 * z * z, 2 * x * y + 2 * z * w, 2 * x * z - 2 * y * w},
            {2 * x * y - 2 * z * w, 1 - 2 * x * x - 2 * z * z, 2 * y * z + 2 * x * w},
            {2 * x * z + 2 * y * w, 2 * y * z - 2 * x * w, 1 - 2 * x * x - 2 * y * y},
            xlate};
}

//----------------------------------------------------
// Float4x4
//----------------------------------------------------

struct Float4x4 {
    typedef Float4 V;
    static const ureg Cols = 4;

    V col[Cols];

    Float4x4() = default;

    Float4x4(std::initializer_list<V> init) {
        PLY_ASSERT(init.size() == Cols);
        ureg i = 0;
        for (const V& c : init)
            col[i++] = c;
    }

    V& operator[](ureg i) {
        PLY_ASSERT(i < Cols);
        return col[i];
    }

    const V& operator[](ureg i) const {
        PLY_ASSERT(i < Cols);
        return col[i];
    }

    const Float3& origin() const {
        return col[3].asFloat3();
    }

    static Float4x4 zero() {
        return Float4x4{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
    }

    static Float4x4 identity() {
        return Float4x4{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
    }

    Float3x3 toFloat3x3() const {
        return Float3x3{col[0].asFloat3(), col[1].asFloat3(), col[2].asFloat3()};
    }

    Float3x4 toFloat3x4() const {
        return Float3x4{col[0].asFloat3(), col[1].asFloat3(), col[2].asFloat3(), col[3].asFloat3()};
    }

    bool operator==(const Float4x4 arg) const {
        return (col[0] == arg.col[0]) && (col[1] == arg.col[1]) && (col[2] == arg.col[2]) &&
               (col[3] == arg.col[3]);
    }

    Float4 operator*(const Float4& arg) const;

    Float4x4 operator*(const Float4x4& arg) const;

    Float4x4 operator*(const Float3x4& arg) const;

    Float4x4 transposed() const;

    Float4x4 invertedOrtho() const;

    static Float4x4 makeScale(const Float3& arg) {
        return {{arg.x, 0, 0, 0}, {0, arg.y, 0, 0}, {0, 0, arg.z, 0}, {0, 0, 0, 1}};
    }

    static Float4x4 makeScale(float arg) {
        return {{arg, 0, 0, 0}, {0, arg, 0, 0}, {0, 0, arg, 0}, {0, 0, 0, 1}};
    }

    static Float4x4 makeRotation(const Float3& unitAxis, float angle) {
        return Quaternion::fromAxisAngle(unitAxis, angle).toFloat4x4();
    }

    static Float4x4 makeTranslation(const Float3& arg) {
        return {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {arg, 1}};
    }

    // frustum is on Z = -1 plane
    static Float4x4 makeProjection(const Rect& frustum, float zNear, float zFar) {
        Float4x4 result = Float4x4::zero();
        float ooXDenom = 1.f / (frustum.maxs.x - frustum.mins.x);
        float ooYDenom = 1.f / (frustum.maxs.y - frustum.mins.y);
        float ooZDenom = 1.f / (zNear - zFar);
        result.col[0].x = 2.f * ooXDenom;
        result.col[2].x = (frustum.mins.x + frustum.maxs.x) * ooXDenom;
        result.col[1].y = 2.f * ooYDenom;
        result.col[2].y = (frustum.mins.y + frustum.maxs.y) * ooXDenom;
        result.col[2].z = (zNear + zFar) * ooZDenom;
        result.col[2].w = -1.f;
        result.col[3].z = (2 * zNear * zFar) * ooZDenom;
        return result;
    }

    static PLY_INLINE Float4x4 makeProjection(float fovY, float aspect, float zNear, float zFar) {
        float halfTanY = tanf(fovY / 2);
        return makeProjection(expand(Rect{{0, 0}}, {halfTanY * aspect, halfTanY}), zNear, zFar);
    }

    // FIXME: This is coordinate system-specific, maybe should re-organize:
    static Float4x4 makeOrtho(const Rect& rect, float zNear, float zFar) {
        Float4x4 result = Float4x4::zero();
        float tow = 2 / rect.width();
        float toh = 2 / rect.height();
        float ooZRange = 1 / (zNear - zFar);
        result.col[0].x = tow;
        result.col[3].x = -rect.mid().x * tow;
        result.col[1].y = toh;
        result.col[3].y = -rect.mid().y * toh;
        result.col[2].z = 2 * ooZRange;
        result.col[3].z = (zNear + zFar) * ooZRange;
        result.col[3].w = 1.f;
        return result;
    }
};

inline Float4x4 Quaternion::toFloat4x4(const Float3& xlate) const {
    return {{1 - 2 * y * y - 2 * z * z, 2 * x * y + 2 * z * w, 2 * x * z - 2 * y * w, 0},
            {2 * x * y - 2 * z * w, 1 - 2 * x * x - 2 * z * z, 2 * y * z + 2 * x * w, 0},
            {2 * x * z + 2 * y * w, 2 * y * z - 2 * x * w, 1 - 2 * x * x - 2 * y * y, 0},
            {xlate, 1}};
}

inline Float3x4 Float3x3::toFloat3x4(const Float3& translate) const {
    return Float3x4{col[0], col[1], col[2], translate};
}

inline Float4x4 Float3x3::toFloat4x4(const Float3& translate) const {
    return Float4x4{{col[0], 0}, {col[1], 0}, {col[2], 0}, {translate, 1}};
}

inline Float4x4 Float3x4::toFloat4x4() const {
    return Float4x4{{col[0], 0}, {col[1], 0}, {col[2], 0}, {col[3], 1}};
}

} // namespace ply
