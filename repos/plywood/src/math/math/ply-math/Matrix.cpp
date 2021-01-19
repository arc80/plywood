/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-math/Core.h>
#include <ply-math/Matrix.h>

namespace ply {

//--------------------------------------------
//  Float2x2
//--------------------------------------------
PLY_NO_INLINE Float2x2 Float2x2::identity() {
    return {{1, 0}, {0, 1}};
}

PLY_NO_INLINE Float2x2 Float2x2::makeScale(const Float2& scale) {
    return {{scale.x, 0}, {0, scale.y}};
}

PLY_NO_INLINE Float2x2 Float2x2::makeRotation(float radians) {
    return fromComplex(Complex::fromAngle(radians));
}

PLY_NO_INLINE Float2x2 Float2x2::fromComplex(const Float2& c) {
    return {{c.x, c.y}, {-c.y, c.x}};
}

PLY_NO_INLINE Float2x2 Float2x2::transposed() const {
    PLY_PUN_SCOPE
    auto* m = reinterpret_cast<const float(*)[2]>(this);
    return {
        {m[0][0], m[1][0]},
        {m[0][1], m[1][1]},
    };
}

PLY_NO_INLINE bool operator==(const Float2x2& a, const Float2x2& b) {
    return (a.col[0] == b.col[0]) && (a.col[1] == b.col[1]);
}

PLY_NO_INLINE Float2 operator*(const Float2x2& m_, const Float2& v_) {
    Float2 result;
    {
        PLY_PUN_SCOPE
        auto* res = reinterpret_cast<float*>(&result);
        auto* m = reinterpret_cast<const float(*)[2]>(&m_);
        auto* v = reinterpret_cast<const float*>(&v_);
        for (ureg r = 0; r < 2; r++) {
            res[r] = m[0][r] * v[0] + m[1][r] * v[1];
        }
    }
    return result;
}

PLY_NO_INLINE Float2x2 operator*(const Float2x2& a, const Float2x2& b) {
    Float2x2 result;
    for (ureg c = 0; c < 2; c++) {
        result[c] = a * b.col[c];
    }
    return result;
}

//--------------------------------------------
//  Float3x3
//--------------------------------------------
PLY_NO_INLINE Float3x3 Float3x3::identity() {
    return {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
}

PLY_NO_INLINE Float3x3 Float3x3::makeScale(const Float3& arg) {
    return {{arg.x, 0, 0}, {0, arg.y, 0}, {0, 0, arg.z}};
}

PLY_NO_INLINE Float3x3 Float3x3::makeRotation(const Float3& unitAxis, float radians) {
    return Float3x3::fromQuaternion(Quaternion::fromAxisAngle(unitAxis, radians));
}

PLY_NO_INLINE Float3x3 Float3x3::fromQuaternion(const Quaternion& q) {
    return {{1 - 2 * q.y * q.y - 2 * q.z * q.z, 2 * q.x * q.y + 2 * q.z * q.w,
             2 * q.x * q.z - 2 * q.y * q.w},
            {2 * q.x * q.y - 2 * q.z * q.w, 1 - 2 * q.x * q.x - 2 * q.z * q.z,
             2 * q.y * q.z + 2 * q.x * q.w},
            {2 * q.x * q.z + 2 * q.y * q.w, 2 * q.y * q.z - 2 * q.x * q.w,
             1 - 2 * q.x * q.x - 2 * q.y * q.y}};
}

PLY_NO_INLINE bool Float3x3::hasScale(float thresh) const {
    return !col[0].isUnit(thresh) || !col[1].isUnit(thresh) || !col[2].isUnit(thresh);
}

PLY_NO_INLINE Float3x3 Float3x3::transposed() const {
    PLY_PUN_SCOPE
    auto* m = reinterpret_cast<const float(*)[3]>(this);
    return {
        {m[0][0], m[1][0], m[2][0]},
        {m[0][1], m[1][1], m[2][1]},
        {m[0][2], m[1][2], m[2][2]},
    };
}

PLY_NO_INLINE bool operator==(const Float3x3& a_, const Float3x3& b_) {
    PLY_PUN_SCOPE
    auto* a = reinterpret_cast<const float*>(&a_);
    auto* b = reinterpret_cast<const float*>(&b_);
    for (ureg r = 0; r < 9; r++) {
        if (a[r] != b[r])
            return false;
    }
    return true;
}

PLY_NO_INLINE Float3 operator*(const Float3x3& m_, const Float3& v_) {
    Float3 result;
    {
        PLY_PUN_SCOPE
        auto* res = reinterpret_cast<float*>(&result);
        auto* m = reinterpret_cast<const float(*)[3]>(&m_);
        auto* v = reinterpret_cast<const float*>(&v_);
        for (ureg r = 0; r < 3; r++) {
            res[r] = m[0][r] * v[0] + m[1][r] * v[1] + m[2][r] * v[2];
        }
    }
    return result;
}

PLY_NO_INLINE Float3x3 operator*(const Float3x3& a, const Float3x3& b) {
    Float3x3 result;
    for (ureg c = 0; c < 3; c++) {
        result.col[c] = a * b.col[c];
    }
    return result;
}

//--------------------------------------------
//  Float3x4
//--------------------------------------------
PLY_NO_INLINE Float3x4::Float3x4(const Float3x3& m3x3, const Float3& pos) {
    for (u32 i = 0; i < 3; i++) {
        col[i] = m3x3.col[i];
    }
    col[3] = pos;
}

PLY_NO_INLINE Float4x4 Float3x4::toFloat4x4() const {
    return Float4x4{{col[0], 0}, {col[1], 0}, {col[2], 0}, {col[3], 1}};
}

PLY_NO_INLINE Float3x4 Float3x4::identity() {
    return {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}};
}

PLY_NO_INLINE Float3x4 Float3x4::makeScale(const Float3& arg) {
    return {{arg.x, 0, 0}, {0, arg.y, 0}, {0, 0, arg.z}, {0, 0, 0}};
}

PLY_NO_INLINE Float3x4 Float3x4::makeRotation(const Float3& unitAxis, float radians) {
    return Float3x4::fromQuaternion(Quaternion::fromAxisAngle(unitAxis, radians));
}

PLY_NO_INLINE Float3x4 Float3x4::makeTranslation(const Float3& pos) {
    return {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, pos};
}

PLY_NO_INLINE Float3x4 Float3x4::fromQuaternion(const Quaternion& q, const Float3& pos) {
    return {{1 - 2 * q.y * q.y - 2 * q.z * q.z, 2 * q.x * q.y + 2 * q.z * q.w,
             2 * q.x * q.z - 2 * q.y * q.w},
            {2 * q.x * q.y - 2 * q.z * q.w, 1 - 2 * q.x * q.x - 2 * q.z * q.z,
             2 * q.y * q.z + 2 * q.x * q.w},
            {2 * q.x * q.z + 2 * q.y * q.w, 2 * q.y * q.z - 2 * q.x * q.w,
             1 - 2 * q.x * q.x - 2 * q.y * q.y},
            pos};
}

PLY_NO_INLINE bool Float3x4::hasScale(float thresh) const {
    return asFloat3x3().hasScale(thresh);
}

PLY_NO_INLINE Float3x4 Float3x4::invertedOrtho() const {
    Float3x4 result;
    reinterpret_cast<Float3x3&>(result) = reinterpret_cast<const Float3x3&>(*this).transposed();
    result.col[3] = reinterpret_cast<Float3x3&>(result) * -col[3];
    return result;
}

PLY_NO_INLINE bool operator==(const Float3x4& a_, const Float3x4& b_) {
    PLY_PUN_SCOPE
    auto* a = reinterpret_cast<const float*>(&a_);
    auto* b = reinterpret_cast<const float*>(&b_);
    for (ureg r = 0; r < 12; r++) {
        if (a[r] != b[r])
            return false;
    }
    return true;
}

PLY_NO_INLINE Float3 operator*(const Float3x4& m_, const Float3& v_) {
    Float3 result;
    {
        PLY_PUN_SCOPE
        auto* res = reinterpret_cast<float*>(&result);
        auto* m = reinterpret_cast<const float(*)[3]>(&m_);
        auto* v = reinterpret_cast<const float*>(&v_);
        for (ureg r = 0; r < 3; r++) {
            res[r] = m[0][r] * v[0] + m[1][r] * v[1] + m[2][r] * v[2] + m[3][r];
        }
    }
    return result;
}

PLY_NO_INLINE Float4 operator*(const Float3x4& m_, const Float4& v_) {
    Float4 result;
    {
        PLY_PUN_SCOPE
        auto* res = reinterpret_cast<float*>(&result);
        auto* m = reinterpret_cast<const float(*)[3]>(&m_);
        auto* v = reinterpret_cast<const float*>(&v_);
        for (ureg r = 0; r < 3; r++) {
            res[r] = m[0][r] * v[0] + m[1][r] * v[1] + m[2][r] * v[2] + m[3][r] * v[3];
        }
        res[3] = v[3];
    }
    return result;
}

PLY_NO_INLINE Float3x4 operator*(const Float3x4& a, const Float3x4& b) {
    Float3x4 result;
    for (ureg c = 0; c < 3; c++) {
        result.col[c] = a.asFloat3x3() * b.col[c];
    }
    result.col[3] = a * b.col[3];
    return result;
}

//--------------------------------------------
//  Float4x4
//--------------------------------------------
PLY_NO_INLINE Float4x4::Float4x4(const Float3x3& m3x3, const Float3& pos) {
    for (u32 i = 0; i < 3; i++) {
        col[i] = {m3x3.col[i], 0};
    }
    col[3] = {pos, 1};
}

PLY_NO_INLINE Float3x3 Float4x4::toFloat3x3() const {
    return Float3x3{col[0].asFloat3(), col[1].asFloat3(), col[2].asFloat3()};
}

PLY_NO_INLINE Float3x4 Float4x4::toFloat3x4() const {
    return Float3x4{col[0].asFloat3(), col[1].asFloat3(), col[2].asFloat3(), col[3].asFloat3()};
}

PLY_NO_INLINE Float4x4 Float4x4::identity() {
    return {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
}

PLY_NO_INLINE Float4x4 Float4x4::makeScale(const Float3& arg) {
    return {{arg.x, 0, 0, 0}, {0, arg.y, 0, 0}, {0, 0, arg.z, 0}, {0, 0, 0, 1}};
}

PLY_NO_INLINE Float4x4 Float4x4::makeRotation(const Float3& unitAxis, float radians) {
    return Float4x4::fromQuaternion(Quaternion::fromAxisAngle(unitAxis, radians));
}

PLY_NO_INLINE Float4x4 Float4x4::makeTranslation(const Float3& pos) {
    return {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {pos, 1}};
}

PLY_NO_INLINE Float4x4 Float4x4::fromQuaternion(const Quaternion& q, const Float3& pos) {
    return {{1 - 2 * q.y * q.y - 2 * q.z * q.z, 2 * q.x * q.y + 2 * q.z * q.w,
             2 * q.x * q.z - 2 * q.y * q.w, 0},
            {2 * q.x * q.y - 2 * q.z * q.w, 1 - 2 * q.x * q.x - 2 * q.z * q.z,
             2 * q.y * q.z + 2 * q.x * q.w, 0},
            {2 * q.x * q.z + 2 * q.y * q.w, 2 * q.y * q.z - 2 * q.x * q.w,
             1 - 2 * q.x * q.x - 2 * q.y * q.y, 0},
            {pos, 1}};
}

PLY_NO_INLINE Float4x4 Float4x4::makeProjection(const Rect& frustum, float zNear, float zFar) {
    PLY_ASSERT(zNear > 0 && zFar > 0);
    Float4x4 result = {0, 0, 0, 0};
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

PLY_NO_INLINE Float4x4 Float4x4::makeOrtho(const Rect& rect, float zNear, float zFar) {
    Float4x4 result = {0, 0, 0, 0};
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

PLY_NO_INLINE Float4x4 Float4x4::transposed() const {
    PLY_PUN_SCOPE
    auto* m = reinterpret_cast<const float(*)[4]>(this);
    return {
        {m[0][0], m[1][0], m[2][0], m[3][0]},
        {m[0][1], m[1][1], m[2][1], m[3][1]},
        {m[0][2], m[1][2], m[2][2], m[3][2]},
        {m[0][3], m[1][3], m[2][3], m[3][3]},
    };
}

PLY_NO_INLINE Float4x4 Float4x4::invertedOrtho() const {
    Float4x4 result = transposed();
    result.col[0].w = 0;
    result.col[1].w = 0;
    result.col[2].w = 0;
    result.col[3] = result * -col[3];
    result.col[3].w = 1;
    return result;
}

PLY_NO_INLINE bool operator==(const Float4x4& a_, const Float4x4& b_) {
    PLY_PUN_SCOPE
    auto* a = reinterpret_cast<const float*>(&a_);
    auto* b = reinterpret_cast<const float*>(&b_);
    for (ureg r = 0; r < 16; r++) {
        if (a[r] != b[r])
            return false;
    }
    return true;
}

PLY_NO_INLINE Float4 operator*(const Float4x4& m_, const Float4& v_) {
    Float4 result;
    {
        PLY_PUN_SCOPE
        auto* res = reinterpret_cast<float*>(&result);
        auto* m = reinterpret_cast<const float(*)[4]>(&m_);
        auto* v = reinterpret_cast<const float*>(&v_);
        for (ureg r = 0; r < 4; r++) {
            res[r] = m[0][r] * v[0] + m[1][r] * v[1] + m[2][r] * v[2] + m[3][r] * v[3];
        }
    }
    return result;
}

PLY_NO_INLINE Float4x4 operator*(const Float4x4& a, const Float4x4& b) {
    Float4x4 result;
    for (ureg c = 0; c < 4; c++) {
        result.col[c] = a * b.col[c];
    }
    return result;
}

PLY_NO_INLINE Float4x4 operator*(const Float3x4& a, const Float4x4& b) {
    Float4x4 result;
    for (ureg c = 0; c < 4; c++) {
        result[c] = a * b.col[c];
    }
    return result;
}

PLY_NO_INLINE Float4x4 operator*(const Float4x4& a, const Float3x4& b) {
    Float4x4 result;
    for (ureg c = 0; c < 3; c++) {
        result.col[c] = a * Float4{b.col[c], 0};
    }
    result[3] = a * Float4{b.col[3], 1};
    return result;
}

} // namespace ply
