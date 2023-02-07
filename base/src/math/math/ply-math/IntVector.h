/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-math/Core.h>
#include <ply-math/Box.h>

namespace ply {

//----------------------------------------------------
// Int2
//----------------------------------------------------

template <typename T_>
struct Int2 {
    typedef T_ T;
    static const size_t Rows = 2;

    T x;
    T y;

    Int2() = default;

    Int2(T x) : x{x}, y{x} {
    }

    Int2(T x, T y) : x{x}, y{y} {
    }

    template <typename OtherVec2>
    OtherVec2 to() const {
        using T = decltype(OtherVec2::x);
        PLY_STATIC_ASSERT(sizeof(OtherVec2) == sizeof(T) * 2);
        return {(T) x, (T) y};
    }

    T& operator[](size_t i) {
        PLY_ASSERT(i < Rows);
        return reinterpret_cast<T*>(this)[i];
    }

    const T& operator[](size_t i) const {
        PLY_ASSERT(i < Rows);
        return reinterpret_cast<const T*>(this)[i];
    }

    static Int2 splat(T arg) { // FIXME: Obsolete now that there's a constructor?
        return Int2{arg, arg};
    }

    Int2 swizzle(size_t i, size_t j) const {
        return Int2{((const T*) (this))[i], ((const T*) (this))[j]};
    }

    Int2 reverse() const {
        return Int2{y, x};
    }

    void operator=(const Int2& arg) {
        x = arg.x;
        y = arg.y;
    }

    bool operator==(const Int2& arg) const {
        return (x == arg.x) && (y == arg.y);
    }

    bool operator!=(const Int2& arg) const {
        return !(*this == arg);
    }

    Int2 operator-() const {
        return Int2{-x, -y};
    }

    Int2 operator+(T arg) const {
        return Int2{T(x + arg), T(y + arg)};
    }

    Int2 operator+(const Int2& arg) const {
        return Int2{T(x + arg.x), T(y + arg.y)};
    }

    void operator+=(const Int2& arg) {
        x += arg.x;
        y += arg.y;
    }

    Int2 operator-(T arg) const {
        return Int2{T(x - arg), T(y - arg)};
    }

    Int2 operator-(const Int2& arg) const {
        return Int2{T(x - arg.x), T(y - arg.y)};
    }

    void operator-=(const Int2& arg) {
        x -= arg.x;
        y -= arg.y;
    }

    Int2 operator*(T arg) const {
        return Int2{x * arg, y * arg};
    }

    void operator*=(T arg) {
        x *= arg;
        y *= arg;
    }

    Int2 operator*(const Int2& arg) const {
        return Int2{x * arg.x, y * arg.y};
    }

    void operator*=(const Int2& arg) {
        x *= arg.x;
        y *= arg.y;
    }

    Int2 operator/(T arg) const {
        return Int2{x / arg, y / arg};
    }

    void operator/=(T arg) {
        x /= arg;
        y /= arg;
    }

    Int2 operator/(const Int2& arg) const {
        return Int2{x / arg.x, y / arg.y};
    }

    void operator/=(const Int2& arg) {
        x /= arg.x;
        y /= arg.y;
    }

    ureg length2() const {
        return sreg(x) * x + sreg(y) * y;
    }

    Int2 yx() const {
        return {y, x};
    }
};

template <typename T>
sreg dot(const Int2<T>& a, const Int2<T>& b) {
    return sreg(a.x) * b.x + sreg(a.y) * b.y;
}

template <typename T>
sreg cross(const Int2<T>& a, const Int2<T>& b) {
    return sreg(a.x) * b.y - sreg(a.y) * b.x;
}

template <typename T>
Int2<T> min(const Int2<T>& a, const Int2<T>& b) {
    return Int2<T>{min(a.x, b.x), min(a.y, b.y)};
}

template <typename T>
Int2<T> max(const Int2<T>& a, const Int2<T>& b) {
    return Int2<T>{max(a.x, b.x), max(a.y, b.y)};
}

template <typename T>
Bool2 operator<(const Int2<T>& a, const Int2<T>& b) {
    return {a.x < b.x, a.y < b.y};
}
template <typename T>
Bool2 operator<=(const Int2<T>& a, const Int2<T>& b) {
    return {a.x <= b.x, a.y <= b.y};
}
template <typename T>
Bool2 operator>(const Int2<T>& a, const Int2<T>& b) {
    return {a.x > b.x, a.y > b.y};
}
template <typename T>
Bool2 operator>=(const Int2<T>& a, const Int2<T>& b) {
    return {a.x >= b.x, a.y >= b.y};
}

using IntVec2 = Int2<s32>;
using IntRect = Box<Int2<s32>>;

//----------------------------------------------------
// Int3
//----------------------------------------------------

template <typename T_>
struct Int3 {
    typedef T_ T;
    static const size_t Rows = 3;

    T x;
    T y;
    T z;

    Int3() = default;

    Int3(T x) : x{x}, y{x}, z{x} {
    }

    Int3(T x, T y, T z) : x{x}, y{y}, z{z} {
    }

    Int2<T>& as_int2() {
        return reinterpret_cast<Int2<T>&>(*this);
    }

    const Int2<T>& as_int2() const {
        return reinterpret_cast<const Int2<T>&>(*this);
    }

    template <typename OtherVec3>
    OtherVec3 to() const {
        using T = decltype(OtherVec3::x);
        PLY_STATIC_ASSERT(sizeof(OtherVec3) == sizeof(T) * 3);
        return {(T) x, (T) y, (T) z};
    }

    T& operator[](size_t i) {
        PLY_ASSERT(i < Rows);
        return reinterpret_cast<T*>(this)[i];
    }

    const T& operator[](size_t i) const {
        PLY_ASSERT(i < Rows);
        return reinterpret_cast<const T*>(this)[i];
    }

    static Int3 splat(T arg) {
        return Int3{arg, arg, arg};
    }

    Int3 swizzle(size_t i0, size_t i1, size_t i2) const {
        return Int3{((const T*) (this))[i0], ((const T*) (this))[i1],
                    ((const T*) (this))[i2]};
    }

    Int3 reverse() const {
        return Int3{z, y, x};
    }

    void operator=(const Int3& arg) {
        x = arg.x;
        y = arg.y;
        z = arg.z;
    }

    bool operator==(const Int3& arg) const {
        return (x == arg.x) && (y == arg.y) && (z == arg.z);
    }

    bool operator!=(const Int3& arg) const {
        return !(*this == arg);
    }

    Int3 operator-() const {
        return Int3{-x, -y, -z};
    }

    Int3 operator+(const Int3& arg) const {
        return Int3{T(x + arg.x), T(y + arg.y), T(z + arg.z)};
    }

    void operator+=(const Int3& arg) {
        x += arg.x;
        y += arg.y;
        z += arg.z;
    }

    Int3 operator-(const Int3& arg) const {
        return Int3{T(x - arg.x), T(y - arg.y), T(z - arg.z)};
    }

    void operator-=(const Int3& arg) {
        x -= arg.x;
        y -= arg.y;
        z -= arg.z;
    }

    Int3 operator*(T arg) const {
        return Int3{x * arg, y * arg, z * arg};
    }

    void operator*=(T arg) {
        x *= arg;
        y *= arg;
        z *= arg;
    }

    Int3 operator*(const Int3& arg) const {
        return Int3{x * arg.x, y * arg.y, z * arg.z};
    }

    void operator*=(const Int3& arg) {
        x *= arg.x;
        y *= arg.y;
        z *= arg.z;
    }

    Int3 operator/(T arg) const {
        return Int3{x / arg, y / arg, z / arg};
    }

    void operator/=(T arg) {
        x /= arg;
        y /= arg;
        z /= arg;
    }

    Int3 operator/(const Int3& arg) const {
        return Int3{x / arg.x, y / arg.y, z / arg.z};
    }

    void operator/=(const Int3& arg) {
        x /= arg.x;
        y /= arg.y;
        z /= arg.z;
    }

    ureg length2() const {
        return sreg(x) * x + sreg(y) * y + sreg(z) * z;
    }
};

template <typename T>
sreg dot(const Int3<T>& a, const Int3<T>& b) {
    return sreg(a.x) * b.x + sreg(a.y) * b.y + sreg(a.z) * b.z;
}

template <typename T>
Int3<T> min(const Int3<T>& a, const Int3<T>& b) {
    return Int3<T>{min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)};
}

template <typename T>
Int3<T> max(const Int3<T>& a, const Int3<T>& b) {
    return Int3<T>{max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)};
}

template <typename T>
Bool3 operator<(const Int3<T>& a, const Int3<T>& b) {
    return {a.x < b.x, a.y < b.y, a.z < b.z};
}
template <typename T>
Bool3 operator<=(const Int3<T>& a, const Int3<T>& b) {
    return {a.x <= b.x, a.y <= b.y, a.z <= b.z};
}
template <typename T>
Bool3 operator>(const Int3<T>& a, const Int3<T>& b) {
    return {a.x > b.x, a.y > b.y, a.z > b.z};
}
template <typename T>
Bool3 operator>=(const Int3<T>& a, const Int3<T>& b) {
    return {a.x >= b.x, a.y >= b.y, a.z >= b.z};
}

using IntVec3 = Int3<s32>;

//----------------------------------------------------
// Int4
//----------------------------------------------------

template <typename T_>
struct Int4 {
    typedef T_ T;
    static const size_t Rows = 4;

    T x;
    T y;
    T z;
    T w;

    Int4() = default;

    Int4(T x, T y, T z, T w) : x{x}, y{y}, z{z}, w{w} {
    }

    template <typename OtherVec4>
    OtherVec4 to() const {
        using T = decltype(OtherVec4::x);
        PLY_STATIC_ASSERT(sizeof(OtherVec4) == sizeof(T) * 4);
        return {(T) x, (T) y, (T) z, (T) w};
    }

    T& operator[](size_t i) {
        PLY_ASSERT(i < Rows);
        return reinterpret_cast<T*>(this)[i];
    }

    const T& operator[](size_t i) const {
        PLY_ASSERT(i < Rows);
        return reinterpret_cast<const T*>(this)[i];
    }

    static Int4 splat(T arg) {
        return Int4{arg, arg, arg, arg};
    }

    Int4 swizzle(size_t i0, size_t i1, size_t i2, size_t i3) const {
        return Int4{((const T*) (this))[i0], ((const T*) (this))[i1],
                    ((const T*) (this))[i2], ((const T*) (this))[i3]};
    }

    Int4 reverse() const {
        return Int4{w, z, y, x};
    }

    void operator=(const Int4& arg) {
        x = arg.x;
        y = arg.y;
        z = arg.z;
        w = arg.w;
    }

    bool operator==(const Int4& arg) const {
        return (x == arg.x) && (y == arg.y) && (z == arg.z) && (w == arg.w);
    }

    bool operator!=(const Int4& arg) const {
        return !(*this == arg);
    }

    Int4 operator-() const {
        return Int4{-x, -y, -z, -w};
    }

    Int4 operator+(const Int4& arg) const {
        return Int4{T(x + arg.x), T(y + arg.y), T(z + arg.z), T(w + arg.w)};
    }

    void operator+=(const Int4& arg) {
        x += arg.x;
        y += arg.y;
        z += arg.z;
        w += arg.w;
    }

    Int4 operator-(const Int4& arg) const {
        return Int4{T(x - arg.x), T(y - arg.y), T(z - arg.z), T(w - arg.w)};
    }

    void operator-=(const Int4& arg) {
        x -= arg.x;
        y -= arg.y;
        z -= arg.z;
        w -= arg.w;
    }

    Int4 operator*(T arg) const {
        return Int4{x * arg, y * arg, z * arg, w * arg};
    }

    void operator*=(T arg) {
        x *= arg;
        y *= arg;
        z *= arg;
        w *= arg;
    }

    Int4 operator*(const Int4& arg) const {
        return Int4{x * arg.x, y * arg.y, z * arg.z, w * arg.w};
    }

    void operator*=(const Int4& arg) {
        x *= arg.x;
        y *= arg.y;
        z *= arg.z;
        w *= arg.w;
    }

    Int4 operator/(T arg) const {
        return Int4{x / arg, y / arg, z / arg, w / arg};
    }

    void operator/=(T arg) {
        x /= arg;
        y /= arg;
        z /= arg;
        w /= arg;
    }

    Int4 operator/(const Int4& arg) const {
        return Int4{x / arg.x, y / arg.y, z / arg.z, w / arg.w};
    }

    void operator/=(const Int4& arg) {
        x /= arg.x;
        y /= arg.y;
        z /= arg.z;
        w /= arg.w;
    }

    ureg length2() const {
        return sreg(x) * x + sreg(y) * y + sreg(z) * z + sreg(w) * w;
    }
};

template <typename T>
sreg dot(const Int4<T>& a, const Int4<T>& b) {
    return sreg(a.x) * b.x + sreg(a.y) * b.y + sreg(a.z) * b.z + sreg(a.w) * b.w;
}

template <typename T>
Int4<T> min(const Int4<T>& a, const Int4<T>& b) {
    return Int4<T>{min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w)};
}

template <typename T>
Int4<T> max(const Int4<T>& a, const Int4<T>& b) {
    return Int4<T>{max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w)};
}

template <typename T>
Bool4 operator<(const Int4<T>& a, const Int4<T>& b) {
    return {a.x < b.x, a.y < b.y, a.z < b.z, a.w < b.w};
}
template <typename T>
Bool4 operator<=(const Int4<T>& a, const Int4<T>& b) {
    return {a.x <= b.x, a.y <= b.y, a.z <= b.z, a.w <= b.w};
}
template <typename T>
Bool4 operator>(const Int4<T>& a, const Int4<T>& b) {
    return {a.x > b.x, a.y > b.y, a.z > b.z, a.w > b.w};
}
template <typename T>
Bool4 operator>=(const Int4<T>& a, const Int4<T>& b) {
    return {a.x >= b.x, a.y >= b.y, a.z >= b.z, a.w >= b.w};
}

using IntVec4 = Int4<s32>;

} // namespace ply
