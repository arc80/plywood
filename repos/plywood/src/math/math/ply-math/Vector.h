/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-math/Core.h>
#include <ply-math/Box.h>

namespace ply {

struct Quaternion;

//----------------------------------------------------
// Float2
//----------------------------------------------------

struct Float2 {
    typedef float T;
    static const size_t Rows = 2;

    float x;
    float y;

    Float2() = default;

    Float2(float t) : x{t}, y{t} {
    }

    Float2(float x, float y) : x{x}, y{y} {
    }

    T& operator[](size_t i) {
        PLY_ASSERT(i < Rows);
        return reinterpret_cast<float*>(this)[i];
    }

    const T& operator[](size_t i) const {
        PLY_ASSERT(i < Rows);
        return reinterpret_cast<const float*>(this)[i];
    }

    template <typename OtherVec2>
    OtherVec2 to() const {
        return {typename OtherVec2::T(x), typename OtherVec2::T(y)};
    }

    void operator=(const Float2& arg) {
        x = arg.x;
        y = arg.y;
    }

    bool operator==(const Float2& arg) const {
        return (x == arg.x) && (y == arg.y);
    }

    bool operator!=(const Float2& arg) const {
        return !(*this == arg);
    }

    bool operator<(const Float2& arg) const {
        if (x == arg.x)
            return y < arg.y;
        else
            return x < arg.x;
    }

    Float2 operator-() const {
        return {-x, -y};
    }

    Float2 operator+(const Float2& arg) const {
        return {x + arg.x, y + arg.y};
    }

    Float2 operator+(float arg) const {
        return {x + arg, y + arg};
    }

    friend Float2 operator+(float a, const Float2& b) {
        return {a + b.x, a + b.y};
    }

    void operator+=(const Float2& arg) {
        x += arg.x;
        y += arg.y;
    }

    Float2 operator-(const Float2& arg) const {
        return {x - arg.x, y - arg.y};
    }

    Float2 operator-(float arg) const {
        return {x - arg, y - arg};
    }

    friend Float2 operator-(float a, const Float2& b) {
        return {a - b.x, a - b.y};
    }

    void operator-=(const Float2& arg) {
        x -= arg.x;
        y -= arg.y;
    }

    Float2 operator*(float arg) const {
        return {x * arg, y * arg};
    }

    void operator*=(float arg) {
        x *= arg;
        y *= arg;
    }

    Float2 operator*(const Float2& arg) const {
        return {x * arg.x, y * arg.y};
    }

    void operator*=(const Float2& arg) {
        x *= arg.x;
        y *= arg.y;
    }

    // FIXME: Should implement other operators this way? (with scalar as first argument)
    inline friend Float2 operator/(float a, const Float2& b) {
        return {a / b.x, a / b.y};
    }

    Float2 operator/(float arg) const {
        return *this * (1.f / arg);
    }

    void operator/=(float arg) {
        *this *= (1.f / arg);
    }

    Float2 operator/(const Float2& arg) const {
        return {x / arg.x, y / arg.y};
    }

    void operator/=(const Float2& arg) {
        x /= arg.x;
        y /= arg.y;
    }

    float length2() const {
        return x * x + y * y;
    }

    float length() const {
        return sqrtf(length2());
    }

    bool isUnit(float thresh = 0.001f) const {
        return fabsf(1.f - length2()) < thresh;
    }

    Float2 normalized() const {
        return *this / length();
    }

    Float2 yx() const {
        return {y, x};
    }

    template <typename Hasher>
    void appendTo(Hasher& hasher) const {
        for (u32 i = 0; i < 2; i++) {
            hasher.append(((const float*) this)[i]);
        }
    }
};

inline float dot(const Float2& a, const Float2& b) {
    return a.x * b.x + a.y * b.y;
}

inline Float2 clamp(const Float2& v, const Float2& mins, const Float2& maxs) {
    return {clamp(v.x, mins.x, maxs.x), clamp(v.y, mins.y, maxs.y)};
}

inline float cross(const Float2& a, const Float2& b) {
    return a.x * b.y - a.y * b.x;
}

inline Float2 abs(const Float2& a) {
    return {fabsf(a.x), fabsf(a.y)};
}

inline Float2 min(const Float2& a, float b) {
    return {min(a.x, b), min(a.y, b)};
}

inline Float2 max(const Float2& a, float b) {
    return {max(a.x, b), max(a.y, b)};
}

inline Float2 min(const Float2& a, const Float2& b) {
    return {min(a.x, b.x), min(a.y, b.y)};
}

inline Float2 max(const Float2& a, const Float2& b) {
    return {max(a.x, b.x), max(a.y, b.y)};
}

inline bool allLessThan(const Float2& a, const Float2& b) {
    return (a.x < b.x) && (a.y < b.y);
}

inline bool allLessThanOrEqual(const Float2& a, const Float2& b) {
    return (a.x <= b.x) && (a.y <= b.y);
}

inline Float2 quantizeNearest(const Float2& value, float spacing) {
    return {quantizeNearest(value.x, spacing), quantizeNearest(value.y, spacing)};
}

inline bool isQuantized(const Float2& value, float spacing) {
    return quantizeNearest(value, spacing) == value;
}

inline Float2 quantizeDown(const Float2& value, float spacing) {
    return {quantizeDown(value.x, spacing), quantizeDown(value.y, spacing)};
}

inline Float2 quantizeUp(const Float2& value, float spacing) {
    return {quantizeUp(value.x, spacing), quantizeUp(value.y, spacing)};
}

//----------------------------------------------------
// Float3
//----------------------------------------------------

struct Float3 {
    typedef float T;
    static const size_t Rows = 3;

    float x;
    float y;
    float z;

    Float3() = default;

    Float3(float t) : x{t}, y{t}, z{t} {
    }

    Float3(float x, float y, float z) : x{x}, y{y}, z{z} {
    }

    Float3(const Float2& v, float z) : x{v.x}, y{v.y}, z{z} {
    }

    Float2& asFloat2() {
        return reinterpret_cast<Float2&>(*this);
    }

    const Float2& asFloat2() const {
        return reinterpret_cast<const Float2&>(*this);
    }

    T& operator[](size_t i) {
        PLY_ASSERT(i < Rows);
        return reinterpret_cast<float*>(this)[i];
    }

    const T& operator[](size_t i) const {
        PLY_ASSERT(i < Rows);
        return reinterpret_cast<const float*>(this)[i];
    }

    template <typename OtherVec3>
    OtherVec3 to() const {
        return {typename OtherVec3::T(x), typename OtherVec3::T(y), typename OtherVec3::T(z)};
    }

    float& r() {
        return x;
    }

    float r() const {
        return x;
    }

    float& g() {
        return y;
    }

    float g() const {
        return y;
    }

    float& b() {
        return z;
    }

    float b() const {
        return z;
    }

    void operator=(const Float3& arg) {
        x = arg.x;
        y = arg.y;
        z = arg.z;
    }

    bool operator==(const Float3& arg) const {
        return (x == arg.x) && (y == arg.y) && (z == arg.z);
    }

    bool operator!=(const Float3& arg) const {
        return !(*this == arg);
    }

    bool operator<(const Float3& arg) const {
        if (x == arg.x) {
            if (y == arg.y)
                return z < arg.z;
            else
                return y < arg.y;
        } else
            return x < arg.x;
    }

    Float3 operator-() const {
        return {-x, -y, -z};
    }

    Float3 operator+(const Float3& arg) const {
        return {x + arg.x, y + arg.y, z + arg.z};
    }

    Float3 operator+(float arg) const {
        return {x + arg, y + arg, z + arg};
    }

    friend Float3 operator+(float a, const Float3& b) {
        return {a + b.x, a + b.y, a + b.z};
    }

    void operator+=(const Float3& arg) {
        x += arg.x;
        y += arg.y;
        z += arg.z;
    }

    Float3 operator-(const Float3& arg) const {
        return {x - arg.x, y - arg.y, z - arg.z};
    }

    Float3 operator-(float arg) const {
        return {x - arg, y - arg, z - arg};
    }

    friend Float3 operator-(float a, const Float3& b) {
        return {a - b.x, a - b.y, a - b.z};
    }

    void operator-=(const Float3& arg) {
        x -= arg.x;
        y -= arg.y;
        z -= arg.z;
    }

    Float3 operator*(float arg) const {
        return {x * arg, y * arg, z * arg};
    }

    void operator*=(float arg) {
        x *= arg;
        y *= arg;
        z *= arg;
    }

    Float3 operator*(const Float3& arg) const {
        return {x * arg.x, y * arg.y, z * arg.z};
    }

    void operator*=(const Float3& arg) {
        x *= arg.x;
        y *= arg.y;
        z *= arg.z;
    }

    Float3 operator/(float arg) const {
        return *this * (1.f / arg);
    }

    void operator/=(float arg) {
        *this *= (1.f / arg);
    }

    Float3 operator/(const Float3& arg) const {
        return {x / arg.x, y / arg.y, z / arg.z};
    }

    void operator/=(const Float3& arg) {
        x /= arg.x;
        y /= arg.y;
        z /= arg.z;
    }

    float length2() const {
        return x * x + y * y + z * z;
    }

    float length() const {
        return sqrtf(length2());
    }

    bool isUnit(float thresh = 0.001f) const {
        return fabsf(1.f - length2()) < thresh;
    }

    PLY_NO_DISCARD Float3 normalized() const {
        return *this / length();
    }

    bool safeNormalize() {
        float L = length();
        if (L < 1e-20f)
            return false;
        *this /= L;
        return true;
    }

    PLY_NO_DISCARD Float3 safeNormalized(const Float3& fallback) const {
        float L = length2();
        if (L < 1e-20f)
            return fallback;
        return *this / sqrtf(L);
    }

    Float2 xz() const {
        return {x, z};
    }

    template <typename Hasher>
    void appendTo(Hasher& hasher) const {
        for (u32 i = 0; i < 3; i++) {
            hasher.append(((const float*) this)[i]);
        }
    }
};

inline float dot(const Float3& a, const Float3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Float3 clamp(const Float3& v, const Float3& mins, const Float3& maxs) {
    return {clamp(v.x, mins.x, maxs.x), clamp(v.y, mins.y, maxs.y), clamp(v.z, mins.z, maxs.z)};
}

inline Float3 cross(const Float3& a, const Float3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline Float3 abs(const Float3& a) {
    return {fabsf(a.x), fabsf(a.y), fabsf(a.z)};
}

inline Float3 pow(const Float3& a, const Float3& b) {
    return {powf(a.x, b.x), powf(a.y, b.y), powf(a.z, b.z)};
}

inline Float3 min(const Float3& a, float b) {
    return {min(a.x, b), min(a.y, b), min(a.z, b)};
}

inline Float3 max(const Float3& a, float b) {
    return {max(a.x, b), max(a.y, b), max(a.z, b)};
}

inline Float3 min(const Float3& a, const Float3& b) {
    return {min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)};
}

inline Float3 max(const Float3& a, const Float3& b) {
    return {max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)};
}

inline bool allLessThan(const Float3& a, const Float3& b) {
    return (a.x < b.x) && (a.y < b.y) && (a.z < b.z);
}

inline bool allLessThanOrEqual(const Float3& a, const Float3& b) {
    return (a.x <= b.x) && (a.y <= b.y) && (a.z <= b.z);
}

inline Float3 quantizeNearest(const Float3& value, float spacing) {
    return {quantizeNearest(value.x, spacing), quantizeNearest(value.y, spacing),
            quantizeNearest(value.z, spacing)};
}

inline bool isQuantized(const Float3& value, float spacing) {
    return quantizeNearest(value, spacing) == value;
}

inline Float3 quantizeDown(const Float3& value, float spacing) {
    return {quantizeDown(value.x, spacing), quantizeDown(value.y, spacing),
            quantizeDown(value.z, spacing)};
}

inline Float3 quantizeUp(const Float3& value, float spacing) {
    return {quantizeUp(value.x, spacing), quantizeUp(value.y, spacing),
            quantizeUp(value.z, spacing)};
}

//----------------------------------------------------
// Float4
//----------------------------------------------------

struct Float4 {
    typedef float T;
    static const size_t Rows = 4;

    float x;
    float y;
    float z;
    float w;

    Float4() = default;

    Float4(float t) : x{t}, y{t}, z{t}, w{t} {
    }

    Float4(float x, float y, float z, float w) : x{x}, y{y}, z{z}, w{w} {
    }

    Float4(const Float2& v, float z, float w) : x{v.x}, y{v.y}, z{z}, w{w} {
    }

    Float4(const Float3& v, float w) : x{v.x}, y{v.y}, z{v.z}, w{w} {
    }

    Float2& asFloat2() {
        return reinterpret_cast<Float2&>(*this);
    }

    const Float2& asFloat2() const {
        return reinterpret_cast<const Float2&>(*this);
    }

    // FIXME: make cast operator?
    Float3& asFloat3() {
        return reinterpret_cast<Float3&>(*this);
    }

    const Float3& asFloat3() const {
        return reinterpret_cast<const Float3&>(*this);
    }

    Quaternion& asQuaternion();
    const Quaternion& asQuaternion() const;

    T& operator[](size_t i) {
        PLY_ASSERT(i < Rows);
        return reinterpret_cast<float*>(this)[i];
    }

    const T& operator[](size_t i) const {
        PLY_ASSERT(i < Rows);
        return reinterpret_cast<const float*>(this)[i];
    }

    template <typename OtherVec4>
    OtherVec4 to() const {
        return {typename OtherVec4::T(x), typename OtherVec4::T(y), typename OtherVec4::T(z),
                typename OtherVec4::T(w)};
    }

    float& r() {
        return x;
    }

    float r() const {
        return x;
    }

    float& g() {
        return y;
    }

    float g() const {
        return y;
    }

    float& b() {
        return z;
    }

    float b() const {
        return z;
    }

    float& a() {
        return w;
    }

    float a() const {
        return w;
    }

    void operator=(const Float4& arg) {
        x = arg.x;
        y = arg.y;
        z = arg.z;
        w = arg.w;
    }

    bool operator==(const Float4& arg) const {
        return (x == arg.x) && (y == arg.y) && (z == arg.z) && (w == arg.w);
    }

    bool operator!=(const Float4& arg) const {
        return !(*this == arg);
    }

    bool operator<(const Float4& arg) const {
        if (x == arg.x) {
            if (y == arg.y) {
                if (z == arg.z)
                    return w < arg.w;
                else
                    return z < arg.z;
            } else
                return y < arg.y;
        } else
            return x < arg.x;
    }

    Float4 operator-() const {
        return {-x, -y, -z, -w};
    }

    Float4 operator+(const Float4& arg) const {
        return {x + arg.x, y + arg.y, z + arg.z, w + arg.w};
    }

    // FIXME: Implement all variants of this:
    Float4 operator+(float arg) const {
        return {x + arg, y + arg, z + arg, w + arg};
    }

    friend Float4 operator+(float a, const Float4& b) {
        return {a + b.x, a + b.y, a + b.z, a + b.w};
    }

    void operator+=(const Float4& arg) {
        x += arg.x;
        y += arg.y;
        z += arg.z;
        w += arg.w;
    }

    Float4 operator-(const Float4& arg) const {
        return {x - arg.x, y - arg.y, z - arg.z, w - arg.w};
    }

    Float4 operator-(float arg) const {
        return {x - arg, y - arg, z - arg, w - arg};
    }

    friend Float4 operator-(float a, const Float4& b) {
        return {a - b.x, a - b.y, a - b.z, a - b.w};
    }

    void operator-=(const Float4& arg) {
        x -= arg.x;
        y -= arg.y;
        z -= arg.z;
        w -= arg.w;
    }

    Float4 operator*(float arg) const {
        return {x * arg, y * arg, z * arg, w * arg};
    }

    void operator*=(float arg) {
        x *= arg;
        y *= arg;
        z *= arg;
        w *= arg;
    }

    Float4 operator*(const Float4& arg) const {
        return {x * arg.x, y * arg.y, z * arg.z, w * arg.w};
    }

    void operator*=(const Float4& arg) {
        x *= arg.x;
        y *= arg.y;
        z *= arg.z;
        w *= arg.w;
    }

    Float4 operator/(float arg) const {
        return *this * (1.f / arg);
    }

    void operator/=(float arg) {
        *this *= (1.f / arg);
    }

    Float4 operator/(const Float4& arg) const {
        return {x / arg.x, y / arg.y, z / arg.z, w / arg.w};
    }

    void operator/=(const Float4& arg) {
        x /= arg.x;
        y /= arg.y;
        z /= arg.z;
        w /= arg.w;
    }

    float length2() const {
        return x * x + y * y + z * z + w * w;
    }

    float length() const {
        return sqrtf(length2());
    }

    Float4 normalized() const {
        return *this / length();
    }

    template <typename Hasher>
    void appendTo(Hasher& hasher) const {
        for (u32 i = 0; i < 4; i++) {
            hasher.append(((const float*) this)[i]);
        }
    }
};

inline float dot(const Float4& a, const Float4& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline Float4 clamp(const Float4& v, const Float4& mins, const Float4& maxs) {
    return {clamp(v.x, mins.x, maxs.x), clamp(v.y, mins.y, maxs.y), clamp(v.z, mins.z, maxs.z),
            clamp(v.w, mins.w, maxs.w)};
}

inline Float4 clamp(const Float4& v, float lo, float hi) {
    return {clamp(v.x, lo, hi), clamp(v.y, lo, hi), clamp(v.z, lo, hi), clamp(v.w, lo, hi)};
}

inline Float4 abs(const Float4& a) {
    return {fabsf(a.x), fabsf(a.y), fabsf(a.z), fabsf(a.w)};
}

inline Float4 pow(const Float4& a, const Float4& b) {
    return {powf(a.x, b.x), powf(a.y, b.y), powf(a.z, b.z), powf(a.w, b.w)};
}

inline Float4 min(const Float4& a, float b) {
    return {min(a.x, b), min(a.y, b), min(a.z, b), min(a.w, b)};
}

inline Float4 max(const Float4& a, float b) {
    return {max(a.x, b), max(a.y, b), max(a.z, b), max(a.w, b)};
}

inline Float4 min(const Float4& a, const Float4& b) {
    return {min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w)};
}

inline Float4 max(const Float4& a, const Float4& b) {
    return {max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w)};
}

inline bool allLessThan(const Float4& a, const Float4& b) {
    return (a.x < b.x) && (a.y < b.y) && (a.z < b.z) && (a.w < b.w);
}

inline bool allLessThanOrEqual(const Float4& a, const Float4& b) {
    return (a.x <= b.x) && (a.y <= b.y) && (a.z <= b.z) && (a.w <= b.w);
}

typedef Box<Float2> Rect;
typedef Box<Float3> Box3D;

} // namespace ply
