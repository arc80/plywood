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
    static const size_t Cols = 2;

    V col[Cols];

    Float2x2() = default;

    Float2x2(std::initializer_list<V> init) {
        PLY_ASSERT(init.size() == Cols);
        size_t i = 0;
        for (const V& c : init)
            col[i++] = c;
    }

    V& operator[](size_t i) {
        PLY_ASSERT(i < Cols);
        return col[i];
    }

    const V& operator[](size_t i) const {
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

    Float2 operator*(const Float2& arg) const {
        Float2 result;
        for (size_t r = 0; r < 2; r++) {
            float v = 0;
            for (size_t c = 0; c < 2; c++) {
                v += col[c][r] * arg[c];
            }
            result[r] = v;
        }
        return result;
    }

    Float2x2 operator*(const Float2x2& arg) const {
        Float2x2 result;
        for (size_t c = 0; c < 2; c++)
            result[c] = *this * arg.col[c];
        return result;
    }

    Float2x2 transposed() const {
        Float2x2 result;
        for (size_t c = 0; c < 2; c++)
            for (size_t r = 0; r < 2; r++)
                result.col[c][r] = col[r][c];
        return result;
    }

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
    static const size_t Cols = 3;

    V col[Cols];

    Float3x3() = default;

    Float3x3(std::initializer_list<V> init) {
        PLY_ASSERT(init.size() == Cols);
        size_t i = 0;
        for (const V& c : init)
            col[i++] = c;
    }

    V& operator[](size_t i) {
        PLY_ASSERT(i < Cols);
        return col[i];
    }

    const V& operator[](size_t i) const {
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

    Float3 operator*(const Float3& arg) const {
        Float3 result;
        for (size_t r = 0; r < 3; r++) {
            float v = 0;
            for (size_t c = 0; c < 3; c++) {
                v += col[c][r] * arg[c];
            }
            result[r] = v;
        }
        return result;
    }

    Float3x3 operator*(const Float3x3& arg) const {
        Float3x3 result;
        for (size_t c = 0; c < 3; c++)
            result[c] = *this * arg.col[c];
        return result;
    }

    Float3x3 transposed() const {
        Float3x3 result;
        for (size_t c = 0; c < 3; c++)
            for (size_t r = 0; r < 3; r++)
                result.col[c][r] = col[r][c];
        return result;
    }

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
    static const size_t Cols = 4;

    V col[Cols];

    Float3x4() = default;

    Float3x4(const Float3x3 m, const Float3 t) : col{m.col[0], m.col[1], m.col[2], t} {
    }

    Float3x4(std::initializer_list<V> init) {
        PLY_ASSERT(init.size() == Cols);
        size_t i = 0;
        for (const V& c : init)
            col[i++] = c;
    }

    Float3x3& asFloat3x3() {
        return reinterpret_cast<Float3x3&>(*this);
    }

    const Float3x3& asFloat3x3() const {
        return reinterpret_cast<const Float3x3&>(*this);
    }

    V& operator[](size_t i) {
        PLY_ASSERT(i < Cols);
        return col[i];
    }

    const V& operator[](size_t i) const {
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

    Float3 operator*(const Float3& arg) const {
        Float3 result;
        for (size_t r = 0; r < 3; r++) {
            result[r] = col[0][r] * arg[0] + col[1][r] * arg[1] + col[2][r] * arg[2] + col[3][r];
        }
        return result;
    }

    Float3 operator*(const Float4& arg) const {
        Float3 result;
        for (size_t r = 0; r < 3; r++) {
            result[r] =
                col[0][r] * arg[0] + col[1][r] * arg[1] + col[2][r] * arg[2] + col[3][r] * arg[3];
        }
        return result;
    }

    Float3x4 operator*(const Float3x4& b) const {
        Float3x4 result;
        for (size_t c = 0; c < 3; c++)
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
    static const size_t Cols = 4;

    V col[Cols];

    Float4x4() = default;

    Float4x4(std::initializer_list<V> init) {
        PLY_ASSERT(init.size() == Cols);
        size_t i = 0;
        for (const V& c : init)
            col[i++] = c;
    }

    V& operator[](size_t i) {
        PLY_ASSERT(i < Cols);
        return col[i];
    }

    const V& operator[](size_t i) const {
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

    Float4 operator*(const Float4& arg) const {
        Float4 result;
        for (size_t r = 0; r < 4; r++) {
            float v = 0;
            for (size_t c = 0; c < 4; c++) {
                v += col[c][r] * arg[c];
            }
            result[r] = v;
        }
        return result;
    }

    Float4x4 operator*(const Float4x4& arg) const {
        Float4x4 result;
        for (size_t c = 0; c < 4; c++)
            result[c] = *this * arg.col[c];
        return result;
    }

    Float4x4 operator*(const Float3x4& arg) const {
        Float4x4 result;
        for (size_t c = 0; c < 3; c++)
            result[c] = *this * Float4{arg.col[c], 0};
        result[3] = *this * Float4{arg.col[3], 1};
        return result;
    }

    friend Float4x4 operator*(const Float3x4& m0, const Float4x4& m1) {
        Float4x4 result;
        for (size_t c = 0; c < 4; c++)
            result[c] = Float4{m0 * m1.col[c], m1.col[c][3]};
        return result;
    }

    Float4x4 transposed() const {
        Float4x4 result;
        for (size_t c = 0; c < 4; c++)
            for (size_t r = 0; r < 4; r++)
                result.col[c][r] = col[r][c];
        return result;
    }

    Float4x4 invertedOrtho() const {
        Float4x4 result = transposed();
        result.col[0][3] = 0;
        result.col[1][3] = 0;
        result.col[2][3] = 0;
        result.col[3][3] = 1;
        result.col[3].asFloat3() = (result * -col[3]).asFloat3();
        return result;
    }

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

    // FIXME: This is coordinate system-specific, maybe should re-organize:
    static Float4x4 makeProjection(float fovY, float aspect, float zNear, float zFar) {
        Float4x4 result = Float4x4::zero();
        float f = 1 / tanf(fovY / 2);
        float ooZRange = 1 / (zNear - zFar);
        result.col[0][0] = f / aspect;
        result.col[1][1] = f;
        result.col[2][2] = (zNear + zFar) * ooZRange;
        result.col[2][3] = -1.f;
        result.col[3][2] = (2 * zNear * zFar) * ooZRange;
        return result;
    }

    // FIXME: This is coordinate system-specific, maybe should re-organize:
    static Float4x4 makeOrtho(const Rect& rect, float zNear, float zFar) {
        Float4x4 result = Float4x4::zero();
        float tow = 2 / rect.width();
        float toh = 2 / rect.height();
        float ooZRange = 1 / (zNear - zFar);
        result.col[0][0] = tow;
        result.col[3][0] = -rect.mid().x * tow;
        result.col[1][1] = toh;
        result.col[3][1] = -rect.mid().y * toh;
        result.col[2][2] = 2 * ooZRange;
        result.col[3][3] = 1.f;
        result.col[3][2] = (zNear + zFar) * ooZRange;
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
