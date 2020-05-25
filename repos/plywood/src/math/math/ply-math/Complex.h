/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-math/Core.h>
#include <ply-math/Vector.h>

namespace ply {

struct Complex {
    static Float2 identity() {
        return Float2{1, 0};
    }

    static Float2 fromAngle(float radians) {
        float c = cosf(radians);
        float s = sinf(radians);
        return Float2{c, s};
    }

    static float getAngle(const Float2& v) {
        return atan2f(v.y, v.x);
    }

    static Float2 mul(const Float2& a, const Float2& b) {
        return {a.x * b.x - a.y * b.y, 2 * a.x * b.y};
    }
};

} // namespace ply
