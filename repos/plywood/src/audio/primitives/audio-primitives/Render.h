/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <audio-primitives/Core.h>
#include <audio-primitives/Buffer.h>

namespace ply {
namespace audio {

struct MixPitchedDirectParams {
    u8* dst;
    u8* dstEnd;
    u8* src;
    u8* srcEnd;
    u32 sf16;
    u32 srcStep;
    s32 volume16;
};

struct MixLeveledDirectParams {
    u8* dst;
    u8* dstEnd;
    u8* src;
    s32 level16;
};

void mix(const Buffer& dstBuffer, const Buffer& srcBuffer);
void mixLeveled(const Buffer& dstBuffer, const Buffer& srcBuffer, s32 level16);
void mixDynLevel(const Buffer& dstBuffer, const Buffer& srcBuffer, u32 level30, s32 stepLevel30);
void mixPitchedDirect(const MixPitchedDirectParams& params);

} // namespace audio
} // namespace ply
