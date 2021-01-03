/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
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

// Constants
static constexpr float Pi = 3.14159265358979323846f;
static constexpr double DPi = 3.14159265358979323846;

// Math
inline float square(float v) {
    return v * v;
}
inline double square(double v) {
    return v * v;
}
template <typename T>
inline T approach(T from, T to, T step) {
    return (from <= to) ? min(from + step, to) : max(from - step, to);
}
inline u32 reverse(u32 v) {
    return (v << 24) | ((v << 8) & 0xff0000) | ((v >> 8) & 0xff00) | (v >> 24);
}
template <typename T>
inline bool bitwiseEqual(const T& a, const T& b) {
    return memcmp(&a, &b, sizeof(T)) == 0;
}
inline float fastRound(float x) {
#if PLY_CPU_ARM64
    return roundf(x);
#elif PLY_CPU_X86 || PLY_CPU_X64
    // Intrinsic version
    __m128 signBit = _mm_and_ps(_mm_set_ss(x), _mm_castsi128_ps(_mm_cvtsi32_si128(0x80000000u)));
    __m128 added = _mm_add_ss(_mm_set_ss(x),
                              _mm_or_ps(signBit, _mm_castsi128_ps(_mm_cvtsi32_si128(0x3f000000u))));
    return (float) _mm_cvtt_ss2si(added);
#else
    // Non-intrinsic version
    // PLY_PUN_SCOPE
    float frac = 0.5f;
    u32 s = (*(u32*) &x) & 0x80000000u;
    u32 c = (*(u32*) &frac) | s;
    return (float) (s32)(x + *(float*) &c);
#endif
}
inline s32 exactInt(float value) {
    s32 result = (s32) value;
    PLY_ASSERT(float(result) == value);
    return result;
}
inline s32 fastRoundInt(float x) {
#if PLY_CPU_ARM64
    return (s32) roundf(x);
#elif PLY_CPU_X86 || PLY_CPU_X64
    // Intrinsic version
    __m128 signBit = _mm_and_ps(_mm_set_ss(x), _mm_castsi128_ps(_mm_cvtsi32_si128(0x80000000u)));
    __m128 added = _mm_add_ss(_mm_set_ss(x),
                              _mm_or_ps(signBit, _mm_castsi128_ps(_mm_cvtsi32_si128(0x3f000000u))));
    return _mm_cvtt_ss2si(added);
#else
    // Non-intrinsic version
    float frac = 0.5f;
    u32 s = (*(u32*) &x) & 0x80000000u;
    u32 c = (*(u32*) &frac) | s;
    return (s32)(x + *(float*) &c);
#endif
}
inline float roundNearest(float value, float spacing = 1) {
    return fastRound(value / spacing) * spacing;
}

PLY_INLINE float roundDown(float value, float spacing = 1) {
    // FIXME: Optimize this
    return floorf(value / spacing) * spacing;
}

PLY_INLINE float roundUp(float value, float spacing = 1) {
    // FIXME: Optimize this
    return ceilf(value / spacing) * spacing;
}

inline float wrap(float value, float positiveRange) {
    PLY_ASSERT(positiveRange > 0);
    float t = floorf(value / positiveRange);
    return value - t * positiveRange;
}

inline float wrapOne(float value) {
    return value - floorf(value);
}

template <typename V, typename S>
inline V mix(const V& from, const V& to, const S& f) {
    return from * (S(1) - f) + to * f;
}

template <typename V, typename S>
inline V unmix(const V& from, const V& to, const S& x) {
    return (x - from) / (to - from);
}

} // namespace ply
