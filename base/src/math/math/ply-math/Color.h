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

void convert_from_hex(float* values, size_t num_values, const char* hex);

template <typename V>
V from_hex(const char* hex) {
    static constexpr u32 Rows = sizeof(V) / sizeof(V::x);
    V result;
    convert_from_hex(&result.x, Rows, hex);
    return result;
}

inline Int4<u8> to8888(const Float4 value) {
    return (value * 255.9f).to<Int4<u8>>();
}

// FIXME: Accelerate this using a lookup table
inline float s_rgbto_linear(float s) {
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

inline Float3 from_srgb(const Float3& vec) {
    return {s_rgbto_linear(vec.x), s_rgbto_linear(vec.y), s_rgbto_linear(vec.z)};
}

inline Float4 from_srgb(const Float4& vec) {
    return {s_rgbto_linear(vec.x), s_rgbto_linear(vec.y), s_rgbto_linear(vec.z), vec.w};
}

inline Float3 to_srgb(const Float3& vec) {
    return {linear_to_srgb(vec.x), linear_to_srgb(vec.y), linear_to_srgb(vec.z)};
}

inline Float4 to_srgb(const Float4& vec) {
    return {linear_to_srgb(vec.x), linear_to_srgb(vec.y), linear_to_srgb(vec.z), vec.w};
}

} // namespace ply
