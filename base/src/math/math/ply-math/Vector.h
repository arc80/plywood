/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
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
A vector with two floating-point components `x` and `y`.
*/
struct Float2 {
    /*!
    \begin_group
    */
    float x;
    float y;
    /*!
    \end_group
    */

    /*!
    \category Constructors
    Constructs an uninitialized `Float2`.
    */
    PLY_INLINE Float2() = default;
    /*!
    Constructs a `Float2` with both components set to `t`.

        Float2 v = {1};
        Console::out() << v;  // "{1, 1}"
    */
    PLY_INLINE Float2(float t) : x{t}, y{t} {
    }
    /*!
    Constructs a `Float2` from the given components.

        Float2 v = {1, 0};
    */
    PLY_INLINE Float2(float x, float y) : x{x}, y{y} {
    }
    /*!
    \category Assignment
    Copy assignment. Declared with an [lvalue
    ref-qualifier](https://en.cppreference.com/w/cpp/language/member_functions#ref-qualified_member_functions)
    so that it's an error to assign to an rvalue.

        a.normalized() = b;  // error
    */
    PLY_INLINE void operator=(const Float2& arg) & {
        x = arg.x;
        y = arg.y;
    }
    /*!
    \category Arithmetic Operators
    \category Comparison Functions
    \category Geometric Functions
    \category Length Functions
    Returns the square of the length of the vector.
    */
    PLY_INLINE float length2() const {
        return x * x + y * y;
    }
    /*!
    Returns the length of the vector. Equivalent to `sqrtf(this->length2())`.
    */
    PLY_INLINE float length() const {
        return sqrtf(length2());
    }
    /*!
    Returns `true` if the squared length of the vector is sufficiently close to 1.0. The
    threshold is given by `thresh`.
    */
    PLY_INLINE bool is_unit(float thresh = 0.001f) const {
        return fabsf(1.f - length2()) < thresh;
    }
    /*!
    Returns a unit-length vector having the same direction as `this`. No safety check is
    performed.
    */
    PLY_NO_DISCARD Float2 normalized() const;
    /*!
    Returns a unit-length vector having the same direction as `this` with safety checks.
    */
    PLY_NO_DISCARD Float2 safe_normalized(const Float2& fallback = {1, 0},
                                          float epsilon = 1e-20f) const;
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
    \category Color Functions
    \begin_group
    Convenience functions for interpreting the vector as a color. The `r()` and `g()`
    functions are aliases for the `x` and `y` components respectively.

        Float4 c = {1.0f, 0.8f};
        Console.out().format("{}, {}", c.r(), c.g());  // "1.0, 0.8"
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
    \end_group
    */
    /*!
    \category Swizzle Functions
    \begin_group
    Returns a new vector whose components are taken from the given indices. `x` and `y`
    are at indices 0 and 1 respectively. Similar to [GLSL
    swizzling](https://www.khronos.org/opengl/wiki/Data_Type_(GLSL)#Swizzling) except
    that the components are specified by numeric index, and you can't use it to modify
    the original vector; only to read from it.

        Float2 v = {4, 5};
        Console::out() << v.swizzle(1, 0);        // "{5, 4}"
        Console::out() << v.swizzle(0, 1, 1, 0);  // "{4, 5, 5, 4}"

    These functions work correctly in the current version of all major compilers even
    though they use type punning, which is undefined behavior in standard C++.
    */
    PLY_INLINE PLY_NO_DISCARD Float2 swizzle(u32 i0, u32 i1) const;
    PLY_INLINE PLY_NO_DISCARD Float3 swizzle(u32 i0, u32 i1, u32 i2) const;
    PLY_INLINE PLY_NO_DISCARD Float4 swizzle(u32 i0, u32 i1, u32 i2, u32 i3) const;
    /*!
    \end_group
    */
};

/*!
\add_to_class Float2
\category Arithmetic Operators
Unary negation.
*/
PLY_INLINE Float2 operator-(const Float2& a) {
    return {-a.x, -a.y};
}
/*!
\begin_group
Returns a vector whose components are the result of applying the given operation to the
corresponding components of `a` and `b`. Each component is acted on independently.

    Console::out() << Float2{2, 3} * Float2{4, 1};  // "{8, 3}"

If you specify a scalar value in place of a `Float2`, it will be promoted to a `Float2`
by replicating the value to each component.

    Console::out() << Float2{2, 3} * 2;  // "{4, 6}"
    Console::out() << 8 / Float2{2, 4};  // "{4, 2}"
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
\end_group
*/
PLY_INLINE Float2 operator/(const Float2& a, float b) {
    float oob = 1.f / b;
    return {a.x * oob, a.y * oob};
}
/*!
\begin_group
In-place versions of the above operators.

    Float2 v = {2, 3};
    v *= {4, 1};
    Console::out() << v;  // "{8, 3}"
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
\end_group
*/
PLY_INLINE void operator/=(Float2& a, float b) {
    float oob = 1.f / b;
    a.x *= oob;
    a.y *= oob;
}
/*!
\category Geometric Functions
Returns the dot product of two vectors.

    Console::out() << dot(Float2{1, 0}, Float2{3, 4});  // "2"
*/
PLY_INLINE float dot(const Float2& a, const Float2& b) {
    return a.x * b.x + a.y * b.y;
}
/*!
Returns the cross product of two vectors.

    Console::out() << cross(Float2{1, 0}, Float2{3, 4});  // "4"
*/
PLY_INLINE float cross(const Float2& a, const Float2& b) {
    return a.x * b.y - a.y * b.x;
}
/*!
\category Componentwise Functions
Returns a copy of `v` with each component constrained to lie within the range determined
by the corresponding components of `mins` and `maxs`.

    Float2 v = {3, 1.5f};
    Console::out() << clamp(v, Float2{0, 1}, Float2{1, 2});  // "{1, 1.5}"
    Console::out() << clamp(v, 0, 1);                        // "{1, 1}"
*/
PLY_INLINE Float2 clamp(const Float2& v, const Float2& mins, const Float2& maxs) {
    return {clamp(v.x, mins.x, maxs.x), clamp(v.y, mins.y, maxs.y)};
}
/*!
Returns a vector with each component set to the absolute value of the corresponding
component of `a`.

    Console::out() << abs(Float2{-2, 3});  // "{2, 3}"
*/
PLY_INLINE Float2 abs(const Float2& a) {
    return {fabsf(a.x), fabsf(a.y)};
}
/*!
Returns a vector with each component set to the corresponding component of `a` raised to
the power of the corresponding component of `b`.

    Console::out() << pow(Float2{1, 2}, Float2{2, 3});  // "{1, 8}"
    Console::out() << pow(Float2{1, 2}, 2);             // "{1, 4}"
*/
PLY_INLINE Float2 pow(const Float2& a, const Float2& b) {
    return {powf(a.x, b.x), powf(a.y, b.y)};
}
/*!
Returns a vector with each component set to minimum of the corresponding components of
`a` and `b`.

    Console::out() << min(Float2{0, 1}, Float2{1, 0});  // "{0, 0}"
*/
PLY_INLINE Float2 min(const Float2& a, const Float2& b) {
    return {min(a.x, b.x), min(a.y, b.y)};
}
/*!
Returns a vector with each component set to maximum of the corresponding components of
`a` and `b`.

    Console::out() << max(Float2{0, 1}, Float2{1, 0});  // "{1, 1}"
*/
PLY_INLINE Float2 max(const Float2& a, const Float2& b) {
    return {max(a.x, b.x), max(a.y, b.y)};
}
/*!
\category Comparison Functions
\begin_group
Returns `true` if the vectors are equal (or not equal) using floating-point comparison.
In particular, `Float2{0.f} == Float2{-0.f}` is `true`.
*/
PLY_INLINE bool operator==(const Float2& a, const Float2& b) {
    return a.x == b.x && a.y == b.y;
}
PLY_INLINE bool operator!=(const Float2& a, const Float2& b) {
    return !(a == b);
}
/*!
\end_group
*/
/*!
Returns `true` if `a` is approximately equal to `b`. The tolerance is given by
`epsilon`.

    Float2 v = {0.9999f, 0.0001f};
    Console::out() << is_near(v, Float2{1, 0}, 1e-3f);  // "true"
*/
PLY_INLINE bool is_near(const Float2& a, const Float2& b, float epsilon) {
    return (b - a).length2() <= square(epsilon);
}
/*!
\begin_group
These functions compare each component individually. The result of each comparison is
returned in a `Bool2`. Call `all()` to check if the result was `true` for all
components, or call `any()` to check if the result was `true` for any component.

    Console::out() << all(Float2{1, 2} > Float2{0, 1});  // "true"

These functions are useful for testing whether a point is inside a box. See the
implementation of `Box<>::contains` for an example.
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
\end_group
*/
/*!
\category Rounding Functions
\begin_group
Returns a vector with each component set to the rounded result of the corresponding
component of `vec`. The optional `spacing` argument can be used to round to arbitrary
spacings. Most precise when `spacing` is a power of 2.

    Console::out() << round_up(Float2{-0.3f, 1.4f});   // "{0, 2}"
    Console::out() << round_down(Float2{1.8f}, 0.5f);  // "{1.5, 1.5}"
*/
PLY_INLINE Float2 round_up(const Float2& value, float spacing = 1) {
    return {round_up(value.x, spacing), round_up(value.y, spacing)};
}
PLY_INLINE Float2 round_down(const Float2& value, float spacing = 1) {
    return {round_down(value.x, spacing), round_down(value.y, spacing)};
}
PLY_INLINE Float2 round_nearest(const Float2& value, float spacing = 1) {
    // Good to let the compiler see the spacing so it can optimize the divide by
    // constant
    return {round_nearest(value.x, spacing), round_nearest(value.y, spacing)};
}
/*!
\end_group
*/
/*!
Returns `true` if every component of `vec` is already rounded. The optional `spacing`
argument can be used to round to arbitrary spacings. Most precise when `spacing` is a
power of 2.

    Console::out() << is_rounded(Float2{1.5f, 2.5f}, 0.5f);  // "true"
*/
PLY_INLINE bool is_rounded(const Float2& value, float spacing = 1) {
    return round_nearest(value, spacing) == value;
}

//------------------------------------------------------------------------------------------------
/*!
A vector with three floating-point components `x`, `y` and `z`.
*/
struct Float3 {
    /*!
    \begin_group
    */
    float x;
    float y;
    float z;
    /*!
    \end_group
    */

    /*!
    \category Constructors
    Constructs an uninitialized `Float3`.
    */
    PLY_INLINE Float3() = default;
    /*!
    Constructs a `Float3` with all components set to `t`.

        Float3 v = {1};
        Console::out() << v;  // "{1, 1, 1}"
    */
    PLY_INLINE Float3(float t) : x{t}, y{t}, z{t} {
    }
    // Catch the case where 2 scalars are passed to constructor.
    // This would otherwise promote the first scalar to Float2:
    Float3(float, float) = delete;
    /*!
    Constructs a `Float3` from the given components.

        Float3 v = {1, 0, 0};
    */
    PLY_INLINE Float3(float x, float y, float z) : x{x}, y{y}, z{z} {
    }
    /*!
    Constructs a `Float3` from a `Float2` and a third component.

        Float2 a = {1, 2};
        Console::out() << Float3{a, 0};  // "{1, 2, 0}"
    */
    PLY_INLINE Float3(const Float2& v, float z) : x{v.x}, y{v.y}, z{z} {
    }
    /*!
    \category Assignment
    Copy assignment. Declared with an [lvalue
    ref-qualifier](https://en.cppreference.com/w/cpp/language/member_functions#ref-qualified_member_functions)
    so that it's an error to assign to an rvalue.

        a.normalized() = b;  // error
    */
    PLY_INLINE void operator=(const Float3& arg) & {
        x = arg.x;
        y = arg.y;
        z = arg.z;
    }
    /*!
    \category Arithmetic Operators
    \category Comparison Functions
    \category Geometric Functions
    \category Length Functions
    Returns the square of the length of the vector.
    */
    PLY_INLINE float length2() const {
        return x * x + y * y + z * z;
    }
    /*!
    Returns the length of the vector. Equivalent to `sqrtf(this->length2())`.
    */
    PLY_INLINE float length() const {
        return sqrtf(length2());
    }
    /*!
    Returns `true` if the squared length of the vector is sufficiently close to 1.0. The
    threshold is given by `thresh`.
    */
    PLY_INLINE bool is_unit(float thresh = 0.001f) const {
        return fabsf(1.f - length2()) < thresh;
    }
    /*!
    Returns a unit-length vector having the same direction as `this`. No safety check is
    performed.
    */
    PLY_NO_DISCARD Float3 normalized() const;
    /*!
    Returns a unit-length vector having the same direction as `this` with safety checks.
    */
    PLY_NO_DISCARD Float3 safe_normalized(const Float3& fallback = {1, 0, 0},
                                          float epsilon = 1e-20f) const;
    /*!
    \category Conversion Functions
    Returns a const reference to the first two components as a `Float2` using type
    punning. This should only be used as a temporary expression.

        Float3 v = {4, 5, 6};
        Console::out() << v.as_float2();  // "{4, 5}"
    */
    PLY_INLINE const Float2& as_float2() const {
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
    \category Color Functions
    \begin_group
    Convenience functions for interpreting the vector as a color. The `r()`, `g()` and
    `b()` functions are aliases for the `x`, `y` and `z` components respectively.

        Float3 c = {1.0f, 0.8f, 0.7f};
        Console.out().format("{}, {}, {}", c.r(), c.g(), c.b());  // "1.0, 0.8, 0.7"
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
    \end_group
    */
    /*!
    \category Swizzle Functions
    \begin_group
    Returns a new vector whose components are taken from the given indices. `x`, `y` and
    `z` are at indices 0, 1 and 2 respectively. Similar to [GLSL
    swizzling](https://www.khronos.org/opengl/wiki/Data_Type_(GLSL)#Swizzling) except
    that the components are specified by numeric index, and you can't use it to modify
    the original vector; only to read from it.

        Float3 v = {4, 5, 6};
        Console::out() << v.swizzle(1, 0);        // "{5, 4}"
        Console::out() << v.swizzle(2, 0, 2, 1);  // "{6, 4, 6, 5}"

    These functions work correctly in the current version of all major compilers even
    though they use type punning, which is undefined behavior in standard C++.
    */
    PLY_INLINE PLY_NO_DISCARD Float2 swizzle(u32 i0, u32 i1) const;
    PLY_INLINE PLY_NO_DISCARD Float3 swizzle(u32 i0, u32 i1, u32 i2) const;
    PLY_INLINE PLY_NO_DISCARD Float4 swizzle(u32 i0, u32 i1, u32 i2, u32 i3) const;
    /*!
    \end_group
    */
};

/*!
\add_to_class Float3
\category Arithmetic Operators
Unary negation.
*/
PLY_INLINE Float3 operator-(const Float3& a) {
    return {-a.x, -a.y, -a.z};
}
/*!
\begin_group
Returns a vector whose components are the result of applying the given operation to the
corresponding components of `a` and `b`. Each component is acted on independently.

    Console::out() << Float3{2, 3, 2} * Float3{4, 1, 2};  // "{8, 3, 4}"

If you specify a scalar value in place of a `Float3`, it will be promoted to a `Float3`
by replicating the value to each component.

    Console::out() << Float3{2, 3, 2} * 2;  // "{4, 6, 4}"
    Console::out() << 8 / Float3{2, 4, 1};  // "{4, 2, 8}"
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
\end_group
*/
PLY_INLINE Float3 operator/(const Float3& a, float b) {
    float oob = 1.f / b;
    return {a.x * oob, a.y * oob, a.z * oob};
}
/*!
\begin_group
In-place versions of the above operators.

    Float3 v = {2, 3, 2};
    v *= {4, 1, 2};
    Console::out() << v;  // "{8, 3, 4}"
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
\end_group
*/
PLY_INLINE void operator/=(Float3& a, float b) {
    float oob = 1.f / b;
    a.x *= oob;
    a.y *= oob;
    a.z *= oob;
}
/*!
\category Geometric Functions
Returns the dot product of two vectors.

    Console::out() << dot(Float3{2, 3, 1}, Float3{4, 5, 1});  // "24"
*/
PLY_INLINE float dot(const Float3& a, const Float3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
/*!
Returns the cross product of two vectors.

    Console::out() << cross(Float3{1, 0, 0}, Float3{0, 1, 0});  // "{0, 0, 1}"
*/
Float3 cross(const Float3& a, const Float3& b);
/*!
\category Componentwise Functions
Returns a copy of `v` with each component constrained to lie within the range determined
by the corresponding components of `mins` and `maxs`.

    Float3 v = {3, 1.5f, 0};
    Console::out() << clamp(v, Float3{0, 1, 2}, Float3{1, 2, 3});  // "{1, 1.5, 2}"
    Console::out() << clamp(v, 0, 1);                              // "{1, 1, 0}"
*/
Float3 clamp(const Float3& v, const Float3& mins, const Float3& maxs);
/*!
Returns a vector with each component set to the absolute value of the corresponding
component of `a`.

    Console::out() << abs(Float3{-2, 3, 0});  // "{2, 3, 0}"
*/
PLY_INLINE Float3 abs(const Float3& a) {
    return {fabsf(a.x), fabsf(a.y), fabsf(a.z)};
}
/*!
Returns a vector with each component set to the corresponding component of `a` raised to
the power of the corresponding component of `b`.

    Console::out() << pow(Float3{1, 2, 2}, Float3{2, 3, 1});  // "{1, 8, 2}"
    Console::out() << pow(Float3{1, 2, 3}, 2);                // "{1, 4, 9}"
*/
Float3 pow(const Float3& a, const Float3& b);
/*!
Returns a vector with each component set to minimum of the corresponding components of
`a` and `b`.

    Console::out() << min(Float3{0, 1, 0}, Float3{1, 0, 1});  // "{0, 0, 0}"
*/
PLY_INLINE Float3 min(const Float3& a, const Float3& b) {
    return {min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)};
}
/*!
Returns a vector with each component set to maximum of the corresponding components of
`a` and `b`.

    Console::out() << max(Float3{0, 1, 0}, Float3{1, 0, 1});  // "{1, 1, 1}"
*/
PLY_INLINE Float3 max(const Float3& a, const Float3& b) {
    return {max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)};
}
/*!
\category Comparison Functions
\begin_group
Returns `true` if the vectors are equal (or not equal) using floating-point comparison.
In particular, `Float3{0.f} == Float3{-0.f}` is `true`.
*/
PLY_INLINE bool operator==(const Float3& a, const Float3& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
PLY_INLINE bool operator!=(const Float3& a, const Float3& b) {
    return !(a == b);
}
/*!
\end_group
*/
/*!
Returns `true` if `a` is approximately equal to `b`. The tolerance is given by
`epsilon`.

    Float3 v = {0.9999f, 0.0001f, 1.9999f};
    Console::out() << is_near(v, Float3{1, 0, 2}, 1e-3f);  // "true"
*/
PLY_INLINE bool is_near(const Float3& a, const Float3& b, float epsilon) {
    return (b - a).length2() <= square(epsilon);
}
/*!
\begin_group
These functions compare each component individually. The result of each comparison is
returned in a `Bool3`. Call `all()` to check if the result was `true` for all
components, or call `any()` to check if the result was `true` for any component.

    Console::out() << all(Float3{1, 2, 3} > Float3{0, 1, 2});  // "true"

These functions are useful for testing whether a point is inside a box. See the
implementation of `Box<>::contains` for an example.
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
\end_group
*/
/*!
\category Rounding Functions
\begin_group
Returns a vector with each component set to the rounded result of the corresponding
component of `vec`. The optional `spacing` argument can be used to round to arbitrary
spacings. Most precise when `spacing` is a power of 2.

    Console::out() << round_up(Float3{-0.3f, 1.4f, 0.8f});  // "{0, 2, 1}"
    Console::out() << round_down(Float3{1.8f}, 0.5f);       // "{1.5, 1.5, 1.5}"
*/
PLY_INLINE Float3 round_up(const Float3& value, float spacing = 1) {
    return {round_up(value.x, spacing), round_up(value.y, spacing),
            round_up(value.z, spacing)};
}
PLY_INLINE Float3 round_down(const Float3& value, float spacing = 1) {
    return {round_down(value.x, spacing), round_down(value.y, spacing),
            round_down(value.z, spacing)};
}
PLY_INLINE Float3 round_nearest(const Float3& value, float spacing = 1) {
    return {round_nearest(value.x, spacing), round_nearest(value.y, spacing),
            round_nearest(value.z, spacing)};
}
/*!
\end_group
*/
/*!
Returns `true` if every component of `vec` is already rounded. The optional `spacing`
argument can be used to round to arbitrary spacings. Most precise when `spacing` is a
power of 2.

    Console::out() << is_rounded(Float3{1.5f, 0.5f, 0}, 0.5f);  // "true"
*/
PLY_INLINE bool is_rounded(const Float3& value, float spacing = 1) {
    return round_nearest(value, spacing) == value;
}

//------------------------------------------------------------------------------------------------
/*!
A vector with four floating-point components `x`, `y`, `z` and `w`.
*/
struct Float4 {
    /*!
    \begin_group
    `w` is the fourth component. It follows `z` sequentially in memory.
    */
    float x;
    float y;
    float z;
    float w;
    /*!
    \end_group
    */

    /*!
    \category Constructors
    Constructs an uninitialized `Float4`.
    */
    PLY_INLINE Float4() = default;
    /*!
    Constructs a `Float4` with all components set to `t`.

        Float4 v = {1};
        Console::out() << v;  // "{1, 1, 1, 1}"
    */
    PLY_INLINE Float4(float t) : x{t}, y{t}, z{t}, w{t} {
    }
    // Catch wrong number of scalars passed to constructor.
    // This would otherwise promote the first argument to Float2 or Float3:
    Float4(float, float) = delete;
    Float4(float, float, float) = delete;
    /*!
    Constructs a `Float4` from the given components.

        Float4 v = {1, 0, 0, 0};
    */
    PLY_INLINE Float4(float x, float y, float z, float w) : x{x}, y{y}, z{z}, w{w} {
    }
    /*!
    Constructs a `Float4` from a `Float3` and a fourth component.

        Float3 a = {1, 2, 3};
        Console::out() << Float4{a, 0};  // "{1, 2, 3, 0}"
    */
    PLY_INLINE Float4(const Float3& v, float w) : x{v.x}, y{v.y}, z{v.z}, w{w} {
    }
    /*!
    Constructs a `Float4` from a `Float2` and two additional components.

        Float2 a = {1, 2};
        Console::out() << Float4{a, 0, 0};  // "{1, 2, 0, 0}"
    */
    PLY_INLINE Float4(const Float2& v, float z, float w) : x{v.x}, y{v.y}, z{z}, w{w} {
    }
    /*!
    \category Assignment
    Copy assignment. Declared with an [lvalue
    ref-qualifier](https://en.cppreference.com/w/cpp/language/member_functions#ref-qualified_member_functions)
    so that it's an error to assign to an rvalue.

        a.normalized() = b;  // error
    */
    PLY_INLINE void operator=(const Float4& arg) & {
        x = arg.x;
        y = arg.y;
        z = arg.z;
        w = arg.w;
    }
    /*!
    \category Arithmetic Operators
    \category Comparison Functions
    \category Geometric Functions
    \category Length Functions
    Returns the square of the length of the vector.
    */
    float length2() const {
        return x * x + y * y + z * z + w * w;
    }
    /*!
    Returns the length of the vector. Equivalent to `sqrtf(this->length2())`.
    */
    float length() const {
        return sqrtf(length2());
    }
    /*!
    Returns `true` if the squared length of the vector is sufficiently close to 1.0. The
    threshold is given by `thresh`.
    */
    PLY_INLINE bool is_unit(float thresh = 0.001f) const {
        return fabsf(1.f - length2()) < thresh;
    }
    /*!
    Returns a unit-length vector having the same direction as `this`. No safety check is
    performed.
    */
    PLY_NO_DISCARD Float4 normalized() const;
    /*!
    Returns a unit-length vector having the same direction as `this` with safety checks.
    */
    PLY_NO_DISCARD Float4 safe_normalized(const Float4& fallback = {1, 0, 0, 0},
                                          float epsilon = 1e-20f) const;
    /*!
    \category Conversion Functions
    Returns a const reference to the first two components as a `Float2` using type
    punning. This should only be used as a temporary expression.

        Float4 v = {4, 5, 6, 7};
        Console::out() << v.as_float2();  // "{4, 5}"
    */
    PLY_INLINE const Float2& as_float2() const {
        PLY_PUN_SCOPE
        return reinterpret_cast<const Float2&>(*this);
    }
    /*!
    Returns a const reference to the first three components as a `Float3` using type
    punning. This should only be used as a temporary expression.

        Float4 v = {4, 5, 6, 7};
        Console::out() << v.as_float3();  // "{4, 5, 6}"
    */
    PLY_INLINE const Float3& as_float3() const {
        PLY_PUN_SCOPE
        return reinterpret_cast<const Float3&>(*this);
    }
    /*!
    Casts to `Quaternion` using type punning. This should only be used as a temporary
    expression.
    */
    PLY_INLINE const Quaternion& as_quaternion() const;
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
    \category Color Functions
    \begin_group
    Convenience functions for interpreting the vector as a color. The `r()`, `g()`,
    `b()` and `a()` functions are aliases for the `x`, `y`, `z` and `w` components
    respectively.

        Float4 c = {1.0f, 0.8f, 0.7f, 0.5f};
        Console.out().format("{}, {}, {}, {}", c.r(), c.g(), c.b(), c.a());
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
    \end_group
    */
    /*!
    \category Swizzle Functions
    \begin_group
    Returns a new vector whose components are taken from the given indices. `x`, `y`,
    `z` and `w` are at indices 0, 1, 2 and 3 respectively. Similar to [GLSL
    swizzling](https://www.khronos.org/opengl/wiki/Data_Type_(GLSL)#Swizzling) except
    that the components are specified by numeric index, and you can't use it to modify
    the original vector; only to read from it.

        Float4 v = {4, 5, 6, 0};
        Console::out() << v.swizzle(1, 0);        // "{5, 4}"
        Console::out() << v.swizzle(2, 3, 2, 1);  // "{6, 0, 6, 5}"

    These functions work correctly in the current version of all major compilers even
    though they use type punning, which is undefined behavior in standard C++.
    */
    PLY_INLINE PLY_NO_DISCARD Float2 swizzle(u32 i0, u32 i1) const;
    PLY_INLINE PLY_NO_DISCARD Float3 swizzle(u32 i0, u32 i1, u32 i2) const;
    PLY_INLINE PLY_NO_DISCARD Float4 swizzle(u32 i0, u32 i1, u32 i2, u32 i3) const;
    /*!
    \end_group
    */
};

/*!
\add_to_class Float4
\category Arithmetic Operators
Unary negation.
*/
PLY_INLINE Float4 operator-(const Float4& a) {
    return {-a.x, -a.y, -a.z, -a.w};
}
/*!
\begin_group
Returns a vector whose components are the result of applying the given operation to the
corresponding components of `a` and `b`. Each component is acted on independently.

    Console::out() << Float4{2, 3, 2, 0} * Float4{4, 1, 2, 5};  // "{8, 3, 4, 0}"

If you specify a scalar value in place of a `Float4`, it will be promoted to a `Float4`
by replicating the value to each component.

    Console::out() << Float4{2, 3, 2, 0} * 2;  // "{4, 6, 4, 0}"
    Console::out() << 8 / Float4{2, 4, 1, 8};  // "{4, 2, 8, 1}"
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
\end_group
*/
PLY_INLINE Float4 operator/(const Float4& a, float b) {
    float oob = 1.f / b;
    return {a.x * oob, a.y * oob, a.z * oob, a.w * oob};
}
/*!
\begin_group
In-place versions of the above operators.

    Float4 v = {2, 3, 2, 0};
    v *= {4, 1, 2, 5};
    Console::out() << v;  // "{8, 3, 4, 0}"
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
\end_group
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
Returns the dot product of two vectors.

    Console::out() << dot(Float4{2, 3, 1, 3}, Float4{4, 5, 1, 0});  // "24"
*/
PLY_INLINE float dot(const Float4& a, const Float4& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
/*!
\category Componentwise Functions
Returns a copy of `v` with each component constrained to lie within the range determined
by the corresponding components of `mins` and `maxs`.

    Float4 v = {3, 1.5f, 0, 0.5f};
    Console::out() << clamp(v, Float4{0, 1, 2, 3}, Float4{1, 2, 3, 4});  // "{1, 1.5, 2,
3}" Console::out() << clamp(v, 0, 1);                                    // "{1, 1, 0,
0.5f}"
*/
PLY_INLINE Float4 clamp(const Float4& v, const Float4& mins, const Float4& maxs) {
    return {clamp(v.x, mins.x, maxs.x), clamp(v.y, mins.y, maxs.y),
            clamp(v.z, mins.z, maxs.z), clamp(v.w, mins.w, maxs.w)};
}
/*!
Returns a vector with each component set to the absolute value of the corresponding
component of `a`.

    Console::out() << abs(Float4{-2, 3, 0, -1});  // "{2, 3, 0, 1}"
*/
PLY_INLINE Float4 abs(const Float4& a) {
    return {fabsf(a.x), fabsf(a.y), fabsf(a.z), fabsf(a.w)};
}
/*!
Returns a vector with each component set to the corresponding component of `a` raised to
the power of the corresponding component of `b`.

    Console::out() << pow(Float4{1, 2, 2, 3}, Float4{2, 3, 1, 2});  // "{1, 8, 2, 9}"
    Console::out() << pow(Float4{1, 2, 3, -2}, 2);                  // "{1, 4, 9, 4}"
*/
Float4 pow(const Float4& a, const Float4& b);
/*!
Returns a vector with each component set to minimum of the corresponding components of
`a` and `b`.

    Console::out() << min(Float4{0, 1, 0, 1}, Float4{1, 0, 1, 0});  // "{0, 0, 0, 0}"
*/
PLY_INLINE Float4 min(const Float4& a, const Float4& b) {
    return {min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w)};
}
/*!
Returns a vector with each component set to maximum of the corresponding components of
`a` and `b`.

    Console::out() << max(Float4{0, 1, 0, 1}, Float4{1, 0, 1, 0});  // "{1, 1, 1, 1}"
*/
PLY_INLINE Float4 max(const Float4& a, const Float4& b) {
    return {max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w)};
}
/*!
\category Comparison Functions
\begin_group
Returns `true` if the vectors are equal (or not equal) using floating-point comparison.
In particular, `Float4{0.f} == Float4{-0.f}` is `true`.
*/
PLY_INLINE bool operator==(const Float4& a, const Float4& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}
PLY_INLINE bool operator!=(const Float4& a, const Float4& b) {
    return !(a == b);
}
/*!
\end_group
*/
/*!
Returns `true` if `a` is approximately equal to `b`. The tolerance is given by
`epsilon`.

    Float4 v = {0.9999f, 0.0001f, 1.9999f, 3.0001f};
    Console::out() << is_near(v, Float4{1, 0, 2, 3}, 1e-3f);  // "true"
*/
PLY_INLINE bool is_near(const Float4& a, const Float4& b, float epsilon) {
    return (b - a).length2() <= square(epsilon);
}
/*!
\begin_group
These functions compare each component individually. The result of each comparison is
returned in a `Bool4`. Call `all()` to check if the result was `true` for all
components, or call `any()` to check if the result was `true` for any component.

    Console::out() << all(Float4{1, 2, 3, 4} > Float4{0, 1, 2, 3});  // "true"

These functions are useful for testing whether a point is inside a box. See the
implementation of `Box<>::contains` for an example.
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
\end_group
*/
/*!
\category Rounding Functions
\begin_group
Returns a vector with each component set to the rounded result of the corresponding
component of `vec`. The optional `spacing` argument can be used to round to arbitrary
spacings. Most precise when `spacing` is a power of 2.

    Console::out() << round_up(Float4{-0.3f, 1.4f, 0.8f, -1.2f});  // "{0, 2, 1, -1}"
    Console::out() << round_down(Float4{1.8f}, 0.5f);              //
"{1.5, 1.5, 1.5, 1.5}"
*/
PLY_INLINE Float4 round_up(const Float4& vec, float spacing = 1) {
    return {round_up(vec.x, spacing), round_up(vec.y, spacing),
            round_up(vec.z, spacing), round_up(vec.w, spacing)};
}
PLY_INLINE Float4 round_down(const Float4& vec, float spacing = 1) {
    return {round_down(vec.x, spacing), round_down(vec.y, spacing),
            round_down(vec.z, spacing), round_down(vec.w, spacing)};
}
PLY_INLINE Float4 round_nearest(const Float4& vec, float spacing = 1) {
    return {round_nearest(vec.x, spacing), round_nearest(vec.y, spacing),
            round_nearest(vec.z, spacing), round_nearest(vec.w, spacing)};
}
/*!
\end_group
*/
/*!
Returns `true` if every component of `vec` is already rounded. The optional `spacing`
argument can be used to round to arbitrary spacings. Most precise when `spacing` is a
power of 2.

    Console::out() << is_rounded(Float4{1.5f, 0.5f, 0, 2}, 0.5f);  // true
*/
PLY_INLINE bool is_rounded(const Float4& vec, float spacing = 1) {
    return round_nearest(vec, spacing) == vec;
}

//---------------------------------

typedef Box<Float2> Rect;
typedef Box<Float3> Box3D;

Rect rect_from_fov(float fov_y, float aspect);

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
