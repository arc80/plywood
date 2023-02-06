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

inline u16 float_to_half(float x) {
    PLY_ASSERT(fabsf(x) <
               65000.f); // Only certain floats are supported by this function
    u32 single = *(u32*) &x;
    u16 zero_mask = -(
        single + single >=
        0x71000000); // is exponent is less than -14, this will force the result to zero
    u16 half = ((single >> 16) & 0x8000) | // sign
               (((single >> 13) - 0x1c000) &
                0x7fff); // exponent and mantissa (just assume exponent
                         // is small enough to avoid wrap around)
    return half & zero_mask;
}

inline Int4<u16> float_to_half(const Float4& v) {
    return {float_to_half(v.x), float_to_half(v.y), float_to_half(v.z),
            float_to_half(v.w)};
}

} // namespace ply
