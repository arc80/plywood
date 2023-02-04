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
// It maps the value linearly from [-1, 1] to [0, 2 * Pi] and returns the sin of the mapped
// value The wrapper function fastSin() accepts any argument value in radians
inline float fastSinPart(float x) {
    float val = 4 * x * (fabsf(x) - 1);
    return val * (0.225f * fabsf(val) + 0.775f);
}

inline float fastSin(float rad) {
    float frac = rad * (0.5f / Pi);
    return fastSinPart((frac - floorf(frac)) * 2 - 1);
}

inline float fastCos(float rad) {
    return fastSin(rad + (Pi * 0.5f));
}

inline Float2 fastCosSin(float rad) {
    return {fastCos(rad), fastSin(rad)};
}

} // namespace ply
