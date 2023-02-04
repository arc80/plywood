/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-math/Core.h>

namespace ply {

template <typename T>
T interpolateCubic(const T& p0, const T& p1, const T& p2, const T& p3, float t) {
    float omt = 1.f - t;
    return p0 * (omt * omt * omt) + p1 * (3 * omt * omt * t) + p2 * (3 * omt * t * t) +
           p3 * (t * t * t);
}

template <typename T>
T derivativeCubic(const T& p0, const T& p1, const T& p2, const T& p3, float t) {
    T q0 = p1 - p0;
    T q1 = p2 - p1;
    T q2 = p3 - p2;
    T r0 = mix(q0, q1, t);
    T r1 = mix(q1, q2, t);
    T p = mix(r0, r1, t);
    return p;
}

inline float applySimpleCubic(float t) {
    return (3.f - 2.f * t) * t * t;
}

} // namespace ply
