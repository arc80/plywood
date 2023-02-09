/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-platform/Core.h>
#include <stddef.h>
#include <math.h>
#include <float.h>
#include <type_traits>
#if defined(_MSC_VER)
#include <intrin.h>
#endif
#if PLY_CPU_X86 || PLY_CPU_X64
#include <emmintrin.h>
#include <xmmintrin.h>
#endif

namespace ply {

static constexpr float Pi = 3.14159265358979323846f;
static constexpr double DPi = 3.14159265358979323846;

template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
inline T square(T v) {
    return v * v;
}

inline float fast_round(float x) {
#if PLY_CPU_ARM64
    return roundf(x);
#elif PLY_CPU_X86 || PLY_CPU_X64
    // Intrinsic version
    __m128 sign_bit =
        _mm_and_ps(_mm_set_ss(x), _mm_castsi128_ps(_mm_cvtsi32_si128(0x80000000u)));
    __m128 added = _mm_add_ss(
        _mm_set_ss(x),
        _mm_or_ps(sign_bit, _mm_castsi128_ps(_mm_cvtsi32_si128(0x3f000000u))));
    return (float) _mm_cvtt_ss2si(added);
#else
    // Non-intrinsic version
    // PLY_PUN_SCOPE
    float frac = 0.5f;
    u32 s = (*(u32*) &x) & 0x80000000u;
    u32 c = (*(u32*) &frac) | s;
    return (float) (s32) (x + *(float*) &c);
#endif
}
inline s32 exact_int(float value) {
    s32 result = (s32) value;
    PLY_ASSERT(float(result) == value);
    return result;
}
inline s32 fast_round_int(float x) {
#if PLY_CPU_ARM64
    return (s32) roundf(x);
#elif PLY_CPU_X86 || PLY_CPU_X64
    // Intrinsic version
    __m128 sign_bit =
        _mm_and_ps(_mm_set_ss(x), _mm_castsi128_ps(_mm_cvtsi32_si128(0x80000000u)));
    __m128 added = _mm_add_ss(
        _mm_set_ss(x),
        _mm_or_ps(sign_bit, _mm_castsi128_ps(_mm_cvtsi32_si128(0x3f000000u))));
    return _mm_cvtt_ss2si(added);
#else
    // Non-intrinsic version
    float frac = 0.5f;
    u32 s = (*(u32*) &x) & 0x80000000u;
    u32 c = (*(u32*) &frac) | s;
    return (s32) (x + *(float*) &c);
#endif
}
inline float round_nearest(float value, float spacing = 1) {
    return fast_round(value / spacing) * spacing;
}
inline float round_down(float value, float spacing = 1) {
    // FIXME: Optimize this
    return floorf(value / spacing) * spacing;
}
inline float round_up(float value, float spacing = 1) {
    // FIXME: Optimize this
    return ceilf(value / spacing) * spacing;
}

inline float wrap(float value, float positive_range) {
    PLY_ASSERT(positive_range > 0);
    float t = floorf(value / positive_range);
    return value - t * positive_range;
}
inline float fraction_part(float value) {
    return value - floorf(value);
}

inline u16 float_to_half(float x) {
    // Only certain floats are supported by this function.
    PLY_ASSERT(fabsf(x) < 65000.f);
    u32 single = *(u32*) &x;
    // If exponent is less than -14, this will force the result to zero.
    u16 zero_mask = -(single + single >= 0x71000000);
    // Exponent and mantissa. Just assume exponent is small enough to avoid wrap around.
    u16 half = ((single >> 16) & 0x8000) | (((single >> 13) - 0x1c000) & 0x7fff);
    return half & zero_mask;
}

template <typename V>
V mix(const V& a, const V& b, float t) {
    return a * (1.f - t) + b * t;
}
template <typename V>
V mix(const V& a, const V& b, const V& t) {
    return a * (V{1} - t) + b * t;
}
template <typename V>
V unmix(const V& a, const V& b, const V& mixed) {
    return (mixed - a) / (b - a);
}
template <typename V>
float is_within(const V& a, const V& b, float epsilon) {
    return square(b - a) <= epsilon * epsilon;
}

template <typename T, typename = std::enable_if_t<!std::is_arithmetic<T>::value>>
T step(const T& from, const T& to, decltype(T::x) amount) {
    T delta = to - from;
    decltype(T::x) length = delta.length();
    return (length < amount) ? to : from + delta * (amount / length);
}

// ┏━━━━━━━━━━━━━━━━━┓
// ┃  Bézier curves  ┃
// ┗━━━━━━━━━━━━━━━━━┛
template <typename T>
T interpolate_cubic(const T& p0, const T& p1, const T& p2, const T& p3, float t) {
    float omt = 1.f - t;
    return p0 * (omt * omt * omt) + p1 * (3 * omt * omt * t) + p2 * (3 * omt * t * t) +
           p3 * (t * t * t);
}

template <typename T>
T derivative_cubic(const T& p0, const T& p1, const T& p2, const T& p3, float t) {
    T q0 = p1 - p0;
    T q1 = p2 - p1;
    T q2 = p3 - p2;
    T r0 = mix(q0, q1, t);
    T r1 = mix(q1, q2, t);
    T p = mix(r0, r1, t);
    return p;
}

inline float apply_simple_cubic(float t) {
    return (3.f - 2.f * t) * t * t;
}

//  ▄▄                   ▄▄▄                           ▄▄
//  ██▄▄▄   ▄▄▄▄   ▄▄▄▄   ██     ▄▄   ▄▄  ▄▄▄▄   ▄▄▄▄ ▄██▄▄  ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄
//  ██  ██ ██  ██ ██  ██  ██     ▀█▄ ▄█▀ ██▄▄██ ██     ██   ██  ██ ██  ▀▀ ▀█▄▄▄
//  ██▄▄█▀ ▀█▄▄█▀ ▀█▄▄█▀ ▄██▄      ▀█▀   ▀█▄▄▄  ▀█▄▄▄  ▀█▄▄ ▀█▄▄█▀ ██      ▄▄▄█▀
//

// ┏━━━━━━━━━┓
// ┃  bvec2  ┃
// ┗━━━━━━━━━┛
struct bvec2 {
    bool x;
    bool y;

    bvec2(bool x, bool y) : x{x}, y{y} {
    }
};

inline bool all(const bvec2& v) {
    return v.x && v.y;
}

inline bool any(const bvec2& v) {
    return v.x || v.y;
}

// ┏━━━━━━━━━┓
// ┃  bvec3  ┃
// ┗━━━━━━━━━┛
struct bvec3 {
    bool x;
    bool y;
    bool z;

    bvec3(bool x, bool y, bool z) : x{x}, y{y}, z{z} {
    }
};

inline bool all(const bvec3& v) {
    return v.x && v.y && v.z;
}

inline bool any(const bvec3& v) {
    return v.x || v.y || v.z;
}

// ┏━━━━━━━━━┓
// ┃  bvec4  ┃
// ┗━━━━━━━━━┛
struct bvec4 {
    bool x;
    bool y;
    bool z;
    bool w;

    bvec4(bool x, bool y, bool z, bool w) : x{x}, y{y}, z{z}, w{w} {
    }
};

inline bool all(const bvec4& v) {
    return v.x && v.y && v.z && v.w;
}

inline bool any(const bvec4& v) {
    return v.x || v.y || v.z || v.w;
}

//   ▄▄▄▄         ▄▄
//  ██  ██ ▄▄  ▄▄ ▄▄  ▄▄▄▄
//  ██▀▀██  ▀██▀  ██ ▀█▄▄▄
//  ██  ██ ▄█▀▀█▄ ██  ▄▄▄█▀
//

enum class Axis : u8 {
    XPos = 0,
    XNeg,
    YPos,
    YNeg,
    ZPos,
    ZNeg,
    Count,
};

inline bool is_valid(Axis vec) {
    return vec < Axis::Count;
}
inline Axis abs(Axis vec) {
    return Axis(s32(vec) & 6);
}
inline s32 sgn(Axis vec) {
    return 1 - (s32(vec) & 1) * 2;
}
inline bool is_perp(Axis va, Axis vb) {
    return ((s32(va) ^ s32(vb)) & 6) != 0;
}
inline Axis cross(Axis va, Axis vb);
inline s32 dot(Axis va, Axis vb);
inline Axis negate(Axis axis3_d) {
    return (Axis) (u8(axis3_d) ^ 1u);
}
inline Axis mul_sign(Axis axis3_d, s32 sgn) {
    PLY_ASSERT(sgn == 1 || sgn == -1);
    return Axis(u32(axis3_d) ^ (sgn < 0));
}

//                        ▄▄▄▄
//  ▄▄   ▄▄  ▄▄▄▄   ▄▄▄▄ ▀▀  ██
//  ▀█▄ ▄█▀ ██▄▄██ ██     ▄█▀▀
//    ▀█▀   ▀█▄▄▄  ▀█▄▄▄ ██▄▄▄▄
//

struct vec3;
struct vec4;

struct vec2 {
    float x;
    float y;

    vec2() = default;
    vec2(float t) : x{t}, y{t} {
    }
    vec2(float x, float y) : x{x}, y{y} {
    }
    template <typename V, typename = void_t<decltype(V::x), decltype(V::y)>>
    explicit vec2(const V& v) : x{(float) v.x}, y{(float) v.y} {
    }

    float length() const;
    bool is_unit() const;
    PLY_NO_DISCARD vec2 normalized() const;
    PLY_NO_DISCARD vec2 safe_normalized(const vec2& fallback = {1, 0},
                                        float epsilon = 1e-9f) const;

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

    PLY_NO_DISCARD vec2 swizzle(u32 i0, u32 i1) const;
    PLY_NO_DISCARD vec3 swizzle(u32 i0, u32 i1, u32 i2) const;
    PLY_NO_DISCARD vec4 swizzle(u32 i0, u32 i1, u32 i2, u32 i3) const;
};

inline float square(const vec2& v) {
    return v.x * v.x + v.y * v.y;
}
inline float vec2::length() const {
    return sqrtf(square(*this));
}
inline bool vec2::is_unit() const {
    return is_within(square(*this), 1.f, 0.001f);
}
inline bool operator==(const vec2& a, const vec2& b) {
    return a.x == b.x && a.y == b.y;
}
inline bool operator!=(const vec2& a, const vec2& b) {
    return !(a == b);
}
inline bvec2 operator<(const vec2& a, const vec2& b) {
    return {a.x < b.x, a.y < b.y};
}
inline bvec2 operator<=(const vec2& a, const vec2& b) {
    return {a.x <= b.x, a.y <= b.y};
}
inline bvec2 operator>(const vec2& a, const vec2& b) {
    return {a.x > b.x, a.y > b.y};
}
inline bvec2 operator>=(const vec2& a, const vec2& b) {
    return {a.x >= b.x, a.y >= b.y};
}
inline vec2 operator-(const vec2& a) {
    return {-a.x, -a.y};
}
inline vec2 operator+(const vec2& a, const vec2& b) {
    return {a.x + b.x, a.y + b.y};
}
inline vec2 operator-(const vec2& a, const vec2& b) {
    return {a.x - b.x, a.y - b.y};
}
inline vec2 operator*(const vec2& a, const vec2& b) {
    return {a.x * b.x, a.y * b.y};
}
inline vec2 operator/(const vec2& a, const vec2& b) {
    return {a.x / b.x, a.y / b.y};
}
inline vec2 operator/(const vec2& a, float b) {
    float oob = 1.f / b;
    return {a.x * oob, a.y * oob};
}
inline void operator+=(vec2& a, const vec2& b) {
    a.x += b.x;
    a.y += b.y;
}
inline void operator-=(vec2& a, const vec2& b) {
    a.x -= b.x;
    a.y -= b.y;
}
inline void operator*=(vec2& a, const vec2& b) {
    a.x *= b.x;
    a.y *= b.y;
}
inline void operator/=(vec2& a, const vec2& b) {
    a.x /= b.x;
    a.y /= b.y;
}
inline void operator/=(vec2& a, float b) {
    float oob = 1.f / b;
    a.x *= oob;
    a.y *= oob;
}
inline float dot(const vec2& a, const vec2& b) {
    return a.x * b.x + a.y * b.y;
}
inline float cross(const vec2& a, const vec2& b) {
    return a.x * b.y - a.y * b.x;
}
inline vec2 clamp(const vec2& v, const vec2& mins, const vec2& maxs) {
    return {clamp(v.x, mins.x, maxs.x), clamp(v.y, mins.y, maxs.y)};
}
inline vec2 abs(const vec2& a) {
    return {fabsf(a.x), fabsf(a.y)};
}
inline vec2 pow(const vec2& a, const vec2& b) {
    return {powf(a.x, b.x), powf(a.y, b.y)};
}
inline vec2 min(const vec2& a, const vec2& b) {
    return {min(a.x, b.x), min(a.y, b.y)};
}
inline vec2 max(const vec2& a, const vec2& b) {
    return {max(a.x, b.x), max(a.y, b.y)};
}
vec2 round_up(const vec2& value, float spacing = 1);
vec2 round_down(const vec2& value, float spacing = 1);
vec2 round_nearest(const vec2& value, float spacing = 1);
bool is_rounded(const vec2& value, float spacing = 1);

//                        ▄▄▄▄
//  ▄▄   ▄▄  ▄▄▄▄   ▄▄▄▄ ▀▀  ██
//  ▀█▄ ▄█▀ ██▄▄██ ██      ▀▀█▄
//    ▀█▀   ▀█▄▄▄  ▀█▄▄▄ ▀█▄▄█▀
//

struct vec3 {
    float x;
    float y;
    float z;

    vec3() = default;
    vec3(float t) : x{t}, y{t}, z{t} {
    }
    vec3(float, float) = delete;
    vec3(float x, float y, float z) : x{x}, y{y}, z{z} {
    }
    vec3(const vec2& v, float z) : x{v.x}, y{v.y}, z{z} {
    }
    explicit vec3(Axis axis) {
        static vec3 table[] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                               {0, -1, 0}, {0, 0, 1},  {0, 0, -1}};
        PLY_ASSERT((u32) axis < PLY_STATIC_ARRAY_SIZE(table));
        *this = table[(u32) axis];
    }
    template <typename V,
              typename = void_t<decltype(V::x), decltype(V::y), decltype(V::z)>>
    explicit vec3(const V& v) : x{(float) v.x}, y{(float) v.y}, z{(float) v.z} {
    }

    float length() const;
    bool is_unit() const;
    PLY_NO_DISCARD vec3 normalized() const;
    PLY_NO_DISCARD vec3 safe_normalized(const vec3& fallback = {1, 0, 0},
                                        float epsilon = 1e-9f) const;

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

    PLY_NO_DISCARD vec2 swizzle(u32 i0, u32 i1) const;
    PLY_NO_DISCARD vec3 swizzle(u32 i0, u32 i1, u32 i2) const;
    PLY_NO_DISCARD vec4 swizzle(u32 i0, u32 i1, u32 i2, u32 i3) const;
};

inline float square(const vec3& v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}
inline float vec3::length() const {
    return sqrtf(square(*this));
}
inline bool vec3::is_unit() const {
    return is_within(square(*this), 1.f, 0.001f);
}
inline bool operator==(const vec3& a, const vec3& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
inline bool operator!=(const vec3& a, const vec3& b) {
    return !(a == b);
}
inline bvec3 operator<(const vec3& a, const vec3& b) {
    return {a.x < b.x, a.y < b.y, a.z < b.z};
}
inline bvec3 operator<=(const vec3& a, const vec3& b) {
    return {a.x <= b.x, a.y <= b.y, a.z <= b.z};
}
inline bvec3 operator>(const vec3& a, const vec3& b) {
    return {a.x > b.x, a.y > b.y, a.z > b.z};
}
inline bvec3 operator>=(const vec3& a, const vec3& b) {
    return {a.x >= b.x, a.y >= b.y, a.z >= b.z};
}
inline vec3 operator-(const vec3& a) {
    return {-a.x, -a.y, -a.z};
}
inline vec3 operator+(const vec3& a, const vec3& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
inline vec3 operator-(const vec3& a, const vec3& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
inline vec3 operator*(const vec3& a, const vec3& b) {
    return {a.x * b.x, a.y * b.y, a.z * b.z};
}
inline vec3 operator/(const vec3& a, const vec3& b) {
    return {a.x / b.x, a.y / b.y, a.z / b.z};
}
inline vec3 operator/(const vec3& a, float b) {
    float oob = 1.f / b;
    return {a.x * oob, a.y * oob, a.z * oob};
}
inline void operator+=(vec3& a, const vec3& b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
}
inline void operator-=(vec3& a, const vec3& b) {
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
}
inline void operator*=(vec3& a, const vec3& b) {
    a.x *= b.x;
    a.y *= b.y;
    a.z *= b.z;
}
inline void operator/=(vec3& a, const vec3& b) {
    a.x /= b.x;
    a.y /= b.y;
    a.z /= b.z;
}
inline void operator/=(vec3& a, float b) {
    float oob = 1.f / b;
    a.x *= oob;
    a.y *= oob;
    a.z *= oob;
}
inline float dot(const vec3& a, const vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
vec3 cross(const vec3& a, const vec3& b);
vec3 clamp(const vec3& v, const vec3& mins, const vec3& maxs);
inline vec3 abs(const vec3& a) {
    return {fabsf(a.x), fabsf(a.y), fabsf(a.z)};
}
vec3 pow(const vec3& a, const vec3& b);
inline vec3 min(const vec3& a, const vec3& b) {
    return {min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)};
}
inline vec3 max(const vec3& a, const vec3& b) {
    return {max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)};
}
vec3 round_up(const vec3& value, float spacing = 1);
vec3 round_down(const vec3& value, float spacing = 1);
vec3 round_nearest(const vec3& value, float spacing = 1);
bool is_rounded(const vec3& value, float spacing = 1);

//                          ▄▄▄
//  ▄▄   ▄▄  ▄▄▄▄   ▄▄▄▄  ▄█▀██
//  ▀█▄ ▄█▀ ██▄▄██ ██    ██▄▄██▄
//    ▀█▀   ▀█▄▄▄  ▀█▄▄▄     ██
//

struct Quaternion;

struct vec4 {
    float x;
    float y;
    float z;
    float w;

    vec4() = default;
    vec4(float t) : x{t}, y{t}, z{t}, w{t} {
    }
    vec4(float, float) = delete;
    vec4(float, float, float) = delete;
    vec4(float x, float y, float z, float w) : x{x}, y{y}, z{z}, w{w} {
    }
    vec4(const vec3& v, float w) : x{v.x}, y{v.y}, z{v.z}, w{w} {
    }
    vec4(const vec2& v, float z, float w) : x{v.x}, y{v.y}, z{z}, w{w} {
    }
    template <typename V, typename = void_t<decltype(V::x), decltype(V::y),
                                            decltype(V::z), decltype(V::w)>>
    explicit vec4(const V& v)
        : x{(float) v.x}, y{(float) v.y}, z{(float) v.z}, w{(float) v.w} {
    }

    const Quaternion& as_quaternion() const;

    float length() const;
    bool is_unit() const;
    PLY_NO_DISCARD vec4 normalized() const;
    PLY_NO_DISCARD vec4 safe_normalized(const vec4& fallback = {1, 0, 0, 0},
                                        float epsilon = 1e-9f) const;

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

    PLY_NO_DISCARD vec2 swizzle(u32 i0, u32 i1) const;
    PLY_NO_DISCARD vec3 swizzle(u32 i0, u32 i1, u32 i2) const;
    PLY_NO_DISCARD vec4 swizzle(u32 i0, u32 i1, u32 i2, u32 i3) const;
};

inline float square(const vec4& v) {
    return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
}
inline float vec4::length() const {
    return sqrtf(square(*this));
}
inline bool vec4::is_unit() const {
    return is_within(square(*this), 1.f, 0.001f);
}
inline bool operator==(const vec4& a, const vec4& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}
inline bool operator!=(const vec4& a, const vec4& b) {
    return !(a == b);
}
inline bvec4 operator<(const vec4& a, const vec4& b) {
    return {a.x < b.x, a.y < b.y, a.z < b.z, a.w < b.w};
}
inline bvec4 operator<=(const vec4& a, const vec4& b) {
    return {a.x <= b.x, a.y <= b.y, a.z <= b.z, a.w <= b.w};
}
inline bvec4 operator>(const vec4& a, const vec4& b) {
    return {a.x > b.x, a.y > b.y, a.z > b.z, a.w > b.w};
}
inline bvec4 operator>=(const vec4& a, const vec4& b) {
    return {a.x >= b.x, a.y >= b.y, a.z >= b.z, a.w >= b.w};
}
inline vec4 operator-(const vec4& a) {
    return {-a.x, -a.y, -a.z, -a.w};
}
inline vec4 operator+(const vec4& a, const vec4& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}
inline vec4 operator-(const vec4& a, const vec4& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}
inline vec4 operator*(const vec4& a, const vec4& b) {
    return {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
}
inline vec4 operator/(const vec4& a, const vec4& b) {
    return {a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w};
}
inline vec4 operator/(const vec4& a, float b) {
    float oob = 1.f / b;
    return {a.x * oob, a.y * oob, a.z * oob, a.w * oob};
}
inline void operator+=(vec4& a, const vec4& b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    a.w += b.w;
}
inline void operator-=(vec4& a, const vec4& b) {
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    a.w -= b.w;
}
inline void operator*=(vec4& a, const vec4& b) {
    a.x *= b.x;
    a.y *= b.y;
    a.z *= b.z;
    a.w *= b.w;
}
inline void operator/=(vec4& a, const vec4& b) {
    a.x /= b.x;
    a.y /= b.y;
    a.z /= b.z;
    a.w /= b.w;
}
inline void operator/=(vec4& a, float b) {
    float oob = 1.f / b;
    a.x *= oob;
    a.y *= oob;
    a.z *= oob;
    a.w *= oob;
}
inline float dot(const vec4& a, const vec4& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
inline vec4 clamp(const vec4& v, const vec4& mins, const vec4& maxs) {
    return {clamp(v.x, mins.x, maxs.x), clamp(v.y, mins.y, maxs.y),
            clamp(v.z, mins.z, maxs.z), clamp(v.w, mins.w, maxs.w)};
}
inline vec4 abs(const vec4& a) {
    return {fabsf(a.x), fabsf(a.y), fabsf(a.z), fabsf(a.w)};
}
vec4 pow(const vec4& a, const vec4& b);
inline vec4 min(const vec4& a, const vec4& b) {
    return {min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w)};
}
inline vec4 max(const vec4& a, const vec4& b) {
    return {max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w)};
}
vec4 round_up(const vec4& vec, float spacing = 1);
vec4 round_down(const vec4& vec, float spacing = 1);
vec4 round_nearest(const vec4& vec, float spacing = 1);
bool is_rounded(const vec4& vec, float spacing = 1);

// ┏━━━━━━━━━━━┓
// ┃  swizzle  ┃
// ┗━━━━━━━━━━━┛
inline PLY_NO_DISCARD vec2 vec2::swizzle(u32 i0, u32 i1) const {
    PLY_PUN_SCOPE
    auto* v = (const float*) this;
    PLY_ASSERT(i0 < 2 && i1 < 2);
    return {v[i0], v[i1]};
}

inline PLY_NO_DISCARD vec3 vec2::swizzle(u32 i0, u32 i1, u32 i2) const {
    PLY_PUN_SCOPE
    auto* v = (const float*) this;
    PLY_ASSERT(i0 < 2 && i1 < 2 && i2 < 2);
    return {v[i0], v[i1], v[i2]};
}

inline PLY_NO_DISCARD vec4 vec2::swizzle(u32 i0, u32 i1, u32 i2, u32 i3) const {
    PLY_PUN_SCOPE
    auto* v = (const float*) this;
    PLY_ASSERT(i0 < 2 && i1 < 2 && i2 < 2 && i3 < 2);
    return {v[i0], v[i1], v[i2], v[i3]};
}

inline PLY_NO_DISCARD vec2 vec3::swizzle(u32 i0, u32 i1) const {
    PLY_PUN_SCOPE
    auto* v = (const float*) this;
    PLY_ASSERT(i0 < 3 && i1 < 3);
    return {v[i0], v[i1]};
}

inline PLY_NO_DISCARD vec3 vec3::swizzle(u32 i0, u32 i1, u32 i2) const {
    PLY_PUN_SCOPE
    auto* v = (const float*) this;
    PLY_ASSERT(i0 < 3 && i1 < 3 && i2 < 3);
    return {v[i0], v[i1], v[i2]};
}

inline PLY_NO_DISCARD vec4 vec3::swizzle(u32 i0, u32 i1, u32 i2, u32 i3) const {
    PLY_PUN_SCOPE
    auto* v = (const float*) this;
    PLY_ASSERT(i0 < 3 && i1 < 3 && i2 < 3 && i2 < 3);
    return {v[i0], v[i1], v[i2], v[i3]};
}

inline PLY_NO_DISCARD vec2 vec4::swizzle(u32 i0, u32 i1) const {
    PLY_PUN_SCOPE
    auto* v = (const float*) this;
    PLY_ASSERT(i0 < 4 && i1 < 4);
    return {v[i0], v[i1]};
}

inline PLY_NO_DISCARD vec3 vec4::swizzle(u32 i0, u32 i1, u32 i2) const {
    PLY_PUN_SCOPE
    auto* v = (const float*) this;
    PLY_ASSERT(i0 < 4 && i1 < 4 && i2 < 4);
    return {v[i0], v[i1], v[i2]};
}

inline PLY_NO_DISCARD vec4 vec4::swizzle(u32 i0, u32 i1, u32 i2, u32 i3) const {
    PLY_PUN_SCOPE
    auto* v = (const float*) this;
    PLY_ASSERT(i0 < 4 && i1 < 4 && i2 < 4 && i2 < 4);
    return {v[i0], v[i1], v[i2], v[i3]};
}

//   ▄▄          ▄▄
//  ▄██▄▄ ▄▄▄▄▄  ▄▄  ▄▄▄▄▄
//   ██   ██  ▀▀ ██ ██  ██
//   ▀█▄▄ ██     ██ ▀█▄▄██
//                   ▄▄▄█▀

inline float fast_sin_part(float x) {
    float val = 4 * x * (fabsf(x) - 1);
    return val * (0.225f * fabsf(val) + 0.775f);
}

inline float fast_sin(float rad) {
    float frac = rad * (0.5f / Pi);
    return fast_sin_part((frac - floorf(frac)) * 2 - 1);
}

inline float fast_cos(float rad) {
    return fast_sin(rad + (Pi * 0.5f));
}

inline vec2 fast_cos_sin(float rad) {
    return {fast_cos(rad), fast_sin(rad)};
}

//  ▄▄▄▄▄
//  ██  ██  ▄▄▄▄  ▄▄  ▄▄
//  ██▀▀█▄ ██  ██  ▀██▀
//  ██▄▄█▀ ▀█▄▄█▀ ▄█▀▀█▄
//

template <class V>
struct Box {
    using T = decltype(V::x);

    V mins;
    V maxs;

    Box() = default;
    Box(T s) : mins{s}, maxs{s} {
    }
    Box(const V& v) : mins{v}, maxs{v} {
    }
    Box(const V& mins, const V& maxs) : mins{mins}, maxs{maxs} {
    }
    template <typename U = T, std::enable_if_t<sizeof(V) == sizeof(U) * 2, int> = 0>
    Box(T min_x, T min_y, T max_x, T max_y) : mins{min_x, min_y}, maxs{max_x, max_y} {
    }
    template <typename U>
    explicit Box(const Box<U>& arg) : mins{arg.mins}, maxs{arg.maxs} {
    }

    static Box empty() {
        return {Limits<T>::Max, Limits<T>::Min};
    }
    static Box full() {
        return {Limits<T>::Min, Limits<T>::Max};
    }

    V size() const {
        return maxs - mins;
    }
    bool is_empty() const {
        return any(maxs <= mins);
    }
    T width() const {
        return maxs.x - mins.x;
    }
    T height() const {
        return maxs.y - mins.y;
    }
    T depth() const {
        return maxs.z - mins.z;
    }
    V mix(const V& arg) const {
        return ply::mix(mins, maxs, arg);
    }
    V unmix(const V& arg) const {
        return ply::unmix(mins, maxs, arg);
    }
    V mid() const {
        return (mins + maxs) * 0.5f;
    }
    Box mix(const Box& arg) const {
        return {mix(arg.mins), mix(arg.maxs)};
    }
    Box unmix(const Box& arg) const {
        return {unmix(arg.mins), unmix(arg.maxs)};
    }
    V clamp(const V& arg) const {
        return clamp(arg, mins, maxs);
    }
    V top_left() const {
        return V{mins.x, maxs.y};
    }
    V bottom_right() const {
        return V{maxs.x, mins.y};
    }
    bool contains(const V& arg) const {
        return all(mins <= arg) && all(arg < maxs);
    }
    bool contains(const Box& arg) const {
        return all(mins <= arg.mins) && all(arg.maxs <= maxs);
    }
    bool intersects(const Box& arg) const {
        return !intersect(*this, arg).is_empty();
    }
};

template <typename V>
bool operator==(const Box<V>& a, const Box<V>& b) {
    return (mins == arg.mins) && (maxs == arg.maxs);
}
template <typename V>
bool operator!=(const Box<V>& a, const Box<V>& b) {
    return !(a == b);
}
template <typename V>
Box<V> operator+(const Box<V>& a, const Box<V>& b) {
    return {a.mins + b.mins, a.maxs + b.maxs};
}
template <typename V>
void operator+=(Box<V>& a, const Box<V>& b) {
    a.mins += b.mins;
    a.maxs += b.maxs;
}
template <typename V>
Box<V> operator-(const Box<V>& a, const Box<V>& b) {
    return {a.mins - b.mins, a.maxs - b.maxs};
}
template <typename V>
void operator-=(Box<V>& a, const Box<V>& b) {
    a.mins -= b.mins;
    a.maxs -= b.maxs;
}
template <typename V>
Box<V> operator*(const Box<V>& a, const Box<V>& b) {
    return {a.mins * b.mins, a.maxs * b.maxs};
}
template <typename V>
void operator*=(Box<V>& a, const Box<V>& b) {
    a.mins *= b.mins;
    a.maxs *= b.maxs;
}
template <typename V>
Box<V> operator/(const Box<V>& a, const Box<V>& b) {
    return {a.mins / b.mins, a.maxs / b.maxs};
}
template <typename V>
void operator/=(Box<V>& a, const Box<V>& b) {
    a.mins /= b.mins;
    a.maxs /= b.maxs;
}
template <typename V>
Box<V> make_union(const Box<V>& a, const Box<V>& b) {
    return {min(a.mins, b.mins), max(a.maxs, b.maxs)};
}
template <typename V>
Box<V> intersect(const Box<V>& a, const Box<V>& b) {
    return {max(a.mins, b.mins), min(a.maxs, b.maxs)};
}
template <typename V>
Box<V> inflate(const Box<V>& a, const V& b) {
    return {a.mins - b, a.maxs + b};
}
template <typename V>
Box<V> inflate(const Box<V>& a, typename Box<V>::T b) {
    return {a.mins - b, a.maxs + b};
}
template <typename V>
Box<V> round_nearest(const Box<V>& a, float spacing = 1) {
    return {round_nearest(a.mins, spacing), round_nearest(a.maxs, spacing)};
}
template <typename V>
Box<V> round_expand(const Box<V>& a, float spacing = 1) {
    return {round_down(a.mins, spacing), round_up(a.maxs, spacing)};
}

// ┏━━━━━━━━━━━━━━━┓
// ┃  Rect, Box3D  ┃
// ┗━━━━━━━━━━━━━━━┛
using Rect = Box<vec2>;
using Box3D = Box<vec3>;

Rect rect_from_fov(float fov_y, float aspect);

//  ▄▄         ▄▄                            ▄▄
//  ▄▄ ▄▄▄▄▄  ▄██▄▄    ▄▄   ▄▄  ▄▄▄▄   ▄▄▄▄ ▄██▄▄  ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄
//  ██ ██  ██  ██      ▀█▄ ▄█▀ ██▄▄██ ██     ██   ██  ██ ██  ▀▀ ▀█▄▄▄
//  ██ ██  ██  ▀█▄▄      ▀█▀   ▀█▄▄▄  ▀█▄▄▄  ▀█▄▄ ▀█▄▄█▀ ██      ▄▄▄█▀
//

// ┏━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃  TVec2, ivec2, uvec2  ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━┛
template <typename T_>
struct TVec2 {
    using T = T_;

    T x;
    T y;

    TVec2() = default;
    TVec2(T x) : x{x}, y{x} {
    }
    TVec2(T x, T y) : x{x}, y{y} {
    }
    template <typename V, typename = void_t<decltype(V::x), decltype(V::y)>>
    explicit TVec2(const V& v) : x{(T) v.x}, y{(T) v.y} {
    }

    T& operator[](size_t i) {
        PLY_ASSERT(i < Rows);
        return reinterpret_cast<T*>(this)[i];
    }
    const T& operator[](size_t i) const {
        PLY_ASSERT(i < Rows);
        return reinterpret_cast<const T*>(this)[i];
    }

    TVec2 swizzle(size_t i, size_t j) const {
        return {((const T*) (this))[i], ((const T*) (this))[j]};
    }
};

template <typename T>
bool operator==(const TVec2<T>& a, const TVec2<T>& b) {
    return (a.x == b.x) && (a.y == b.y);
}
template <typename T>
bool operator!=(const TVec2<T>& a, const TVec2<T>& b) {
    return !(a == b);
}
template <typename T>
bvec2 operator<(const TVec2<T>& a, const TVec2<T>& b) {
    return {a.x < b.x, a.y < b.y};
}
template <typename T>
bvec2 operator<=(const TVec2<T>& a, const TVec2<T>& b) {
    return {a.x <= b.x, a.y <= b.y};
}
template <typename T>
bvec2 operator>(const TVec2<T>& a, const TVec2<T>& b) {
    return {a.x > b.x, a.y > b.y};
}
template <typename T>
bvec2 operator>=(const TVec2<T>& a, const TVec2<T>& b) {
    return {a.x >= b.x, a.y >= b.y};
}
template <typename T>
TVec2<T> operator-(const TVec2<T>& a) {
    return {-a.x, -a.y};
}
template <typename T>
TVec2<T> operator+(const TVec2<T>& a, const TVec2<T>& b) {
    return {a.x + b.x, a.y + b.y};
}
template <typename T>
void operator+=(TVec2<T>& a, const TVec2<T>& b) {
    x += arg.x;
    y += arg.y;
}
template <typename T>
TVec2<T> operator-(const TVec2<T>& a, const TVec2<T>& b) {
    return {a.x - b.x, a.y - b.y};
}
template <typename T>
void operator-=(TVec2<T>& a, const TVec2<T>& b) {
    x -= arg.x;
    y -= arg.y;
}
template <typename T>
TVec2<T> operator*(const TVec2<T>& a, const TVec2<T>& b) {
    return {a.x * b.x, a.y * b.y};
}
template <typename T>
void operator*=(TVec2<T>& a, const TVec2<T>& b) {
    x *= arg.x;
    y *= arg.y;
}
template <typename T>
TVec2<T> operator/(const TVec2<T>& a, const TVec2<T>& b) {
    return {a.x / b.x, a.y / b.y};
}
template <typename T>
void operator/=(TVec2<T>& a, const TVec2<T>& b) {
    x /= arg.x;
    y /= arg.y;
}
template <typename T>
sreg square(const TVec2<T>& v) {
    return sreg(v.x) * v.x + sreg(v.y) * v.y;
}
template <typename T>
sreg dot(const TVec2<T>& a, const TVec2<T>& b) {
    return sreg(a.x) * b.x + sreg(a.y) * b.y;
}
template <typename T>
sreg cross(const TVec2<T>& a, const TVec2<T>& b) {
    return sreg(a.x) * b.y - sreg(a.y) * b.x;
}
template <typename T>
TVec2<T> min(const TVec2<T>& a, const TVec2<T>& b) {
    return TVec2<T>{min(a.x, b.x), min(a.y, b.y)};
}
template <typename T>
TVec2<T> max(const TVec2<T>& a, const TVec2<T>& b) {
    return TVec2<T>{max(a.x, b.x), max(a.y, b.y)};
}

using ivec2 = TVec2<s32>;
using uvec2 = TVec2<u32>;
using IntRect = Box<ivec2>;

// ┏━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃  TVec3, ivec3, uvec3  ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━┛
template <typename T_>
struct TVec3 {
    using T = T_;

    T x;
    T y;
    T z;

    TVec3() = default;
    TVec3(T x) : x{x}, y{x}, z{x} {
    }
    TVec3(T x, T y, T z) : x{x}, y{y}, z{z} {
    }
    template <typename V,
              typename = void_t<decltype(V::x), decltype(V::y), decltype(V::z)>>
    explicit TVec3(const V& v) : x{(T) v.x}, y{(T) v.y}, z{(T) v.z} {
    }

    T& operator[](size_t i) {
        PLY_ASSERT(i < Rows);
        return reinterpret_cast<T*>(this)[i];
    }
    const T& operator[](size_t i) const {
        PLY_ASSERT(i < Rows);
        return reinterpret_cast<const T*>(this)[i];
    }

    TVec3 swizzle(size_t i0, size_t i1, size_t i2) const {
        return TVec3{((const T*) (this))[i0], ((const T*) (this))[i1],
                     ((const T*) (this))[i2]};
    }
};

template <typename T>
bool operator==(const TVec3<T>& a, const TVec3<T>& b) {
    return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}
template <typename T>
bool operator!=(const TVec3<T>& a, const TVec3<T>& b) {
    return !(a == b);
}
template <typename T>
bvec3 operator<(const TVec3<T>& a, const TVec3<T>& b) {
    return {a.x < b.x, a.y < b.y, a.z < b.z};
}
template <typename T>
bvec3 operator<=(const TVec3<T>& a, const TVec3<T>& b) {
    return {a.x <= b.x, a.y <= b.y, a.z <= b.z};
}
template <typename T>
bvec3 operator>(const TVec3<T>& a, const TVec3<T>& b) {
    return {a.x > b.x, a.y > b.y, a.z > b.z};
}
template <typename T>
bvec3 operator>=(const TVec3<T>& a, const TVec3<T>& b) {
    return {a.x >= b.x, a.y >= b.y, a.z >= b.z};
}
template <typename T>
TVec3<T> operator-(const TVec3<T>& a) {
    return {-a.x, -a.y, -a.z};
}
template <typename T>
TVec3<T> operator+(const TVec3<T>& a, const TVec3<T>& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
template <typename T>
void operator+=(TVec3<T>& a, const TVec3<T>& b) {
    x += arg.x;
    y += arg.y;
    z += arg.z;
}
template <typename T>
TVec3<T> operator-(const TVec3<T>& a, const TVec3<T>& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
template <typename T>
void operator-=(TVec3<T>& a, const TVec3<T>& b) {
    x -= arg.x;
    y -= arg.y;
    z -= arg.z;
}
template <typename T>
TVec3<T> operator*(const TVec3<T>& a, const TVec3<T>& b) {
    return {a.x * b.x, a.y * b.y, a.z * b.z};
}
template <typename T>
void operator*=(TVec3<T>& a, const TVec3<T>& b) {
    x *= arg.x;
    y *= arg.y;
    z *= arg.z;
}
template <typename T>
TVec3<T> operator/(const TVec3<T>& a, const TVec3<T>& b) {
    return {a.x / b.x, a.y / b.y, a.z / b.z};
}
template <typename T>
void operator/=(TVec3<T>& a, const TVec3<T>& b) {
    x /= arg.x;
    y /= arg.y;
    z /= arg.z;
}
template <typename T>
sreg square(const TVec3<T>& v) {
    return sreg(v.x) * v.x + sreg(v.y) * v.y + sreg(v.z) * v.z;
}
template <typename T>
sreg dot(const TVec3<T>& a, const TVec3<T>& b) {
    return sreg(a.x) * b.x + sreg(a.y) * b.y + sreg(a.z) * b.z;
}
template <typename T>
TVec3<T> min(const TVec3<T>& a, const TVec3<T>& b) {
    return TVec3<T>{min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)};
}
template <typename T>
TVec3<T> max(const TVec3<T>& a, const TVec3<T>& b) {
    return TVec3<T>{max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)};
}

using ivec3 = TVec3<s32>;
using uvec3 = TVec3<u32>;

// ┏━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃  TVec4, ivec4, uvec4  ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━┛
template <typename T_>
struct TVec4 {
    using T = T_;

    T x;
    T y;
    T z;
    T w;

    TVec4() = default;
    TVec4(T x, T y, T z, T w) : x{x}, y{y}, z{z}, w{w} {
    }
    template <typename V, typename = void_t<decltype(V::x), decltype(V::y),
                                            decltype(V::z), decltype(V::w)>>
    explicit TVec4(const V& v) : x{(T) v.x}, y{(T) v.y}, z{(T) v.z}, w{(T) v.w} {
    }

    T& operator[](size_t i) {
        PLY_ASSERT(i < Rows);
        return reinterpret_cast<T*>(this)[i];
    }
    const T& operator[](size_t i) const {
        PLY_ASSERT(i < Rows);
        return reinterpret_cast<const T*>(this)[i];
    }

    TVec4 swizzle(size_t i0, size_t i1, size_t i2, size_t i3) const {
        return TVec4{((const T*) (this))[i0], ((const T*) (this))[i1],
                     ((const T*) (this))[i2], ((const T*) (this))[i3]};
    }
};

template <typename T>
bool operator==(const TVec4<T>& a, const TVec4<T>& b) {
    return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w);
}
template <typename T>
bool operator!=(const TVec4<T>& a, const TVec4<T>& b) {
    return !(a == b);
}
template <typename T>
bvec4 operator<(const TVec4<T>& a, const TVec4<T>& b) {
    return {a.x < b.x, a.y < b.y, a.z < b.z, a.w < b.w};
}
template <typename T>
bvec4 operator<=(const TVec4<T>& a, const TVec4<T>& b) {
    return {a.x <= b.x, a.y <= b.y, a.z <= b.z, a.w <= b.w};
}
template <typename T>
bvec4 operator>(const TVec4<T>& a, const TVec4<T>& b) {
    return {a.x > b.x, a.y > b.y, a.z > b.z, a.w > b.w};
}
template <typename T>
bvec4 operator>=(const TVec4<T>& a, const TVec4<T>& b) {
    return {a.x >= b.x, a.y >= b.y, a.z >= b.z, a.w >= b.w};
}
template <typename T>
TVec4<T> operator-(const TVec4<T>& a) {
    return {-a.x, -a.y, -a.z, -a.w};
}
template <typename T>
TVec4<T> operator+(const TVec4<T>& a, const TVec4<T>& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}
template <typename T>
void operator+=(TVec4<T>& a, const TVec4<T>& b) {
    x += arg.x;
    y += arg.y;
    z += arg.z;
    w += arg.w;
}
template <typename T>
TVec4<T> operator-(const TVec4<T>& a, const TVec4<T>& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}
template <typename T>
void operator-=(TVec4<T>& a, const TVec4<T>& b) {
    x -= arg.x;
    y -= arg.y;
    z -= arg.z;
    w -= arg.w;
}
template <typename T>
TVec4<T> operator*(const TVec4<T>& a, const TVec4<T>& b) {
    return {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
}
template <typename T>
void operator*=(TVec4<T>& a, const TVec4<T>& b) {
    x *= arg.x;
    y *= arg.y;
    z *= arg.z;
    w *= arg.w;
}
template <typename T>
TVec4<T> operator/(const TVec4<T>& a, const TVec4<T>& b) {
    return {a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w};
}
template <typename T>
void operator/=(TVec4<T>& a, const TVec4<T>& b) {
    x /= arg.x;
    y /= arg.y;
    z /= arg.z;
    w /= arg.w;
}
template <typename T>
sreg square(const TVec4<T>& v) {
    return sreg(v.x) * v.x + sreg(v.y) * v.y + sreg(v.z) * v.z + sreg(v.w) * v.w;
}
template <typename T>
sreg dot(const TVec4<T>& a, const TVec4<T>& b) {
    return sreg(a.x) * b.x + sreg(a.y) * b.y + sreg(a.z) * b.z + sreg(a.w) * b.w;
}
template <typename T>
TVec4<T> min(const TVec4<T>& a, const TVec4<T>& b) {
    return TVec4<T>{min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w)};
}
template <typename T>
TVec4<T> max(const TVec4<T>& a, const TVec4<T>& b) {
    return TVec4<T>{max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w)};
}

using ivec4 = TVec4<s32>;
using uvec4 = TVec4<u32>;
using byte4 = TVec4<u8>;

//   ▄▄▄▄         ▄▄▄
//  ██  ▀▀  ▄▄▄▄   ██   ▄▄▄▄  ▄▄▄▄▄
//  ██     ██  ██  ██  ██  ██ ██  ▀▀
//  ▀█▄▄█▀ ▀█▄▄█▀ ▄██▄ ▀█▄▄█▀ ██
//

void convert_from_hex(float* values, size_t num_values, const char* hex);

template <typename V>
V from_hex(const char* hex) {
    static constexpr u32 Rows = sizeof(V) / sizeof(V::x);
    V result;
    convert_from_hex(&result.x, Rows, hex);
    return result;
}

// FIXME: Accelerate this using a lookup table
inline float srgb_to_linear(float s) {
    if (s < 0.0404482362771082f)
        return s / 12.92f;
    else
        return powf(((s + 0.055f) / 1.055f), 2.4f);
}

inline float linear_to_srgb(float l) {
    if (l < 0.00313066844250063f)
        return l * 12.92f;
    else
        return 1.055f * powf(l, 1 / 2.4f) - 0.055f;
}

vec3 srgb_to_linear(const vec3& vec);
vec4 srgb_to_linear(const vec4& vec);
vec3 linear_to_srgb(const vec3& vec);
vec4 linear_to_srgb(const vec4& vec);

//   ▄▄▄▄         ▄▄        ▄▄▄▄▄          ▄▄
//  ██  ██ ▄▄  ▄▄ ▄▄  ▄▄▄▄  ██  ██  ▄▄▄▄  ▄██▄▄
//  ██▀▀██  ▀██▀  ██ ▀█▄▄▄  ██▀▀█▄ ██  ██  ██
//  ██  ██ ▄█▀▀█▄ ██  ▄▄▄█▀ ██  ██ ▀█▄▄█▀  ▀█▄▄
//

struct AxisRot {
    Axis cols[3];

    AxisRot() = default;
    AxisRot(Axis x_img, Axis y_img, Axis z_img) {
        cols[0] = x_img;
        cols[1] = y_img;
        cols[2] = z_img;
    }
    AxisRot(Axis x_img, Axis y_img) : AxisRot{x_img, y_img, cross(x_img, y_img)} {
    }

    static AxisRot identity() {
        return {Axis::XPos, Axis::YPos, Axis::ZPos};
    }

    Axis operator[](u32 i) const {
        PLY_ASSERT(i < 3);
        return cols[i];
    }

    static AxisRot make_basis(Axis v, u32 i);
    bool is_valid() const;
    bool is_ortho() const;
    bool is_right_handed() const;
    AxisRot inverted() const;
};

Axis operator*(const AxisRot& a, Axis b);
vec3 operator*(const AxisRot& a, const vec3& v);
AxisRot operator*(const AxisRot& a, const AxisRot& b);
bool operator==(const AxisRot& a, const AxisRot& b);
inline bool operator!=(const AxisRot& a, const AxisRot& b) {
    return !(a == b);
}
inline bool operator<(const AxisRot& a, const AxisRot& b) {
    if (a.cols[0] != b.cols[0]) {
        return a.cols[0] < b.cols[0];
    } else if (a.cols[1] != b.cols[1]) {
        return a.cols[1] < b.cols[1];
    } else {
        return a.cols[2] < b.cols[2];
    }
}

//                   ▄▄    ▄▄▄▄          ▄▄▄▄
//  ▄▄▄▄▄▄▄   ▄▄▄▄  ▄██▄▄ ▀▀  ██ ▄▄  ▄▄ ▀▀  ██
//  ██ ██ ██  ▄▄▄██  ██    ▄█▀▀   ▀██▀   ▄█▀▀
//  ██ ██ ██ ▀█▄▄██  ▀█▄▄ ██▄▄▄▄ ▄█▀▀█▄ ██▄▄▄▄
//

struct mat2x2 {
    vec2 col[2];

    mat2x2() = default;
    mat2x2(const vec2& col0, const vec2& col1) : col{col0, col1} {
    }
    vec2& operator[](ureg i) {
        PLY_ASSERT(i < 2);
        return col[i];
    }
    const vec2& operator[](ureg i) const {
        PLY_ASSERT(i < 2);
        return col[i];
    }

    static mat2x2 identity();
    static mat2x2 make_scale(const vec2& scale);
    static mat2x2 make_rotation(float radians);
    static mat2x2 from_complex(const vec2& c);
    mat2x2 transposed() const;
};

bool operator==(const mat2x2& a, const mat2x2& b);
inline bool operator!=(const mat2x2& a, const mat2x2& b) {
    return !(a == b);
}

vec2 operator*(const mat2x2& m, const vec2& v);
mat2x2 operator*(const mat2x2& a, const mat2x2& b);

//                   ▄▄    ▄▄▄▄          ▄▄▄▄
//  ▄▄▄▄▄▄▄   ▄▄▄▄  ▄██▄▄ ▀▀  ██ ▄▄  ▄▄ ▀▀  ██
//  ██ ██ ██  ▄▄▄██  ██     ▀▀█▄  ▀██▀    ▀▀█▄
//  ██ ██ ██ ▀█▄▄██  ▀█▄▄ ▀█▄▄█▀ ▄█▀▀█▄ ▀█▄▄█▀
//

struct mat3x4;
struct mat4x4;

struct mat3x3 {
    vec3 col[3];

    mat3x3() = default;
    mat3x3(const vec3& col0, const vec3& col1, const vec3& col2)
        : col{col0, col1, col2} {
    }
    mat3x3(const AxisRot& a);
    explicit mat3x3(const mat3x4& m);
    explicit mat3x3(const mat4x4& m);

    vec3& operator[](ureg i) {
        PLY_ASSERT(i < 3);
        return col[i];
    }
    const vec3& operator[](ureg i) const {
        PLY_ASSERT(i < 3);
        return col[i];
    }

    static mat3x3 identity();
    static mat3x3 make_scale(const vec3& arg);
    static mat3x3 make_rotation(const vec3& unit_axis, float radians);
    static mat3x3 from_quaternion(const Quaternion& q);

    bool has_scale() const;
    mat3x3 transposed() const;
};

bool operator==(const mat3x3& a, const mat3x3& b);
inline bool operator!=(const mat3x3& a, const mat3x3& b) {
    return !(a == b);
}

vec3 operator*(const mat3x3& m, const vec3& v);
mat3x3 operator*(const mat3x3& a, const mat3x3& b);

//                   ▄▄    ▄▄▄▄            ▄▄▄
//  ▄▄▄▄▄▄▄   ▄▄▄▄  ▄██▄▄ ▀▀  ██ ▄▄  ▄▄  ▄█▀██
//  ██ ██ ██  ▄▄▄██  ██     ▀▀█▄  ▀██▀  ██▄▄██▄
//  ██ ██ ██ ▀█▄▄██  ▀█▄▄ ▀█▄▄█▀ ▄█▀▀█▄     ██
//

struct mat4x4;
struct QuatPos;

struct mat3x4 {
    vec3 col[4];

    mat3x4() = default;
    mat3x4(const vec3& col0, const vec3& col1, const vec3& col2, const vec3& col3)
        : col{col0, col1, col2, col3} {
    }
    explicit mat3x4(const mat3x3& m, const vec3& pos = 0);
    explicit mat3x4(const mat4x4& m);
    const mat3x3& as_mat3x3() const {
        PLY_PUN_SCOPE
        return (const mat3x3&) *this;
    }

    vec3& operator[](ureg i) {
        PLY_ASSERT(i < 4);
        return col[i];
    }
    const vec3& operator[](ureg i) const {
        PLY_ASSERT(i < 4);
        return col[i];
    }
    static mat3x4 identity();
    static mat3x4 make_scale(const vec3& arg);
    static mat3x4 make_rotation(const vec3& unit_axis, float radians);
    static mat3x4 make_translation(const vec3& pos);
    static mat3x4 from_quaternion(const Quaternion& q, const vec3& pos = 0);
    static mat3x4 from_quat_pos(const QuatPos& qp);

    bool has_scale() const {
        return ((mat3x3*) this)->has_scale();
    }
    mat3x4 inverted_ortho() const;
};

bool operator==(const mat3x4& a, const mat3x4& b);
inline bool operator!=(const mat3x4& a, const mat3x4& b) {
    return !(a == b);
}

vec3 operator*(const mat3x4& m, const vec3& v);
vec4 operator*(const mat3x4& m, const vec4& v);
mat3x4 operator*(const mat3x4& a, const mat3x4& b);

//                   ▄▄      ▄▄▄            ▄▄▄
//  ▄▄▄▄▄▄▄   ▄▄▄▄  ▄██▄▄  ▄█▀██  ▄▄  ▄▄  ▄█▀██
//  ██ ██ ██  ▄▄▄██  ██   ██▄▄██▄  ▀██▀  ██▄▄██▄
//  ██ ██ ██ ▀█▄▄██  ▀█▄▄     ██  ▄█▀▀█▄     ██
//

struct mat4x4 {
    vec4 col[4];

    mat4x4() = default;
    mat4x4(const vec4& col0, const vec4& col1, const vec4& col2, const vec4& col3)
        : col{col0, col1, col2, col3} {
    }
    explicit mat4x4(const mat3x3& m, const vec3& pos = 0);
    explicit mat4x4(const mat3x4& m);

    vec4& operator[](ureg i) {
        PLY_ASSERT(i < 4);
        return col[i];
    }
    const vec4& operator[](ureg i) const {
        PLY_ASSERT(i < 4);
        return col[i];
    }

    static mat4x4 identity();
    static mat4x4 make_scale(const vec3& arg);
    static mat4x4 make_rotation(const vec3& unit_axis, float radians);
    static mat4x4 make_translation(const vec3& pos);
    static mat4x4 from_quaternion(const Quaternion& q, const vec3& pos = 0);
    static mat4x4 from_quat_pos(const QuatPos& qp);
    static mat4x4 make_projection(const Rect& frustum, float z_near, float z_far);
    static mat4x4 make_ortho(const Rect& rect, float z_near, float z_far);

    mat4x4 transposed() const;
    mat4x4 inverted_ortho() const;
};

bool operator==(const mat4x4& a, const mat4x4& b);
inline bool operator!=(const mat4x4& a, const mat4x4& b) {
    return !(a == b);
}
vec4 operator*(const mat4x4& m, const vec4& v);
mat4x4 operator*(const mat4x4& a, const mat4x4& b);
mat4x4 operator*(const mat3x4& a, const mat4x4& b);
mat4x4 operator*(const mat4x4& a, const mat3x4& b);

// ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃  Basis matrices (for look at)  ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
inline vec3 not_collinear(const vec3& unit_vec) {
    return square(unit_vec.z) < 0.9f ? vec3{0, 0, 1} : vec3{0, -1, 0};
}

inline mat3x3 make_basis(const vec3& unit_fwd_to, const vec3& up_to, Axis fwd_from,
                         Axis up_from) {
    vec3 right_xpos = cross(unit_fwd_to, up_to);
    float L2 = square(right_xpos);
    if (L2 < 1e-6f) {
        vec3 not_collinear =
            (unit_fwd_to.z * unit_fwd_to.z < 0.9f) ? vec3{0, 0, 1} : vec3{0, -1, 0};
        right_xpos = cross(unit_fwd_to, not_collinear);
        L2 = square(right_xpos);
    }
    right_xpos /= sqrtf(L2);
    vec3 fixed_up_zpos = cross(right_xpos, unit_fwd_to);
    PLY_ASSERT(fabsf(fixed_up_zpos.length() - 1) < 0.001f); // Should have unit length
    return mat3x3{right_xpos, unit_fwd_to, fixed_up_zpos} *
           mat3x3{AxisRot{cross(fwd_from, up_from), fwd_from, up_from}.inverted()};
}

inline mat3x3 make_basis(const vec3& unit_fwd, Axis fwd_from) {
    u32 up_from = u32(fwd_from) + 2;
    up_from -= (up_from >= 6) * 6;
    return make_basis(unit_fwd, not_collinear(unit_fwd), fwd_from, Axis(up_from));
}

//   ▄▄▄▄                         ▄▄▄
//  ██  ▀▀  ▄▄▄▄  ▄▄▄▄▄▄▄  ▄▄▄▄▄   ██   ▄▄▄▄  ▄▄  ▄▄
//  ██     ██  ██ ██ ██ ██ ██  ██  ██  ██▄▄██  ▀██▀
//  ▀█▄▄█▀ ▀█▄▄█▀ ██ ██ ██ ██▄▄█▀ ▄██▄ ▀█▄▄▄  ▄█▀▀█▄
//                         ██

struct Complex {
    static vec2 identity() {
        return vec2{1, 0};
    }

    static vec2 from_angle(float radians) {
        float c = cosf(radians);
        float s = sinf(radians);
        return vec2{c, s};
    }

    static float get_angle(const vec2& v) {
        return atan2f(v.y, v.x);
    }

    static vec2 mul(const vec2& a, const vec2& b) {
        return {a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x};
    }
};

//   ▄▄▄▄                 ▄▄                        ▄▄
//  ██  ██ ▄▄  ▄▄  ▄▄▄▄  ▄██▄▄  ▄▄▄▄  ▄▄▄▄▄  ▄▄▄▄▄  ▄▄  ▄▄▄▄  ▄▄▄▄▄
//  ██  ██ ██  ██  ▄▄▄██  ██   ██▄▄██ ██  ▀▀ ██  ██ ██ ██  ██ ██  ██
//  ▀█▄▄█▀ ▀█▄▄██ ▀█▄▄██  ▀█▄▄ ▀█▄▄▄  ██     ██  ██ ██ ▀█▄▄█▀ ██  ██
//      ▀▀

struct Quaternion {
    float x;
    float y;
    float z;
    float w;

    Quaternion() = default;
    Quaternion(float x, float y, float z, float w) : x{x}, y{y}, z{z}, w{w} {
    }
    Quaternion(const vec3& v, float w) : x{v.x}, y{v.y}, z{v.z}, w{w} {
    }

    const vec3& as_vec3() const {
        PLY_PUN_SCOPE
        return (const vec3&) *this;
    }
    const vec4& as_vec4() const {
        PLY_PUN_SCOPE
        return (const vec4&) *this;
    }

    static Quaternion identity() {
        return {0, 0, 0, 1};
    }
    static Quaternion from_axis_angle(const vec3& unit_axis, float radians);
    static Quaternion from_unit_vectors(const vec3& start, const vec3& end);
    static Quaternion from_ortho(const mat3x3& m);
    static Quaternion from_ortho(const mat3x4& m);
    static Quaternion from_ortho(const mat4x4& m);
    PLY_NO_DISCARD Quaternion inverted() const {
        // Small rotations have large w component, so prefer to keep the same sign of w.
        // Better for interpolation.
        return {-x, -y, -z, w};
    }

    vec3 rotate_unit_x() const;
    vec3 rotate_unit_y() const;
    vec3 rotate_unit_z() const;
    Quaternion normalized() const {
        PLY_PUN_SCOPE
        return (Quaternion&) ((vec4*) this)->normalized();
    }
    bool is_unit() const {
        return is_within(square(*(vec4*) this), 1.f, 0.001f);
    }
    Quaternion negated_if_closer_to(const Quaternion& other) const;
};

inline const Quaternion& vec4::as_quaternion() const {
    PLY_PUN_SCOPE
    return (const Quaternion&) *this;
}

inline Quaternion operator-(const Quaternion& q) {
    return {-q.x, -q.y, -q.z, -q.w};
}
vec3 operator*(const Quaternion& q, const vec3& v);
Quaternion operator*(const Quaternion& a, const Quaternion& b);
Quaternion mix(const Quaternion& a, const Quaternion& b, float f);

//   ▄▄▄▄                 ▄▄   ▄▄▄▄▄
//  ██  ██ ▄▄  ▄▄  ▄▄▄▄  ▄██▄▄ ██  ██  ▄▄▄▄   ▄▄▄▄
//  ██  ██ ██  ██  ▄▄▄██  ██   ██▀▀▀  ██  ██ ▀█▄▄▄
//  ▀█▄▄█▀ ▀█▄▄██ ▀█▄▄██  ▀█▄▄ ██     ▀█▄▄█▀  ▄▄▄█▀
//      ▀▀

struct QuatPos {
    Quaternion quat;
    vec3 pos;

    QuatPos() = default;
    QuatPos(const Quaternion& quat, const vec3& pos) : quat(quat), pos(pos) {
    }
    QuatPos inverted() const;
    static QuatPos identity();
    static QuatPos make_translation(const vec3& pos);
    static QuatPos make_rotation(const vec3& unit_axis, float radians);
    static QuatPos from_ortho(const mat3x4& m);
    static QuatPos from_ortho(const mat4x4& m);
};

inline vec3 operator*(const QuatPos& qp, const vec3& v) {
    return (qp.quat * v) + qp.pos;
}
inline QuatPos operator*(const QuatPos& a, const QuatPos& b) {
    return {a.quat * b.quat, (a.quat * b.pos) + a.pos};
}
inline QuatPos operator*(const QuatPos& a, const Quaternion& b) {
    return {a.quat * b, a.pos};
}
inline QuatPos operator*(const Quaternion& a, const QuatPos& b) {
    return {a * b.quat, a * b.pos};
}
inline mat3x4 mat3x4::from_quat_pos(const QuatPos& qp) {
    return from_quaternion(qp.quat, qp.pos);
}
inline mat4x4 mat4x4::from_quat_pos(const QuatPos& qp) {
    return from_quaternion(qp.quat, qp.pos);
}

} // namespace ply
