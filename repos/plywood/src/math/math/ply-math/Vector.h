/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-math/Core.h>
#include <ply-math/Box.h>

namespace ply {

struct Quaternion;
struct Float2;
struct Float3;
struct Float4;

//------------------------------------------------------------------------------------------------
/*!
A 2D vector with floating-point components `x` and `y`.
*/
struct Float2 {
    /*!
    \beginGroup
    The components.
    */
    float x;
    float y;
    /*!
    \endGroup
    */

    /*!
    Constructs an uninitialized 2D vector.
    */
    PLY_INLINE Float2() = default;
    /*!
    Constructs a 2D vector with both components set to `t`.

        Float2 v = {1};
        StdOut::text() << v;  // prints "{1, 1}"
    */
    PLY_INLINE Float2(float t) : x{t}, y{t} {
    }
    /*!
    Constructs a 2D vector from the given components.

        Float2 v = {1, 0};
    */
    PLY_INLINE Float2(float x, float y) : x{x}, y{y} {
    }
    /*!
    Converts to another 2D vector type such as `IntVec2` or `Int2<s16>`.

        Float2 a = {4, 5};
        IntVec2 b = a.to<IntVec2>();
    */
    template <typename OtherVec2>
    PLY_INLINE OtherVec2 to() const {
        using T = decltype(OtherVec2::x);
        PLY_STATIC_ASSERT(sizeof(OtherVec2) == sizeof(T) * 2);
        return {(T) x, (T) y};
    }
    /*!
    Copy assignment operator.
    */
    PLY_INLINE void operator=(const Float2& arg) {
        x = arg.x;
        y = arg.y;
    }
    /*!
    \beginGroup
    Returns `true` if the 2D vectors are equal (or not equal) using floating-point comparison. In
    particular, `Float2{0} == Float2{-0}` is `true`.
    */
    PLY_INLINE bool operator==(const Float2& arg) const {
        return (x == arg.x) && (y == arg.y);
    }
    PLY_INLINE bool operator!=(const Float2& arg) const {
        return !(*this == arg);
    }
    /*!
    \endGroup
    */
    /*!
    Unary negation.
    */
    PLY_INLINE Float2 operator-() const {
        return {-x, -y};
    }
    /*!
    \beginGroup
    Addition operators. A scalar operand `s` is equivalent to `Float2{s}`.
    */
    PLY_INLINE Float2 operator+(const Float2& arg) const {
        return {x + arg.x, y + arg.y};
    }
    PLY_INLINE friend Float2 operator+(float a, const Float2& b) {
        return {a + b.x, a + b.y};
    }
    /*!
    \endGroup
    */
    /*!
    In-place addition.
    */
    PLY_INLINE void operator+=(const Float2& arg) {
        x += arg.x;
        y += arg.y;
    }
    /*!
    \beginGroup
    Subtraction operators. A scalar operand `s` is equivalent to `Float2{s}`.
    */
    PLY_INLINE Float2 operator-(const Float2& arg) const {
        return {x - arg.x, y - arg.y};
    }
    PLY_INLINE friend Float2 operator-(float a, const Float2& b) {
        return {a - b.x, a - b.y};
    }
    /*!
    \endGroup
    */
    /*!
    In-place subtraction.
    */
    PLY_INLINE void operator-=(const Float2& arg) {
        x -= arg.x;
        y -= arg.y;
    }
    /*!
    \beginGroup
    Component-wise multiplication. A scalar operand `s` is equivalent to `Float2{s}`.
    */
    PLY_INLINE Float2 operator*(const Float2& arg) const {
        return {x * arg.x, y * arg.y};
    }
    PLY_INLINE friend Float2 operator*(float a, const Float2& b) {
        return {a * b.x, a * b.y};
    }
    /*!
    \endGroup
    */
    /*!
    \beginGroup
    In-place multiplication.
    */
    PLY_INLINE void operator*=(const Float2& arg) {
        x *= arg.x;
        y *= arg.y;
    }
    /*!
    \endGroup
    */
    /*!
    \beginGroup
    Component-wise division. A scalar operand `s` is equivalent to `Float2{s}`.
    */
    PLY_INLINE Float2 operator/(const Float2& arg) const {
        return {x / arg.x, y / arg.y};
    }
    PLY_INLINE Float2 operator/(float arg) const {
        return *this * (1.f / arg);
    }
    PLY_INLINE friend Float2 operator/(float a, const Float2& b) {
        return {a / b.x, a / b.y};
    }
    /*!
    \endGroup
    */
    /*!
    \beginGroup
    In-place division.
    */
    PLY_INLINE void operator/=(float arg) {
        *this *= (1.f / arg);
    }
    PLY_INLINE void operator/=(const Float2& arg) {
        x /= arg.x;
        y /= arg.y;
    }
    /*!
    \endGroup
    */
    /*!
    Returns the square of the length of the 2D vector. Equivalent to `dot(this, this)`.
    */
    PLY_INLINE float length2() const {
        return x * x + y * y;
    }
    /*!
    Returns the length of the 2D vector. Equivalent to `sqrtf(this.length2())`.
    */
    PLY_INLINE float length() const {
        return sqrtf(length2());
    }
    /*!
    Returns `true` if the squared length of the vector is sufficiently close to 1.0. The threshold
    is given by `thresh`.
    */
    PLY_INLINE bool isUnit(float thresh = 0.001f) const {
        return fabsf(1.f - length2()) < thresh;
    }
    /*!
    Returns a unit-length 2D vector having the same direction as `this`. No safety check is
    performed.
    */
    PLY_NO_DISCARD Float2 normalized() const;
    /*!
    Returns a unit-length 2D vector having the same direction as `this` with safety checks.
    */
    PLY_NO_DISCARD Float2 safeNormalized(const Float2& fallback = {1, 0},
                                         float epsilon = 1e-20f) const;
    /*!
    \beginGroup
    Swizzle functions.

        Float2 v = {4, 5};
        StdOut::text() << v.swizzle(1, 0);        // prints "{5, 4}"
        StdOut::text() << v.swizzle(1, 0, 1);     // prints "{5, 4, 5}"
        StdOut::text() << v.swizzle(1, 1, 1, 0);  // prints "{5, 5, 5, 4}"
    */
    PLY_INLINE PLY_NO_DISCARD Float2 swizzle(u32 i0, u32 i1) const;
    PLY_INLINE PLY_NO_DISCARD Float3 swizzle(u32 i0, u32 i1, u32 i2) const;
    PLY_INLINE PLY_NO_DISCARD Float4 swizzle(u32 i0, u32 i1, u32 i2, u32 i3) const;
    /*!
    \endGroup
    */
    template <typename Hasher>
    void appendTo(Hasher& hasher) const {
        for (u32 i = 0; i < 2; i++) {
            hasher.append(((const float*) this)[i]);
        }
    }
};

PLY_INLINE float dot(const Float2& a, const Float2& b) {
    return a.x * b.x + a.y * b.y;
}

PLY_INLINE float cross(const Float2& a, const Float2& b) {
    return a.x * b.y - a.y * b.x;
}

PLY_INLINE Float2 clamp(const Float2& v, const Float2& mins, const Float2& maxs) {
    return {clamp(v.x, mins.x, maxs.x), clamp(v.y, mins.y, maxs.y)};
}

PLY_INLINE Float2 abs(const Float2& a) {
    return {fabsf(a.x), fabsf(a.y)};
}

PLY_INLINE Float2 pow(const Float2& a, const Float2& b) {
    return {powf(a.x, b.x), powf(a.y, b.y)};
}

PLY_INLINE Float2 min(const Float2& a, const Float2& b) {
    return {min(a.x, b.x), min(a.y, b.y)};
}

PLY_INLINE Float2 max(const Float2& a, const Float2& b) {
    return {max(a.x, b.x), max(a.y, b.y)};
}

PLY_INLINE bool allLess(const Float2& a, const Float2& b) {
    return (a.x < b.x) && (a.y < b.y);
}

PLY_INLINE bool allLessOrEqual(const Float2& a, const Float2& b) {
    return (a.x <= b.x) && (a.y <= b.y);
}

PLY_INLINE Float2 quantizeUp(const Float2& value, float spacing) {
    return {quantizeUp(value.x, spacing), quantizeUp(value.y, spacing)};
}

PLY_INLINE Float2 quantizeDown(const Float2& value, float spacing) {
    return {quantizeDown(value.x, spacing), quantizeDown(value.y, spacing)};
}

PLY_INLINE Float2 quantizeNearest(const Float2& value, float spacing) {
    // Good to let the compiler see the spacing so it can optimize the divide by constant
    return {quantizeNearest(value.x, spacing), quantizeNearest(value.y, spacing)};
}

PLY_INLINE bool isQuantized(const Float2& value, float spacing) {
    return quantizeNearest(value, spacing) == value;
}

//------------------------------------------------------------------------------------------------
/*!
A 3D vector with floating-point components `x`, `y` and `z`.
*/
struct Float3 {
    /*!
    \beginGroup
    The components.
    */
    float x;
    float y;
    float z;
    /*!
    \endGroup
    */

    /*!
    Constructs an uninitialized 3D vector.
    */
    PLY_INLINE Float3() = default;
    /*!
    Constructs a 3D vector with all components set to `t`.

        Float3 v = {1};
        StdOut::text() << v;  // prints "{1, 1, 1}"
    */
    PLY_INLINE Float3(float t) : x{t}, y{t}, z{t} {
    }
    /*!
    Constructs a 3D vector from the given components.

        Float3 v = {1, 0, 0};
    */
    PLY_INLINE Float3(float x, float y, float z) : x{x}, y{y}, z{z} {
    }
    /*!
    Constructs a 3D vector from a 2D vector and a third component.

        Float2 a = {1, 2};
        StdOut::text() << Float3{a, 0};  // prints "{1, 2, 0}"
    */
    PLY_INLINE Float3(const Float2& v, float z) : x{v.x}, y{v.y}, z{z} {
    }
    /*!
    Returns a const reference to the first two components as a `Float2` using type punning. This
    should only be used as a temporary expression.

        Float3 v = {4, 5, 6};
        StdOut::text() << v.asFloat2();  // prints "{4, 5}"
    */
    PLY_INLINE const Float2& asFloat2() const {
        PLY_PUN_SCOPE
        return reinterpret_cast<const Float2&>(*this);
    }
    /*!
    Converts to another 3D vector type such as `IntVec3` or `Int3<s16>`.

        Float3 a = {4, 5, 6};
        IntVec3 b = a.to<IntVec3>();
    */
    template <typename OtherVec3>
    PLY_INLINE OtherVec3 to() const {
        using T = decltype(OtherVec3::x);
        PLY_STATIC_ASSERT(sizeof(OtherVec3) == sizeof(T) * 3);
        return {(T) x, (T) y, (T) z};
    }
    /*!
    \beginGroup
    Aliases for `x`, `y` and `z`.

        Float3 c = {1.0f, 0.8f, 0.7f};
        StdOut::text().format("{}, {}, {}", c.r(), c.g(), c.b());
        // prints "{1.0, 0.8, 0.7}"
    */
    PLY_INLINE float& r() {
        return x;
    }
    PLY_INLINE float r() const {
        return x;
    }
    PLY_INLINE float& g() {
        return y;
    }
    PLY_INLINE float g() const {
        return y;
    }
    PLY_INLINE float& b() {
        return z;
    }
    PLY_INLINE float b() const {
        return z;
    }
    /*!
    \endGroup
    */
    /*!
    Copy assignment operator.
    */
    PLY_INLINE void operator=(const Float3& arg) {
        x = arg.x;
        y = arg.y;
        z = arg.z;
    }
    /*!
    \beginGroup
    Returns `true` if the 3D vectors are equal (or not equal) using floating-point comparison. In
    particular, `Float3{0} == Float3{-0}` is `true`.
    */
    PLY_INLINE bool operator==(const Float3& arg) const {
        return (x == arg.x) && (y == arg.y) && (z == arg.z);
    }
    PLY_INLINE bool operator!=(const Float3& arg) const {
        return !(*this == arg);
    }
    /*!
    \endGroup
    */
    /*!
    Unary negation.
    */
    PLY_INLINE Float3 operator-() const {
        return {-x, -y, -z};
    }
    /*!
    \beginGroup
    Addition operators. A scalar operand `s` is equivalent to `Float3{s}`.
    */
    PLY_INLINE Float3 operator+(const Float3& v) const {
        return {x + v.x, y + v.y, z + v.z};
    }
    PLY_INLINE friend Float3 operator+(float s, const Float3& v) {
        return {s + v.x, s + v.y, s + v.z};
    }
    /*!
    \endGroup
    */
    /*!
    In-place addition.
    */
    PLY_INLINE void operator+=(const Float3& arg) {
        x += arg.x;
        y += arg.y;
        z += arg.z;
    }
    /*!
    \beginGroup
    Subtraction operators. A scalar operand `s` is equivalent to `Float3{s}`.
    */
    PLY_INLINE Float3 operator-(const Float3& v) const {
        return {x - v.x, y - v.y, z - v.z};
    }
    PLY_INLINE friend Float3 operator-(float s, const Float3& v) {
        return {s - v.x, s - v.y, s - v.z};
    }
    /*!
    \endGroup
    */
    /*!
    In-place subtraction.
    */
    PLY_INLINE void operator-=(const Float3& arg) {
        x -= arg.x;
        y -= arg.y;
        z -= arg.z;
    }
    /*!
    \beginGroup
    Component-wise multiplication. A scalar operand `s` is equivalent to `Float3{s}`.
    */
    PLY_INLINE Float3 operator*(const Float3& arg) const {
        return {x * arg.x, y * arg.y, z * arg.z};
    }
    PLY_INLINE friend Float3 operator*(float s, const Float3& v) {
        return {s * v.x, s * v.y, s * v.z};
    }
    /*!
    \endGroup
    */
    /*!
    \beginGroup
    In-place multiplication.
    */
    PLY_INLINE void operator*=(const Float3& arg) {
        x *= arg.x;
        y *= arg.y;
        z *= arg.z;
    }
    /*!
    \endGroup
    */
    /*!
    \beginGroup
    Component-wise division. A scalar operand `s` is equivalent to `Float3{s}`.
    */
    PLY_INLINE Float3 operator/(const Float3& arg) const {
        return {x / arg.x, y / arg.y, z / arg.z};
    }
    PLY_INLINE Float3 operator/(float arg) const {
        return *this * (1.f / arg);
    }
    PLY_INLINE friend Float3 operator/(float s, const Float3& v) {
        return {s / v.x, s / v.y, s / v.z};
    }
    /*!
    \endGroup
    */
    /*!
    \beginGroup
    In-place division.
    */
    PLY_INLINE void operator/=(float arg) {
        *this *= (1.f / arg);
    }
    PLY_INLINE void operator/=(const Float3& arg) {
        x /= arg.x;
        y /= arg.y;
        z /= arg.z;
    }
    /*!
    \endGroup
    */
    /*!
    Returns the square of the length of the 3D vector. Equivalent to `dot(this, this)`.
    */
    PLY_INLINE float length2() const {
        return x * x + y * y + z * z;
    }
    /*!
    Returns the length of the 3D vector. Equivalent to `sqrtf(this.length2())`.
    */
    PLY_INLINE float length() const {
        return sqrtf(length2());
    }
    /*!
    Returns `true` if the squared length of the vector is sufficiently close to 1.0. The threshold
    is given by `thresh`.
    */
    PLY_INLINE bool isUnit(float thresh = 0.001f) const {
        return fabsf(1.f - length2()) < thresh;
    }
    /*!
    Returns a unit-length 3D vector having the same direction as `this`. No safety check is
    performed.
    */
    PLY_NO_DISCARD Float3 normalized() const;
    /*!
    Returns a unit-length 3D vector having the same direction as `this` with safety checks.
    */
    PLY_NO_DISCARD Float3 safeNormalized(const Float3& fallback = {1, 0, 0},
                                         float epsilon = 1e-20f) const;
    /*!
    \beginGroup
    Swizzle functions.

        Float3 v = {4, 5, 6};
        StdOut::text() << v.swizzle(1, 0);        // prints "{5, 4}"
        StdOut::text() << v.swizzle(2, 0, 1);     // prints "{6, 4, 5}"
        StdOut::text() << v.swizzle(2, 2, 2, 1);  // prints "{6, 6, 6, 5}"
    */
    PLY_INLINE PLY_NO_DISCARD Float2 swizzle(u32 i0, u32 i1) const;
    PLY_INLINE PLY_NO_DISCARD Float3 swizzle(u32 i0, u32 i1, u32 i2) const;
    PLY_INLINE PLY_NO_DISCARD Float4 swizzle(u32 i0, u32 i1, u32 i2, u32 i3) const;
    /*!
    \endGroup
    */
    template <typename Hasher>
    PLY_INLINE void appendTo(Hasher& hasher) const {
        PLY_PUN_SCOPE {
            auto* v = (const float*) this;
            for (u32 i = 0; i < 3; i++) {
                hasher.append(v[i]);
            }
        }
    }
};

PLY_INLINE float dot(const Float3& a, const Float3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Float3 cross(const Float3& a, const Float3& b);

Float3 clamp(const Float3& v, const Float3& mins, const Float3& maxs);

PLY_INLINE Float3 abs(const Float3& a) {
    return {fabsf(a.x), fabsf(a.y), fabsf(a.z)};
}

Float3 pow(const Float3& a, const Float3& b);

PLY_INLINE Float3 min(const Float3& a, float b) {
    return {min(a.x, b), min(a.y, b), min(a.z, b)};
}

PLY_INLINE Float3 max(const Float3& a, float b) {
    return {max(a.x, b), max(a.y, b), max(a.z, b)};
}

PLY_INLINE Float3 min(const Float3& a, const Float3& b) {
    return {min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)};
}

PLY_INLINE Float3 max(const Float3& a, const Float3& b) {
    return {max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)};
}

PLY_INLINE bool allLess(const Float3& a, const Float3& b) {
    return (a.x < b.x) && (a.y < b.y) && (a.z < b.z);
}

PLY_INLINE bool allLessOrEqual(const Float3& a, const Float3& b) {
    return (a.x <= b.x) && (a.y <= b.y) && (a.z <= b.z);
}

PLY_INLINE Float3 quantizeUp(const Float3& value, float spacing) {
    return {quantizeUp(value.x, spacing), quantizeUp(value.y, spacing),
            quantizeUp(value.z, spacing)};
}

PLY_INLINE Float3 quantizeDown(const Float3& value, float spacing) {
    return {quantizeDown(value.x, spacing), quantizeDown(value.y, spacing),
            quantizeDown(value.z, spacing)};
}

PLY_INLINE Float3 quantizeNearest(const Float3& value, float spacing) {
    return {quantizeNearest(value.x, spacing), quantizeNearest(value.y, spacing),
            quantizeNearest(value.z, spacing)};
}

PLY_INLINE bool isQuantized(const Float3& value, float spacing) {
    return quantizeNearest(value, spacing) == value;
}


//------------------------------------------------------------------------------------------------
/*!
A 4D vector with floating-point components `x`, `y`, `z` and `w`.

`w` is the fourth component. It comes after `z` sequentially in memory.
*/
struct Float4 {
    /*!
    \beginGroup
    The components.
    */
    float x;
    float y;
    float z;
    float w;
    /*!
    \endGroup
    */

    /*!
    Constructs an uninitialized 4D vector.
    */
    PLY_INLINE Float4() = default;
    /*!
    Constructs a 4D vector with all components set to `t`.

        Float4 v = {1};
        StdOut::text() << v;  // prints "{1, 1, 1, 1}"
    */
    PLY_INLINE Float4(float t) : x{t}, y{t}, z{t}, w{t} {
    }
    /*!
    Constructs a 4D vector from the given components.

        Float4 v = {1, 0, 0, 0};
    */
    PLY_INLINE Float4(float x, float y, float z, float w) : x{x}, y{y}, z{z}, w{w} {
    }
    /*!
    Constructs a 4D vector from a 3D vector and a fourth component.

        Float3 a = {1, 2, 3};
        StdOut::text() << Float4{a, 0};  // prints "{1, 2, 3, 0}"
    */
    PLY_INLINE Float4(const Float3& v, float w) : x{v.x}, y{v.y}, z{v.z}, w{w} {
    }
    /*!
    Constructs a 4D vector from a 2D vector and two additional components.

        Float2 a = {1, 2};
        StdOut::text() << Float4{a, 0, 0};  // prints "{1, 2, 0, 0}"
    */
    PLY_INLINE Float4(const Float2& v, float z, float w) : x{v.x}, y{v.y}, z{z}, w{w} {
    }
    /*!
    Returns a const reference to the first two components as a `Float2` using type punning. This
    should only be used as a temporary expression.

        Float4 v = {4, 5, 6, 7};
        StdOut::text() << v.asFloat2();  // prints "{4, 5}"
    */
    PLY_INLINE const Float2& asFloat2() const {
        PLY_PUN_SCOPE
        return reinterpret_cast<const Float2&>(*this);
    }
    /*!
    Returns a const reference to the first three components as a `Float3` using type punning. This
    should only be used as a temporary expression.

        Float4 v = {4, 5, 6, 7};
        StdOut::text() << v.asFloat3();  // prints "{4, 5, 6}"
    */
    PLY_INLINE const Float3& asFloat3() const {
        PLY_PUN_SCOPE
        return reinterpret_cast<const Float3&>(*this);
    }
    /*!
    Casts the 4D vector to a `Quaternion` using type punning. This should only be used as a
    temporary expression.
    */
    PLY_INLINE const Quaternion& asQuaternion() const;
    /*!
    Converts to another 4D vector type such as `IntVec4` or `Int4<s16>`.

        Float4 a = {4, 5, 6, 7};
        IntVec4 b = a.to<IntVec4>();
    */
    template <typename OtherVec4>
    OtherVec4 to() const {
        using T = decltype(OtherVec4::x);
        PLY_STATIC_ASSERT(sizeof(OtherVec4) == sizeof(T) * 4);
        return {(T) x, (T) y, (T) z, (T) w};
    }
    /*!
    \beginGroup
    Aliases for `x`, `y`, `z` and `w`.

        Float4 c = {1.0f, 0.8f, 0.7f, 0.5f};
        StdOut::text().format("{}, {}, {}, {}", c.r(), c.g(), c.b(), c.a());
        // prints "{1.0, 0.8, 0.7, 0.5}"
    */
    PLY_INLINE float& r() {
        return x;
    }
    PLY_INLINE float r() const {
        return x;
    }
    PLY_INLINE float& g() {
        return y;
    }
    PLY_INLINE float g() const {
        return y;
    }
    PLY_INLINE float& b() {
        return z;
    }
    PLY_INLINE float b() const {
        return z;
    }
    PLY_INLINE float& a() {
        return w;
    }
    PLY_INLINE float a() const {
        return w;
    }
    /*!
    \endGroup
    */
    /*!
    Copy assignment operator.
    */
    PLY_INLINE void operator=(const Float4& arg) {
        x = arg.x;
        y = arg.y;
        z = arg.z;
        w = arg.w;
    }
    /*!
    \beginGroup
    Returns `true` if the 4D vectors are equal (or not equal) using floating-point comparison. In
    particular, `Float4{0} == Float4{-0}` is `true`.
    */
    PLY_INLINE bool operator==(const Float4& arg) const {
        return (x == arg.x) && (y == arg.y) && (z == arg.z) && (w == arg.w);
    }
    PLY_INLINE bool operator!=(const Float4& arg) const {
        return !(*this == arg);
    }
    /*!
    \endGroup
    */
    /*!
    Unary negation.
    */
    Float4 operator-() const {
        return {-x, -y, -z, -w};
    }
    /*!
    \beginGroup
    Addition operators. A scalar operand `s` is equivalent to `Float4{s}`.
    */
    Float4 operator+(const Float4& arg) const {
        return {x + arg.x, y + arg.y, z + arg.z, w + arg.w};
    }
    PLY_INLINE friend Float4 operator+(float s, const Float4& v) {
        return {s + v.x, s + v.y, s + v.z, s + v.w};
    }
    /*!
    \endGroup
    */
    /*!
    In-place addition.
    */
    void operator+=(const Float4& arg) {
        x += arg.x;
        y += arg.y;
        z += arg.z;
        w += arg.w;
    }
    /*!
    \beginGroup
    Subtraction operators. A scalar operand `s` is equivalent to `Float4{s}`.
    */
    Float4 operator-(const Float4& arg) const {
        return {x - arg.x, y - arg.y, z - arg.z, w - arg.w};
    }
    friend Float4 operator-(float a, const Float4& b) {
        return {a - b.x, a - b.y, a - b.z, a - b.w};
    }
    /*!
    \endGroup
    */
    /*!
    In-place subtraction.
    */
    void operator-=(const Float4& arg) {
        x -= arg.x;
        y -= arg.y;
        z -= arg.z;
        w -= arg.w;
    }
    /*!
    \beginGroup
    Component-wise multiplication. A scalar operand `s` is equivalent to `Float4{s}`.
    */
    Float4 operator*(const Float4& arg) const {
        return {x * arg.x, y * arg.y, z * arg.z, w * arg.w};
    }
    PLY_INLINE friend Float4 operator*(float s, const Float4& v) {
        return {s * v.x, s * v.y, s * v.z, s * v.w};
    }
    /*!
    \endGroup
    */
    /*!
    \beginGroup
    In-place multiplication.
    */
    void operator*=(const Float4& arg) {
        x *= arg.x;
        y *= arg.y;
        z *= arg.z;
        w *= arg.w;
    }
    /*!
    \endGroup
    */
    /*!
    \beginGroup
    Component-wise division. A scalar operand `s` is equivalent to `Float3{s}`.
    */
    Float4 operator/(const Float4& arg) const {
        return {x / arg.x, y / arg.y, z / arg.z, w / arg.w};
    }
    PLY_INLINE friend Float4 operator/(float s, const Float4& v) {
        return {s / v.x, s / v.y, s / v.z, s / v.w};
    }
    /*!
    \endGroup
    */
    /*!
    \beginGroup
    In-place division.
    */
    void operator/=(float arg) {
        *this *= (1.f / arg);
    }
    void operator/=(const Float4& arg) {
        x /= arg.x;
        y /= arg.y;
        z /= arg.z;
        w /= arg.w;
    }
    /*!
    \endGroup
    */
    /*!
    Returns the square of the length of the 3D vector. Equivalent to `dot(this, this)`.
    */
    float length2() const {
        return x * x + y * y + z * z + w * w;
    }
    /*!
    Returns the length of the 3D vector. Equivalent to `sqrtf(this.length2())`.
    */
    float length() const {
        return sqrtf(length2());
    }
    /*!
    Returns `true` if the squared length of the vector is sufficiently close to 1.0. The threshold
    is given by `thresh`.
    */
    PLY_INLINE bool isUnit(float thresh = 0.001f) const {
        return fabsf(1.f - length2()) < thresh;
    }
    /*!
    Returns a unit-length 3D vector having the same direction as `this`. No safety check is
    performed.
    */
    PLY_NO_DISCARD Float4 normalized() const;
    /*!
    Returns a unit-length 3D vector having the same direction as `this` with safety checks.
    */
    PLY_NO_DISCARD Float4 safeNormalized(const Float4& fallback = {1, 0, 0, 0},
                                         float epsilon = 1e-20f) const;
    /*!
    \beginGroup
    Swizzle functions.

        Float3 v = {4, 5, 6};
        StdOut::text() << v.swizzle(1, 0);        // prints "{5, 4}"
        StdOut::text() << v.swizzle(2, 0, 1);     // prints "{6, 4, 5}"
        StdOut::text() << v.swizzle(2, 2, 2, 1);  // prints "{6, 6, 6, 5}"
    */
    PLY_INLINE PLY_NO_DISCARD Float2 swizzle(u32 i0, u32 i1) const;
    PLY_INLINE PLY_NO_DISCARD Float3 swizzle(u32 i0, u32 i1, u32 i2) const;
    PLY_INLINE PLY_NO_DISCARD Float4 swizzle(u32 i0, u32 i1, u32 i2, u32 i3) const;
    /*!
    \endGroup
    */
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

Float4 pow(const Float4& a, const Float4& b);

inline Float4 min(const Float4& a, const Float4& b) {
    return {min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w)};
}

inline Float4 max(const Float4& a, const Float4& b) {
    return {max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w)};
}

inline bool allLess(const Float4& a, const Float4& b) {
    return (a.x < b.x) && (a.y < b.y) && (a.z < b.z) && (a.w < b.w);
}

inline bool allLessOrEqual(const Float4& a, const Float4& b) {
    return (a.x <= b.x) && (a.y <= b.y) && (a.z <= b.z) && (a.w <= b.w);
}

PLY_INLINE Float4 quantizeUp(const Float4& value, float spacing) {
    return {quantizeUp(value.x, spacing), quantizeUp(value.y, spacing),
            quantizeUp(value.z, spacing), quantizeUp(value.w, spacing)};
}

PLY_INLINE Float4 quantizeDown(const Float4& value, float spacing) {
    return {quantizeDown(value.x, spacing), quantizeDown(value.y, spacing),
            quantizeDown(value.z, spacing), quantizeDown(value.w, spacing)};
}

PLY_INLINE Float4 quantizeNearest(const Float4& value, float spacing) {
    return {quantizeNearest(value.x, spacing), quantizeNearest(value.y, spacing),
            quantizeNearest(value.z, spacing), quantizeNearest(value.w, spacing)};
}

PLY_INLINE bool isQuantized(const Float4& value, float spacing) {
    return quantizeNearest(value, spacing) == value;
}

//---------------------------------

typedef Box<Float2> Rect;
typedef Box<Float3> Box3D;

//---------------------------------

PLY_INLINE PLY_NO_DISCARD Float2 Float2::swizzle(u32 i0, u32 i1) const {
    PLY_PUN_SCOPE
    auto* v = (const float*) this;
    PLY_ASSERT(i0 < 2 && i1 < 2);
    return {v[i0], v[i1]};
}

PLY_INLINE PLY_NO_DISCARD Float3 Float2::swizzle(u32 i0, u32 i1, u32 i2) const {
    PLY_PUN_SCOPE
    auto* v = (const float*) this;
    PLY_ASSERT(i0 < 2 && i1 < 2 && i2 < 2);
    return {v[i0], v[i1], v[i2]};
}

PLY_INLINE PLY_NO_DISCARD Float4 Float2::swizzle(u32 i0, u32 i1, u32 i2, u32 i3) const {
    PLY_PUN_SCOPE
    auto* v = (const float*) this;
    PLY_ASSERT(i0 < 2 && i1 < 2 && i2 < 2 && i3 < 2);
    return {v[i0], v[i1], v[i2], v[i3]};
}

PLY_INLINE PLY_NO_DISCARD Float2 Float3::swizzle(u32 i0, u32 i1) const {
    PLY_PUN_SCOPE
    auto* v = (const float*) this;
    PLY_ASSERT(i0 < 3 && i1 < 3);
    return {v[i0], v[i1]};
}

PLY_INLINE PLY_NO_DISCARD Float3 Float3::swizzle(u32 i0, u32 i1, u32 i2) const {
    PLY_PUN_SCOPE
    auto* v = (const float*) this;
    PLY_ASSERT(i0 < 3 && i1 < 3 && i2 < 3);
    return {v[i0], v[i1], v[i2]};
}

PLY_INLINE PLY_NO_DISCARD Float4 Float3::swizzle(u32 i0, u32 i1, u32 i2, u32 i3) const {
    PLY_PUN_SCOPE
    auto* v = (const float*) this;
    PLY_ASSERT(i0 < 3 && i1 < 3 && i2 < 3 && i2 < 3);
    return {v[i0], v[i1], v[i2], v[i3]};
}

PLY_INLINE PLY_NO_DISCARD Float2 Float4::swizzle(u32 i0, u32 i1) const {
    PLY_PUN_SCOPE
    auto* v = (const float*) this;
    PLY_ASSERT(i0 < 4 && i1 < 4);
    return {v[i0], v[i1]};
}

PLY_INLINE PLY_NO_DISCARD Float3 Float4::swizzle(u32 i0, u32 i1, u32 i2) const {
    PLY_PUN_SCOPE
    auto* v = (const float*) this;
    PLY_ASSERT(i0 < 4 && i1 < 4 && i2 < 4);
    return {v[i0], v[i1], v[i2]};
}

PLY_INLINE PLY_NO_DISCARD Float4 Float4::swizzle(u32 i0, u32 i1, u32 i2, u32 i3) const {
    PLY_PUN_SCOPE
    auto* v = (const float*) this;
    PLY_ASSERT(i0 < 4 && i1 < 4 && i2 < 4 && i2 < 4);
    return {v[i0], v[i1], v[i2], v[i3]};
}

} // namespace ply
