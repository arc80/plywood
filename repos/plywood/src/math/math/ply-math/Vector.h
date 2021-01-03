/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-math/Core.h>
#include <ply-math/Box.h>
#include <ply-math/BoolVector.h>

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
    \category Constructors
    Constructs an uninitialized 2D vector.
    */
    PLY_INLINE Float2() = default;
    /*!
    Constructs a 2D vector with both components set to `t`.

        Float2 v = {1};
        StdOut::text() << v;  // "{1, 1}"
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
    \category Assignment Operator
    Copy assignment. Declared with an [lvalue
    ref-qualifier](https://en.cppreference.com/w/cpp/language/member_functions#ref-qualified_member_functions)
    so that it's an error to assign to an rvalue, as in `a.normalized() = b`.
    */
    PLY_INLINE void operator=(const Float2& arg) & {
        x = arg.x;
        y = arg.y;
    }
    /*!
    \category Conversion Functions
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
    \category Length Functions
    Returns the square of the length of the 2D vector.
    */
    PLY_INLINE float length2() const {
        return x * x + y * y;
    }
    /*!
    Returns the length of the 2D vector. Equivalent to `sqrtf(this->length2())`.
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
    \category Interpreting as a Color
    \beginGroup
    Aliases for `x` and `y`.

        Float4 c = {1.0f, 0.8f};
        StdOut::text().format("{}, {}", c.r(), c.g());  // "1.0, 0.8"
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
    /*!
    \endGroup
    */
    /*!
    \category Swizzle Functions
    \beginGroup
    Swizzle functions.

        Float2 v = {4, 5};
        StdOut::text() << v.swizzle(1, 0);        // "{5, 4}"
        StdOut::text() << v.swizzle(1, 0, 1);     // "{5, 4, 5}"
        StdOut::text() << v.swizzle(1, 1, 1, 0);  // "{5, 5, 5, 4}"
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

/*!
\addToClass Float2
\category Arithmetic Operators
Unary negation.
*/
PLY_INLINE Float2 operator-(const Float2& a) {
    return {-a.x, -a.y};
}
/*!
\beginGroup
Returns a 2D vector whose components are the result of applying the given operation to the
corresponding components of `a` and `b`. Each component is acted on independently.

    StdOut::text() << Float2{2, 3} * Float2{4, 1};  // "{8, 3}"

If you specify a scalar value in place of a `Float2`, it will be promoted to a `Float2` by
replicating the value to each component.

    StdOut::text() << Float2{2, 3} * 2;  // "{4, 6}"
    StdOut::text() << 8 / Float2{2, 4};  // "{4, 2}"
*/
PLY_INLINE Float2 operator+(const Float2& a, const Float2& b) {
    return {a.x + b.x, a.y + b.y};
}
PLY_INLINE Float2 operator-(const Float2& a, const Float2& b) {
    return {a.x - b.x, a.y - b.y};
}
PLY_INLINE Float2 operator*(const Float2& a, const Float2& b) {
    return {a.x * b.x, a.y * b.y};
}
PLY_INLINE Float2 operator/(const Float2& a, const Float2& b) {
    return {a.x / b.x, a.y / b.y};
}
/*!
\endGroup
*/
PLY_INLINE Float2 operator/(const Float2& a, float b) {
    float oob = 1.f / b;
    return {a.x * oob, a.y * oob};
}
/*!
\beginGroup
In-place versions of the above operators.

    Float2 v = {2, 3};
    v *= {4, 1};
    StdOut::text() << v;  // "{8, 3}"
*/
PLY_INLINE void operator+=(Float2& a, const Float2& b) {
    a.x += b.x;
    a.y += b.y;
}
PLY_INLINE void operator-=(Float2& a, const Float2& b) {
    a.x -= b.x;
    a.y -= b.y;
}
PLY_INLINE void operator*=(Float2& a, const Float2& b) {
    a.x *= b.x;
    a.y *= b.y;
}
PLY_INLINE void operator/=(Float2& a, const Float2& b) {
    a.x /= b.x;
    a.y /= b.y;
}
/*!
\endGroup
*/
PLY_INLINE void operator/=(Float2& a, float b) {
    float oob = 1.f / b;
    a.x *= oob;
    a.y *= oob;
}
/*!
\category Geometric Functions
Returns the dot product of two 2D vectors.

    StdOut::text() << dot(Float2{1, 0}, Float2{3, 4});  // "2"
*/
PLY_INLINE float dot(const Float2& a, const Float2& b) {
    return a.x * b.x + a.y * b.y;
}
/*!
Returns the cross product of two 2D vectors.

    StdOut::text() << cross(Float2{1, 0}, Float2{3, 4});  // "4"
*/
PLY_INLINE float cross(const Float2& a, const Float2& b) {
    return a.x * b.y - a.y * b.x;
}
/*!
\category Component-wise Functions
Returns a copy of `v` with each component constrained to lie within the range determined by the
corresponding components of `mins` and `maxs`.

    Float2 v = {3, 1.5f};
    StdOut::text() << clamp(v, Float2{0, 1}, Float2{1, 2});  // "{1, 1.5}"
    StdOut::text() << clamp(v, 0, 1);                        // "{1, 1}"
*/
PLY_INLINE Float2 clamp(const Float2& v, const Float2& mins, const Float2& maxs) {
    return {clamp(v.x, mins.x, maxs.x), clamp(v.y, mins.y, maxs.y)};
}
/*!
Returns a 2D vector with each component set to the absolute value of the corresponding component of
`a`.

    StdOut::text() << abs(Float2{-2, 3});  // "{2, 3}"
*/
PLY_INLINE Float2 abs(const Float2& a) {
    return {fabsf(a.x), fabsf(a.y)};
}
/*!
Returns a 2D vector with each component set to the corresponding component of `a` raised to the
power of the corresponding component of `b`.

    StdOut::text() << pow(Float2{1, 2}, Float2{2, 3});  // "{1, 8}"
    StdOut::text() << pow(Float2{1, 2}, 2);             // "{1, 4}"
*/
PLY_INLINE Float2 pow(const Float2& a, const Float2& b) {
    return {powf(a.x, b.x), powf(a.y, b.y)};
}
/*!
Returns a 2D vector with each component set to minimum of the corresponding components of `a` and
`b`.

    StdOut::text() << min(Float2{0, 1}, Float2{1, 0});  // "{0, 0}"
*/
PLY_INLINE Float2 min(const Float2& a, const Float2& b) {
    return {min(a.x, b.x), min(a.y, b.y)};
}
/*!
Returns a 2D vector with each component set to maximum of the corresponding components of `a` and
`b`.

    StdOut::text() << max(Float2{0, 1}, Float2{1, 0});  // "{1, 1}"
*/
PLY_INLINE Float2 max(const Float2& a, const Float2& b) {
    return {max(a.x, b.x), max(a.y, b.y)};
}
/*!
\category Comparison Functions
\beginGroup
Returns `true` if the 2D vectors are equal (or not equal) using floating-point comparison. In
particular, `Float2{0.f} == Float2{-0.f}` is `true`.
*/
PLY_INLINE bool operator==(const Float2& a, const Float2& b) {
    return a.x == b.x && a.y == b.y;
}
PLY_INLINE bool operator!=(const Float2& a, const Float2& b) {
    return !(a == b);
}
/*!
\endGroup
*/
/*!
Returns `true` if `a` is approximately equal to `b`. The tolerance is given by `epsilon`.

    Float2 v = {0.9999f, 0.0001f};
    StdOut::text() << isNear(v, Float2{1, 0}, 1e-3f);  // "true"
*/
PLY_INLINE bool isNear(const Float2& a, const Float2& b, float epsilon) {
    return (b - a).length2() <= square(epsilon);
}
/*!
\beginGroup
These functions compare each component individually. The result of each comparison is returned in
a `Bool2`. Call `all()` to check if the result was `true` for all components, or call `any()` to
check if the result was `true` for any component.

    StdOut::text() << all(Float2{1, 2} > Float2{0, 1});  // "true"

These functions are useful for testing whether a point is inside a box. See the implementation of
`Box<>::contains` for an example.
*/
PLY_INLINE Bool2 operator<(const Float2& a, const Float2& b) {
    return {a.x < b.x, a.y < b.y};
}
PLY_INLINE Bool2 operator<=(const Float2& a, const Float2& b) {
    return {a.x <= b.x, a.y <= b.y};
}
PLY_INLINE Bool2 operator>(const Float2& a, const Float2& b) {
    return {a.x > b.x, a.y > b.y};
}
PLY_INLINE Bool2 operator>=(const Float2& a, const Float2& b) {
    return {a.x >= b.x, a.y >= b.y};
}
/*!
\endGroup
*/
/*!
\category Rounding Functions
\beginGroup
Returns a 2D vector with each component set to the rounded result of the corresponding component of
`vec`. The optional `spacing` argument can be used to round to arbitrary spacings. Most precise when
`spacing` is a power of 2.

    StdOut::text() << roundUp(Float2{-0.3f, 1.4f});   // "{0, 2}"
    StdOut::text() << roundDown(Float2{1.8f}, 0.5f);  // "{1.5, 1.5}"
*/
PLY_INLINE Float2 roundUp(const Float2& value, float spacing = 1) {
    return {roundUp(value.x, spacing), roundUp(value.y, spacing)};
}
PLY_INLINE Float2 roundDown(const Float2& value, float spacing = 1) {
    return {roundDown(value.x, spacing), roundDown(value.y, spacing)};
}
PLY_INLINE Float2 roundNearest(const Float2& value, float spacing = 1) {
    // Good to let the compiler see the spacing so it can optimize the divide by constant
    return {roundNearest(value.x, spacing), roundNearest(value.y, spacing)};
}
/*!
\endGroup
*/
/*!
Returns `true` if every component of `vec` is already rounded. The optional `spacing` argument can
be used to round to arbitrary spacings. Most precise when `spacing` is a power of 2.

    StdOut::text() << isRounded(Float2{1.5f, 2.5f}, 0.5f);  // "true"
*/
PLY_INLINE bool isRounded(const Float2& value, float spacing = 1) {
    return roundNearest(value, spacing) == value;
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
    \category Constructors
    Constructs an uninitialized 3D vector.
    */
    PLY_INLINE Float3() = default;
    /*!
    Constructs a 3D vector with all components set to `t`.

        Float3 v = {1};
        StdOut::text() << v;  // "{1, 1, 1}"
    */
    PLY_INLINE Float3(float t) : x{t}, y{t}, z{t} {
    }
    // Catch the case where 2 scalars are passed to constructor.
    // This would otherwise promote the first scalar to Float2:
    Float3(float, float) = delete;
    /*!
    Constructs a 3D vector from the given components.

        Float3 v = {1, 0, 0};
    */
    PLY_INLINE Float3(float x, float y, float z) : x{x}, y{y}, z{z} {
    }
    /*!
    Constructs a 3D vector from a 2D vector and a third component.

        Float2 a = {1, 2};
        StdOut::text() << Float3{a, 0};  // "{1, 2, 0}"
    */
    PLY_INLINE Float3(const Float2& v, float z) : x{v.x}, y{v.y}, z{z} {
    }
    /*!
    \category Assignment Operator
    Copy assignment. Declared with an [lvalue
    ref-qualifier](https://en.cppreference.com/w/cpp/language/member_functions#ref-qualified_member_functions)
    so that it's an error to assign to an rvalue, as in `a.normalized() = b`.
    */
    PLY_INLINE void operator=(const Float3& arg) & {
        x = arg.x;
        y = arg.y;
        z = arg.z;
    }
    /*!
    \category Conversion Functions
    Returns a const reference to the first two components as a `Float2` using type punning. This
    should only be used as a temporary expression.

        Float3 v = {4, 5, 6};
        StdOut::text() << v.asFloat2();  // "{4, 5}"
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
    \category Length Functions
    Returns the square of the length of the 3D vector.
    */
    PLY_INLINE float length2() const {
        return x * x + y * y + z * z;
    }
    /*!
    Returns the length of the 3D vector. Equivalent to `sqrtf(this->length2())`.
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
    \category Interpreting as a Color
    \beginGroup
    Aliases for `x`, `y` and `z`.

        Float3 c = {1.0f, 0.8f, 0.7f};
        StdOut::text().format("{}, {}, {}", c.r(), c.g(), c.b());  // "1.0, 0.8, 0.7"
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
    \category Swizzle Functions
    \beginGroup
    Swizzle functions.

        Float3 v = {4, 5, 6};
        StdOut::text() << v.swizzle(1, 0);        // "{5, 4}"
        StdOut::text() << v.swizzle(2, 0, 1);     // "{6, 4, 5}"
        StdOut::text() << v.swizzle(2, 2, 2, 1);  // "{6, 6, 6, 5}"
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

/*!
\addToClass Float3
\category Arithmetic Operators
Unary negation.
*/
PLY_INLINE Float3 operator-(const Float3& a) {
    return {-a.x, -a.y, -a.z};
}
/*!
\beginGroup
Returns a 3D vector whose components are the result of applying the given operation to the
corresponding components of `a` and `b`. Each component is acted on independently.

    StdOut::text() << Float3{2, 3, 2} * Float3{4, 1, 2};  // "{8, 3, 4}"

If you specify a scalar value in place of a `Float3`, it will be promoted to a `Float3` by
replicating the value to each component.

    StdOut::text() << Float3{2, 3, 2} * 2;  // "{4, 6, 4}"
    StdOut::text() << 8 / Float3{2, 4, 1};  // "{4, 2, 8}"
*/
PLY_INLINE Float3 operator+(const Float3& a, const Float3& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
PLY_INLINE Float3 operator-(const Float3& a, const Float3& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
PLY_INLINE Float3 operator*(const Float3& a, const Float3& b) {
    return {a.x * b.x, a.y * b.y, a.z * b.z};
}
PLY_INLINE Float3 operator/(const Float3& a, const Float3& b) {
    return {a.x / b.x, a.y / b.y, a.z / b.z};
}
/*!
\endGroup
*/
PLY_INLINE Float3 operator/(const Float3& a, float b) {
    float oob = 1.f / b;
    return {a.x * oob, a.y * oob, a.z * oob};
}
/*!
\beginGroup
In-place versions of the above operators.

    Float3 v = {2, 3, 2};
    v *= {4, 1, 2};
    StdOut::text() << v;  // "{8, 3, 4}"
*/
PLY_INLINE void operator+=(Float3& a, const Float3& b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
}
PLY_INLINE void operator-=(Float3& a, const Float3& b) {
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
}
PLY_INLINE void operator*=(Float3& a, const Float3& b) {
    a.x *= b.x;
    a.y *= b.y;
    a.z *= b.z;
}
PLY_INLINE void operator/=(Float3& a, const Float3& b) {
    a.x /= b.x;
    a.y /= b.y;
    a.z /= b.z;
}
/*!
\endGroup
*/
PLY_INLINE void operator/=(Float3& a, float b) {
    float oob = 1.f / b;
    a.x *= oob;
    a.y *= oob;
    a.z *= oob;
}
/*!
\category Geometric Functions
Returns the dot product of two 3D vectors.

    StdOut::text() << dot(Float3{2, 3, 1}, Float3{4, 5, 1});  // "24"
*/
PLY_INLINE float dot(const Float3& a, const Float3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
/*!
Returns the cross product of two 3D vectors.

    StdOut::text() << cross(Float3{1, 0, 0}, Float3{0, 1, 0});  // "{0, 0, 1}"
*/
Float3 cross(const Float3& a, const Float3& b);
/*!
\category Component-wise Functions
Returns a copy of `v` with each component constrained to lie within the range determined by the
corresponding components of `mins` and `maxs`.

    Float3 v = {3, 1.5f, 0};
    StdOut::text() << clamp(v, Float3{0, 1, 2}, Float3{1, 2, 3});  // "{1, 1.5, 2}"
    StdOut::text() << clamp(v, 0, 1);                              // "{1, 1, 0}"
*/
Float3 clamp(const Float3& v, const Float3& mins, const Float3& maxs);
/*!
Returns a 3D vector with each component set to the absolute value of the corresponding component of
`a`.

    StdOut::text() << abs(Float3{-2, 3, 0});  // "{2, 3, 0}"
*/
PLY_INLINE Float3 abs(const Float3& a) {
    return {fabsf(a.x), fabsf(a.y), fabsf(a.z)};
}
/*!
Returns a 3D vector with each component set to the corresponding component of `a` raised to the
power of the corresponding component of `b`.

    StdOut::text() << pow(Float3{1, 2, 2}, Float3{2, 3, 1});  // "{1, 8, 2}"
    StdOut::text() << pow(Float3{1, 2, 3}, 2);                // "{1, 4, 9}"
*/
Float3 pow(const Float3& a, const Float3& b);
/*!
Returns a 3D vector with each component set to minimum of the corresponding components of `a` and
`b`.

    StdOut::text() << min(Float3{0, 1, 0}, Float3{1, 0, 1});  // "{0, 0, 0}"
*/
PLY_INLINE Float3 min(const Float3& a, const Float3& b) {
    return {min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)};
}
/*!
Returns a 3D vector with each component set to maximum of the corresponding components of `a` and
`b`.

    StdOut::text() << max(Float3{0, 1, 0}, Float3{1, 0, 1});  // "{1, 1, 1}"
*/
PLY_INLINE Float3 max(const Float3& a, const Float3& b) {
    return {max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)};
}
/*!
\category Comparison Functions
\beginGroup
Returns `true` if the 3D vectors are equal (or not equal) using floating-point comparison. In
particular, `Float3{0.f} == Float3{-0.f}` is `true`.
*/
PLY_INLINE bool operator==(const Float3& a, const Float3& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
PLY_INLINE bool operator!=(const Float3& a, const Float3& b) {
    return !(a == b);
}
/*!
\endGroup
*/
/*!
Returns `true` if `a` is approximately equal to `b`. The tolerance is given by `epsilon`.

    Float3 v = {0.9999f, 0.0001f, 1.9999f};
    StdOut::text() << isNear(v, Float3{1, 0, 2}, 1e-3f);  // "true"
*/
PLY_INLINE bool isNear(const Float3& a, const Float3& b, float epsilon) {
    return (b - a).length2() <= square(epsilon);
}
/*!
\beginGroup
These functions compare each component individually. The result of each comparison is returned in
a `Bool3`. Call `all()` to check if the result was `true` for all components, or call `any()` to
check if the result was `true` for any component.

    StdOut::text() << all(Float3{1, 2, 3} > Float3{0, 1, 2});  // "true"

These functions are useful for testing whether a point is inside a box. See the implementation of
`Box<>::contains` for an example.
*/
PLY_INLINE Bool3 operator<(const Float3& a, const Float3& b) {
    return {a.x < b.x, a.y < b.y, a.z < b.z};
}
PLY_INLINE Bool3 operator<=(const Float3& a, const Float3& b) {
    return {a.x <= b.x, a.y <= b.y, a.z <= b.z};
}
PLY_INLINE Bool3 operator>(const Float3& a, const Float3& b) {
    return {a.x > b.x, a.y > b.y, a.z > b.z};
}
PLY_INLINE Bool3 operator>=(const Float3& a, const Float3& b) {
    return {a.x >= b.x, a.y >= b.y, a.z >= b.z};
}
/*!
\endGroup
*/
/*!
\category Rounding Functions
\beginGroup
Returns a 3D vector with each component set to the rounded result of the corresponding component of
`vec`. The optional `spacing` argument can be used to round to arbitrary spacings. Most precise when
`spacing` is a power of 2.

    StdOut::text() << roundUp(Float3{-0.3f, 1.4f, 0.8f});  // "{0, 2, 1}"
    StdOut::text() << roundDown(Float3{1.8f}, 0.5f);       // "{1.5, 1.5, 1.5}"
*/
PLY_INLINE Float3 roundUp(const Float3& value, float spacing = 1) {
    return {roundUp(value.x, spacing), roundUp(value.y, spacing), roundUp(value.z, spacing)};
}
PLY_INLINE Float3 roundDown(const Float3& value, float spacing = 1) {
    return {roundDown(value.x, spacing), roundDown(value.y, spacing), roundDown(value.z, spacing)};
}
PLY_INLINE Float3 roundNearest(const Float3& value, float spacing = 1) {
    return {roundNearest(value.x, spacing), roundNearest(value.y, spacing),
            roundNearest(value.z, spacing)};
}
/*!
\endGroup
*/
/*!
Returns `true` if every component of `vec` is already rounded. The optional `spacing` argument can
be used to round to arbitrary spacings. Most precise when `spacing` is a power of 2.

    StdOut::text() << isRounded(Float3{1.5f, 0.5f, 0}, 0.5f);  // "true"
*/
PLY_INLINE bool isRounded(const Float3& value, float spacing = 1) {
    return roundNearest(value, spacing) == value;
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
    \category Constructors
    Constructs an uninitialized 4D vector.
    */
    PLY_INLINE Float4() = default;
    /*!
    Constructs a 4D vector with all components set to `t`.

        Float4 v = {1};
        StdOut::text() << v;  // "{1, 1, 1, 1}"
    */
    PLY_INLINE Float4(float t) : x{t}, y{t}, z{t}, w{t} {
    }
    // Catch wrong number of scalars passed to constructor.
    // This would otherwise promote the first argument to Float2 or Float3:
    Float4(float, float) = delete;
    Float4(float, float, float) = delete;
    /*!
    Constructs a 4D vector from the given components.

        Float4 v = {1, 0, 0, 0};
    */
    PLY_INLINE Float4(float x, float y, float z, float w) : x{x}, y{y}, z{z}, w{w} {
    }
    /*!
    Constructs a 4D vector from a 3D vector and a fourth component.

        Float3 a = {1, 2, 3};
        StdOut::text() << Float4{a, 0};  // "{1, 2, 3, 0}"
    */
    PLY_INLINE Float4(const Float3& v, float w) : x{v.x}, y{v.y}, z{v.z}, w{w} {
    }
    /*!
    Constructs a 4D vector from a 2D vector and two additional components.

        Float2 a = {1, 2};
        StdOut::text() << Float4{a, 0, 0};  // "{1, 2, 0, 0}"
    */
    PLY_INLINE explicit Float4(const Float2& v, float z, float w) : x{v.x}, y{v.y}, z{z}, w{w} {
    }
    /*!
    \category Assignment Operator
    Copy assignment. Declared with an [lvalue
    ref-qualifier](https://en.cppreference.com/w/cpp/language/member_functions#ref-qualified_member_functions)
    so that it's an error to assign to an rvalue, as in `a.normalized() = b`.
    */
    PLY_INLINE void operator=(const Float4& arg) & {
        x = arg.x;
        y = arg.y;
        z = arg.z;
        w = arg.w;
    }
    /*!
    \category Conversion Functions
    Returns a const reference to the first two components as a `Float2` using type punning. This
    should only be used as a temporary expression.

        Float4 v = {4, 5, 6, 7};
        StdOut::text() << v.asFloat2();  // "{4, 5}"
    */
    PLY_INLINE const Float2& asFloat2() const {
        PLY_PUN_SCOPE
        return reinterpret_cast<const Float2&>(*this);
    }
    /*!
    Returns a const reference to the first three components as a `Float4` using type punning. This
    should only be used as a temporary expression.

        Float4 v = {4, 5, 6, 7};
        StdOut::text() << v.asFloat3();  // "{4, 5, 6}"
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
    \category Length Functions
    Returns the square of the length of the 3D vector.
    */
    float length2() const {
        return x * x + y * y + z * z + w * w;
    }
    /*!
    Returns the length of the 3D vector. Equivalent to `sqrtf(this->length2())`.
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
    \category Interpreting as a Color
    \beginGroup
    Aliases for `x`, `y`, `z` and `w`.

        Float4 c = {1.0f, 0.8f, 0.7f, 0.5f};
        StdOut::text().format("{}, {}, {}, {}", c.r(), c.g(), c.b(), c.a());
        // prints "1.0, 0.8, 0.7, 0.5"
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
    \category Swizzle Functions
    \beginGroup
    Swizzle functions.

        Float4 v = {4, 5, 6, 0};
        StdOut::text() << v.swizzle(1, 0);        // "{5, 4}"
        StdOut::text() << v.swizzle(3, 0, 1);     // "{0, 4, 5}"
        StdOut::text() << v.swizzle(2, 3, 2, 1);  // "{6, 0, 6, 5}"
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

/*!
\addToClass Float4
\category Arithmetic Operators
Unary negation.
*/
PLY_INLINE Float4 operator-(const Float4& a) {
    return {-a.x, -a.y, -a.z, -a.w};
}
/*!
\beginGroup
Returns a 4D vector whose components are the result of applying the given operation to the
corresponding components of `a` and `b`. Each component is acted on independently.

    StdOut::text() << Float4{2, 3, 2, 0} * Float4{4, 1, 2, 5};  // "{8, 3, 4, 0}"

If you specify a scalar value in place of a `Float4`, it will be promoted to a `Float4` by
replicating the value to each component.

    StdOut::text() << Float4{2, 3, 2, 0} * 2;  // "{4, 6, 4, 0}"
    StdOut::text() << 8 / Float4{2, 4, 1, 8};  // "{4, 2, 8, 1}"
*/
PLY_INLINE Float4 operator+(const Float4& a, const Float4& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}
PLY_INLINE Float4 operator-(const Float4& a, const Float4& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}
PLY_INLINE Float4 operator*(const Float4& a, const Float4& b) {
    return {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
}
PLY_INLINE Float4 operator/(const Float4& a, const Float4& b) {
    return {a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w};
}
/*!
\endGroup
*/
PLY_INLINE Float4 operator/(const Float4& a, float b) {
    float oob = 1.f / b;
    return {a.x * oob, a.y * oob, a.z * oob, a.w * oob};
}
/*!
\beginGroup
In-place versions of the above operators.

    Float4 v = {2, 3, 2, 0};
    v *= {4, 1, 2, 5};
    StdOut::text() << v;  // "{8, 3, 4, 0}"
*/
PLY_INLINE void operator+=(Float4& a, const Float4& b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    a.w += b.w;
}
PLY_INLINE void operator-=(Float4& a, const Float4& b) {
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    a.w -= b.w;
}
PLY_INLINE void operator*=(Float4& a, const Float4& b) {
    a.x *= b.x;
    a.y *= b.y;
    a.z *= b.z;
    a.w *= b.w;
}
PLY_INLINE void operator/=(Float4& a, const Float4& b) {
    a.x /= b.x;
    a.y /= b.y;
    a.z /= b.z;
    a.w /= b.w;
}
/*!
\endGroup
*/
PLY_INLINE void operator/=(Float4& a, float b) {
    float oob = 1.f / b;
    a.x *= oob;
    a.y *= oob;
    a.z *= oob;
    a.w *= oob;
}
/*!
\category Geometric Functions
Returns the dot product of two 4D vectors.

    StdOut::text() << dot(Float4{2, 3, 1, 3}, Float4{4, 5, 1, 0});  // "24"
*/
PLY_INLINE float dot(const Float4& a, const Float4& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
/*!
\category Component-wise Functions
Returns a copy of `v` with each component constrained to lie within the range determined by the
corresponding components of `mins` and `maxs`.

    Float4 v = {3, 1.5f, 0, 0.5f};
    StdOut::text() << clamp(v, Float4{0, 1, 2, 3}, Float4{1, 2, 3, 4});  // "{1, 1.5, 2, 3}"
    StdOut::text() << clamp(v, 0, 1);                                    // "{1, 1, 0, 0.5f}"
*/
PLY_INLINE Float4 clamp(const Float4& v, const Float4& mins, const Float4& maxs) {
    return {clamp(v.x, mins.x, maxs.x), clamp(v.y, mins.y, maxs.y), clamp(v.z, mins.z, maxs.z),
            clamp(v.w, mins.w, maxs.w)};
}
/*!
Returns a 4D vector with each component set to the absolute value of the corresponding component of
`a`.

    StdOut::text() << abs(Float4{-2, 3, 0, -1});  // "{2, 3, 0, 1}"
*/
PLY_INLINE Float4 abs(const Float4& a) {
    return {fabsf(a.x), fabsf(a.y), fabsf(a.z), fabsf(a.w)};
}
/*!
Returns a 4D vector with each component set to the corresponding component of `a` raised to the
power of the corresponding component of `b`.

    StdOut::text() << pow(Float4{1, 2, 2, 3}, Float4{2, 3, 1, 2});  // "{1, 8, 2, 9}"
    StdOut::text() << pow(Float4{1, 2, 3, -2}, 2);                  // "{1, 4, 9, 4}"
*/
Float4 pow(const Float4& a, const Float4& b);
/*!
Returns a 4D vector with each component set to minimum of the corresponding components of `a` and
`b`.

    StdOut::text() << min(Float4{0, 1, 0, 1}, Float4{1, 0, 1, 0});  // "{0, 0, 0, 0}"
*/
PLY_INLINE Float4 min(const Float4& a, const Float4& b) {
    return {min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w)};
}
/*!
Returns a 4D vector with each component set to maximum of the corresponding components of `a` and
`b`.

    StdOut::text() << max(Float4{0, 1, 0, 1}, Float4{1, 0, 1, 0});  // "{1, 1, 1, 1}"
*/
PLY_INLINE Float4 max(const Float4& a, const Float4& b) {
    return {max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w)};
}
/*!
\category Comparison Functions
\beginGroup
Returns `true` if the 4D vectors are equal (or not equal) using floating-point comparison. In
particular, `Float4{0.f} == Float4{-0.f}` is `true`.
*/
PLY_INLINE bool operator==(const Float4& a, const Float4& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}
PLY_INLINE bool operator!=(const Float4& a, const Float4& b) {
    return !(a == b);
}
/*!
\endGroup
*/
/*!
Returns `true` if `a` is approximately equal to `b`. The tolerance is given by `epsilon`.

    Float4 v = {0.9999f, 0.0001f, 1.9999f, 3.0001f};
    StdOut::text() << isNear(v, Float4{1, 0, 2, 3}, 1e-3f);  // "true"
*/
PLY_INLINE bool isNear(const Float4& a, const Float4& b, float epsilon) {
    return (b - a).length2() <= square(epsilon);
}
/*!
\beginGroup
These functions compare each component individually. The result of each comparison is returned in
a `Bool4`. Call `all()` to check if the result was `true` for all components, or call `any()` to
check if the result was `true` for any component.

    StdOut::text() << all(Float4{1, 2, 3, 4} > Float4{0, 1, 2, 3});  // "true"

These functions are useful for testing whether a point is inside a box. See the implementation of
`Box<>::contains` for an example.
*/
PLY_INLINE Bool4 operator<(const Float4& a, const Float4& b) {
    return {a.x < b.x, a.y < b.y, a.z < b.z, a.w < b.w};
}
PLY_INLINE Bool4 operator<=(const Float4& a, const Float4& b) {
    return {a.x <= b.x, a.y <= b.y, a.z <= b.z, a.w <= b.w};
}
PLY_INLINE Bool4 operator>(const Float4& a, const Float4& b) {
    return {a.x > b.x, a.y > b.y, a.z > b.z, a.w > b.w};
}
PLY_INLINE Bool4 operator>=(const Float4& a, const Float4& b) {
    return {a.x >= b.x, a.y >= b.y, a.z >= b.z, a.w >= b.w};
}
/*!
\endGroup
*/
/*!
\category Rounding Functions
\beginGroup
Returns a 4D vector with each component set to the rounded result of the corresponding component of
`vec`. The optional `spacing` argument can be used to round to arbitrary spacings. Most precise when
`spacing` is a power of 2.

    StdOut::text() << roundUp(Float4{-0.3f, 1.4f, 0.8f, -1.2f});  // "{0, 2, 1, -1}"
    StdOut::text() << roundDown(Float4{1.8f}, 0.5f);              // "{1.5, 1.5, 1.5, 1.5}"
*/
PLY_INLINE Float4 roundUp(const Float4& vec, float spacing = 1) {
    return {roundUp(vec.x, spacing), roundUp(vec.y, spacing), roundUp(vec.z, spacing),
            roundUp(vec.w, spacing)};
}
PLY_INLINE Float4 roundDown(const Float4& vec, float spacing = 1) {
    return {roundDown(vec.x, spacing), roundDown(vec.y, spacing), roundDown(vec.z, spacing),
            roundDown(vec.w, spacing)};
}
PLY_INLINE Float4 roundNearest(const Float4& vec, float spacing = 1) {
    return {roundNearest(vec.x, spacing), roundNearest(vec.y, spacing),
            roundNearest(vec.z, spacing), roundNearest(vec.w, spacing)};
}
/*!
\endGroup
*/
/*!
Returns `true` if every component of `vec` is already rounded. The optional `spacing` argument can
be used to round to arbitrary spacings. Most precise when `spacing` is a power of 2.

    StdOut::text() << isRounded(Float4{1.5f, 0.5f, 0, 2}, 0.5f);  // true
*/
PLY_INLINE bool isRounded(const Float4& vec, float spacing = 1) {
    return roundNearest(vec, spacing) == vec;
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
