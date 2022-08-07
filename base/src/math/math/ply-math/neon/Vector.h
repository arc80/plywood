/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-math/Core.h>
#include <arm_neon.h>

namespace ply {
namespace neon {

struct Float3 {
    float32x4_t v;

    Float3() {
    }

    Float3(float32x4_t v) : v(v) {
    }

    void operator=(const Float3& arg) {
        v = arg.v;
    }

    Float3 normalized() const {
        float32x4_t m = vmulq_f32(v, v);
        float32x2_t t = vpadd_f32(vget_low_f32(m), vget_low_f32(m));
        t = vadd_f32(t, vget_high_f32(m));
        float32x2_t estimate = vrsqrte_f32(t);
        //--- One iteration step
        float32x2_t e2 = vmul_f32(estimate, t);
        estimate = vmul_f32(estimate, vrsqrts_f32(e2, estimate));
        //---
        float32x4_t s = vdupq_lane_f32(estimate, 0);
        return Float3{vmulq_f32(v, s)};
    }
};

struct Float4 {
    float32x4_t v;

    Float4() {
    }

    Float4(float32x4_t v) : v(v) {
    }

    void operator=(const Float4& arg) {
        v = arg.v;
    }

    Float4 normalized() const {
        float32x4_t m = vmulq_f32(v, v);
        float32x2_t t = vpadd_f32(vget_low_f32(m), vget_high_f32(m));
        t = vpadd_f32(t, t);
        float32x2_t estimate = vrsqrte_f32(t);
        //--- One iteration step
        float32x2_t e2 = vmul_f32(estimate, t);
        estimate = vmul_f32(estimate, vrsqrts_f32(e2, estimate));
        //---
        float32x4_t s = vdupq_lane_f32(estimate, 0);
        return Float4{vmulq_f32(v, s)};
    }
};

} // namespace neon
} // namespace ply
