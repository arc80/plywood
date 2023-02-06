/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-math/Core.h>
#include <ply-math/Vector.h>

namespace ply {

// You are expected to pass in a value between -1 and 1
// It maps the value linearly from [-1, 1] to [0, 2 * Pi] and returns the sin of the
// mapped value The wrapper function fast_sin() accepts any argument value in radians
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

inline Float2 fast_cos_sin(float rad) {
    return {fast_cos(rad), fast_sin(rad)};
}

} // namespace ply
