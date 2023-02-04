/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-math/Core.h>
#include <ply-math/Vector.h>
#include <ply-math/IntVector.h>

namespace ply {

void convertFromHex(float* values, size_t numValues, const char* hex);

template <typename V>
V fromHex(const char* hex) {
    static constexpr u32 Rows = sizeof(V) / sizeof(V::x);
    V result;
    convertFromHex(&result.x, Rows, hex);
    return result;
}

inline Int4<u8> to8888(const Float4 value) {
    return (value * 255.9f).to<Int4<u8>>();
}

// FIXME: Accelerate this using a lookup table
inline float sRGBToLinear(float s) {
    if (s < 0.0404482362771082f)
        return s / 12.92f;
    else
        return powf(((s + 0.055f) / 1.055f), 2.4f);
}

inline float linearToSRGB(float l) {
    if (l < 0.00313066844250063f)
        return l * 12.92f;
    else
        return 1.055f * powf(l, 1 / 2.4f) - 0.055f;
}

inline Float3 fromSRGB(const Float3& vec) {
    return {sRGBToLinear(vec.x), sRGBToLinear(vec.y), sRGBToLinear(vec.z)};
}

inline Float4 fromSRGB(const Float4& vec) {
    return {sRGBToLinear(vec.x), sRGBToLinear(vec.y), sRGBToLinear(vec.z), vec.w};
}

inline Float3 toSRGB(const Float3& vec) {
    return {linearToSRGB(vec.x), linearToSRGB(vec.y), linearToSRGB(vec.z)};
}

inline Float4 toSRGB(const Float4& vec) {
    return {linearToSRGB(vec.x), linearToSRGB(vec.y), linearToSRGB(vec.z), vec.w};
}

} // namespace ply
