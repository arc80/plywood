/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-math/Core.h>
#include <ply-math/Matrix.h>

namespace ply {

PLY_NO_INLINE Float2 Float2x2::operator*(const Float2& arg) const {
    Float2 result;
    {
        PLY_PUN_SCOPE
        auto* res = reinterpret_cast<float*>(&result);
        auto* m = reinterpret_cast<const float(*)[2]>(this);
        auto* a = reinterpret_cast<const float*>(&arg);
        for (ureg r = 0; r < 2; r++) {
            res[r] = m[0][r] * a[0] + m[1][r] * a[1];
        }
    }
    return result;
}

PLY_NO_INLINE Float2x2 Float2x2::operator*(const Float2x2& arg) const {
    Float2x2 result;
    for (ureg c = 0; c < 2; c++) {
        result[c] = *this * arg.col[c];
    }
    return result;
}

PLY_NO_INLINE Float2x2 Float2x2::transposed() const {
    PLY_PUN_SCOPE
    auto* m = reinterpret_cast<const float(*)[2]>(this);
    return {
        {m[0][0], m[1][0]},
        {m[0][1], m[1][1]},
    };
}

PLY_NO_INLINE Float3 Float3x3::operator*(const Float3& arg) const {
    Float3 result;
    {
        PLY_PUN_SCOPE
        auto* res = reinterpret_cast<float*>(&result);
        auto* m = reinterpret_cast<const float(*)[3]>(this);
        auto* a = reinterpret_cast<const float*>(&arg);
        for (ureg r = 0; r < 3; r++) {
            res[r] = m[0][r] * a[0] + m[1][r] * a[1] + m[2][r] * a[2];
        }
    }
    return result;
}

PLY_NO_INLINE Float3x3 Float3x3::operator*(const Float3x3& arg) const {
    Float3x3 result;
    for (ureg c = 0; c < 3; c++) {
        result[c] = *this * arg.col[c];
    }
    return result;
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

PLY_NO_INLINE Float3 Float3x4::operator*(const Float3& arg) const {
    Float3 result;
    {
        PLY_PUN_SCOPE
        auto* res = reinterpret_cast<float*>(&result);
        auto* m = reinterpret_cast<const float(*)[3]>(this);
        auto* a = reinterpret_cast<const float*>(&arg);
        for (ureg r = 0; r < 3; r++) {
            res[r] = m[0][r] * a[0] + m[1][r] * a[1] + m[2][r] * a[2] + m[3][r];
        }
    }
    return result;
}

PLY_NO_INLINE Float3 Float3x4::operator*(const Float4& arg) const {
    Float3 result;
    {
        PLY_PUN_SCOPE
        auto* res = reinterpret_cast<float*>(&result);
        auto* m = reinterpret_cast<const float(*)[3]>(this);
        auto* a = reinterpret_cast<const float*>(&arg);
        for (ureg r = 0; r < 3; r++) {
            res[r] = m[0][r] * a[0] + m[1][r] * a[1] + m[2][r] * a[2] + m[3][r] * a[3];
        }
    }
    return result;
}

Float4 Float4x4::operator*(const Float4& arg) const {
    Float4 result;
    {
        PLY_PUN_SCOPE
        auto* res = reinterpret_cast<float*>(&result);
        auto* m = reinterpret_cast<const float(*)[4]>(this);
        auto* a = reinterpret_cast<const float*>(&arg);
        for (ureg r = 0; r < 4; r++) {
            res[r] = m[0][r] * a[0] + m[1][r] * a[1] + m[2][r] * a[2] + m[3][r] * a[3];
        }
    }
    return result;
}

Float4x4 Float4x4::operator*(const Float4x4& arg) const {
    Float4x4 result;
    for (ureg c = 0; c < 4; c++) {
        result[c] = *this * arg.col[c];
    }
    return result;
}

Float4x4 Float4x4::operator*(const Float3x4& arg) const {
    Float4x4 result;
    for (ureg c = 0; c < 3; c++) {
        result[c] = *this * Float4{arg.col[c], 0};
    }
    result[3] = *this * Float4{arg.col[3], 1};
    return result;
}

Float4x4 operator*(const Float3x4& m0, const Float4x4& m1) {
    Float4x4 result;
    for (ureg c = 0; c < 4; c++) {
        result[c] = Float4{m0 * m1.col[c], m1.col[c].w};
    }
    return result;
}

Float4x4 Float4x4::transposed() const {
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

} // namespace ply
