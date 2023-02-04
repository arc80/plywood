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

void mix(const Buffer& dstBuffer, const Buffer& srcBuffer) {
    PLY_ASSERT(dstBuffer.numSamples == srcBuffer.numSamples);
    PLY_ASSERT(dstBuffer.sampleRate == srcBuffer.sampleRate);
    PLY_ASSERT(dstBuffer.format == srcBuffer.format);
    PLY_ASSERT(dstBuffer.format.sampleType == Format::SampleType::S16);

    s16* dst = (s16*) dstBuffer.samples;
    s16* dstEnd = (s16*) dstBuffer.getEndPtr();
    s16* src = (s16*) srcBuffer.samples;
    while (dst < dstEnd) {
        *dst = saturatingAdd(*dst, *src);
        dst++;
        src++;
    }
}

void mixLeveled(const Buffer& dstBuffer, const Buffer& srcBuffer, s32 level16) {
    PLY_ASSERT(dstBuffer.numSamples == srcBuffer.numSamples);
    PLY_ASSERT(dstBuffer.sampleRate == srcBuffer.sampleRate);
    PLY_ASSERT(dstBuffer.format == srcBuffer.format);
    PLY_ASSERT(dstBuffer.format.sampleType == Format::SampleType::S16);

    s16* dst = (s16*) dstBuffer.samples;
    s16* dstEnd = (s16*) dstBuffer.getEndPtr();
    s16* src = (s16*) srcBuffer.samples;
    while (dst < dstEnd) {
        *dst = saturatingAdd(*dst, (*src * level16) >> 16);
        dst++;
        src++;
    }
}

void mixDynLevel(const Buffer& dstBuffer, const Buffer& srcBuffer, u32 level30, s32 stepLevel30) {
    PLY_ASSERT(dstBuffer.numSamples == srcBuffer.numSamples);
    PLY_ASSERT(dstBuffer.sampleRate == srcBuffer.sampleRate);
    PLY_ASSERT(dstBuffer.format == srcBuffer.format);
    PLY_ASSERT(dstBuffer.format == Format::StereoS16);

    s16* dst = (s16*) dstBuffer.samples;
    s16* dstEnd = (s16*) dstBuffer.getEndPtr();
    s16* src = (s16*) srcBuffer.samples;
    while (dst < dstEnd) {
        *dst = saturatingAdd(*dst, (*src * (level30 >> 14)) >> 16);
        dst++;
        src++;
        *dst = saturatingAdd(*dst, (*src * (level30 >> 14)) >> 16);
        dst++;
        src++;
        level30 += stepLevel30;
    }
}

void mixPitchedDirect(const MixPitchedDirectParams& params) {
    struct Sample {
        s16 left;
        s16 right;
    };

    Sample* src = (Sample*) params.src;
    Sample* srcEnd = (Sample*) params.srcEnd;
    Sample* dst = (Sample*) params.dst;
    Sample* dstEnd = (Sample*) params.dstEnd;
    s32 sf16 = params.sf16;
    u32 sampleStep = params.srcStep;
    s32 volume16 = params.volume16;

    while (dst < dstEnd) {
        PLY_ASSERT(src + 1 < srcEnd);
        s32 leftPreVolume = (src[0].left * (65536 - sf16) + src[1].left * sf16) >> 16;
        dst->left = saturatingAdd(dst->left, (leftPreVolume * volume16) >> 16);
        s32 rightPreVolume = (src[0].right * (65536 - sf16) + src[1].right * sf16) >> 16;
        dst->right = saturatingAdd(dst->right, (rightPreVolume * volume16) >> 16);
        dst++;
        sf16 += sampleStep;
        src += sf16 >> 16;
        sf16 &= 65535;
    }

    // Bounds checks
    PLY_ASSERT(src + (s32(sf16 - sampleStep) >> 16) < srcEnd - 1);
    PLY_ASSERT(src <= srcEnd + (sampleStep >> 16));
    PLY_UNUSED(srcEnd);
}

} // namespace audio
} // namespace ply
