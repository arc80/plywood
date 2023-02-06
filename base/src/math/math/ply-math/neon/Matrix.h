/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-math/Core.h>
#include <ply-math/neon/Vector.h>
#include <arm_neon.h>

namespace ply {
namespace neon {

struct Float3x4;

struct Float3x3 {
    Float3 col[3];

    Float3x3() {
    }

    Float3x3(const Float3& col0, const Float3& col1, const Float3& col2)
        : col{col0, col1, col2} {
    }

    static const Float3x3& cast(const Float3x4& arg) {
        return reinterpret_cast<const Float3x3&>(arg);
    }

    void operator=(const Float3x3& arg) {
        col[0] = arg.col[0];
        col[1] = arg.col[1];
        col[2] = arg.col[2];
    }

    Float3 operator*(const Float3& arg) const {
        float32x4_t r = vmulq_lane_f32(col[0].v, vget_low_f32(arg.v), 0);
        r = vmlaq_lane_f32(r, col[1].v, vget_low_f32(arg.v), 1);
        return Float3{vmlaq_lane_f32(r, col[2].v, vget_high_f32(arg.v), 0)};
    }

    Float3x3 operator*(const Float3x3& arg) const {
        return Float3x3{*this * arg.col[0], *this * arg.col[1], *this * arg.col[2]};
    }
};

struct Float3x4 {
    Float3 col[4];

    Float3x4() {
    }

    Float3x4(const Float3& col0, const Float3& col1, const Float3& col2,
             const Float3& col3)
        : col{col0, col1, col2, col3} {
    }

    static const Float3x4& cast(const Float4x4& arg) {
        return reinterpret_cast<const Float3x4&>(arg);
    }

    void operator=(const Float3x4& arg) {
        col[0] = arg.col[0];
        col[1] = arg.col[1];
        col[2] = arg.col[2];
        col[3] = arg.col[3];
    }

    Float3 operator*(const Float3& arg) const {
        float32x4_t r = vmulq_lane_f32(col[0].v, vget_low_f32(arg.v), 0);
        r = vmlaq_lane_f32(r, col[1].v, vget_low_f32(arg.v), 1);
        r = vmlaq_lane_f32(r, col[2].v, vget_high_f32(arg.v), 0);
        return Float3{vaddq_f32(r, col[3].v)};
    }

    Float3x4 operator*(const Float3x4& arg) const {
        const Float3x3& lin = Float3x3::cast(*this);
        return Float3x4{lin * arg.col[0], lin * arg.col[1], lin * arg.col[2],
                        *this * arg.col[3]};
    }
};

struct Float4x4 {
    Float4 col[4];

    Float4x4() {
    }

    Float4x4(const Float4& col0, const Float4& col1, const Float4& col2,
             const Float4& col3)
        : col{col0, col1, col2, col3} {
    }

    static const Float4x4& cast(const Float4x4& arg) {
        return reinterpret_cast<const Float4x4&>(arg);
    }

    void operator=(const Float4x4& arg) {
        col[0] = arg.col[0];
        col[1] = arg.col[1];
        col[2] = arg.col[2];
        col[3] = arg.col[3];
    }

    Float4 operator*(const Float4& arg) const {
        float32x4_t r = vmulq_lane_f32(col[0].v, vget_low_f32(arg.v), 0);
        r = vmlaq_lane_f32(r, col[1].v, vget_low_f32(arg.v), 1);
        r = vmlaq_lane_f32(r, col[2].v, vget_high_f32(arg.v), 0);
        return Float4{vmlaq_lane_f32(r, col[3].v, vget_high_f32(arg.v), 1)};
    }

    Float4x4 operator*(const Float4x4& arg) const {
        return Float4x4{*this * arg.col[0], *this * arg.col[1], *this * arg.col[2],
                        *this * arg.col[3]};
    }
};

} // namespace neon
} // namespace ply
