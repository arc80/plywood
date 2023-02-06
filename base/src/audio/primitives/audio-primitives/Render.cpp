/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <audio-primitives/Core.h>
#include <audio-primitives/Render.h>
#include <audio-primitives/Saturate.h>

namespace ply {
namespace audio {

void mix(const Buffer& dst_buffer, const Buffer& src_buffer) {
    PLY_ASSERT(dst_buffer.num_samples == src_buffer.num_samples);
    PLY_ASSERT(dst_buffer.sample_rate == src_buffer.sample_rate);
    PLY_ASSERT(dst_buffer.format == src_buffer.format);
    PLY_ASSERT(dst_buffer.format.sample_type == Format::SampleType::S16);

    s16* dst = (s16*) dst_buffer.samples;
    s16* dst_end = (s16*) dst_buffer.get_end_ptr();
    s16* src = (s16*) src_buffer.samples;
    while (dst < dst_end) {
        *dst = saturating_add(*dst, *src);
        dst++;
        src++;
    }
}

void mix_leveled(const Buffer& dst_buffer, const Buffer& src_buffer, s32 level16) {
    PLY_ASSERT(dst_buffer.num_samples == src_buffer.num_samples);
    PLY_ASSERT(dst_buffer.sample_rate == src_buffer.sample_rate);
    PLY_ASSERT(dst_buffer.format == src_buffer.format);
    PLY_ASSERT(dst_buffer.format.sample_type == Format::SampleType::S16);

    s16* dst = (s16*) dst_buffer.samples;
    s16* dst_end = (s16*) dst_buffer.get_end_ptr();
    s16* src = (s16*) src_buffer.samples;
    while (dst < dst_end) {
        *dst = saturating_add(*dst, (*src * level16) >> 16);
        dst++;
        src++;
    }
}

void mix_dyn_level(const Buffer& dst_buffer, const Buffer& src_buffer, u32 level30,
                   s32 step_level30) {
    PLY_ASSERT(dst_buffer.num_samples == src_buffer.num_samples);
    PLY_ASSERT(dst_buffer.sample_rate == src_buffer.sample_rate);
    PLY_ASSERT(dst_buffer.format == src_buffer.format);
    PLY_ASSERT(dst_buffer.format == Format::StereoS16);

    s16* dst = (s16*) dst_buffer.samples;
    s16* dst_end = (s16*) dst_buffer.get_end_ptr();
    s16* src = (s16*) src_buffer.samples;
    while (dst < dst_end) {
        *dst = saturating_add(*dst, (*src * (level30 >> 14)) >> 16);
        dst++;
        src++;
        *dst = saturating_add(*dst, (*src * (level30 >> 14)) >> 16);
        dst++;
        src++;
        level30 += step_level30;
    }
}

void mix_pitched_direct(const MixPitchedDirectParams& params) {
    struct Sample {
        s16 left;
        s16 right;
    };

    Sample* src = (Sample*) params.src;
    Sample* src_end = (Sample*) params.src_end;
    Sample* dst = (Sample*) params.dst;
    Sample* dst_end = (Sample*) params.dst_end;
    s32 sf16 = params.sf16;
    u32 sample_step = params.src_step;
    s32 volume16 = params.volume16;

    while (dst < dst_end) {
        PLY_ASSERT(src + 1 < src_end);
        s32 left_pre_volume = (src[0].left * (65536 - sf16) + src[1].left * sf16) >> 16;
        dst->left = saturating_add(dst->left, (left_pre_volume * volume16) >> 16);
        s32 right_pre_volume =
            (src[0].right * (65536 - sf16) + src[1].right * sf16) >> 16;
        dst->right = saturating_add(dst->right, (right_pre_volume * volume16) >> 16);
        dst++;
        sf16 += sample_step;
        src += sf16 >> 16;
        sf16 &= 65535;
    }

    // Bounds checks
    PLY_ASSERT(src + (s32(sf16 - sample_step) >> 16) < src_end - 1);
    PLY_ASSERT(src <= src_end + (sample_step >> 16));
    PLY_UNUSED(src_end);
}

} // namespace audio
} // namespace ply
