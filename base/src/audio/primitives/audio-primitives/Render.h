/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <audio-primitives/Core.h>
#include <audio-primitives/Buffer.h>

namespace ply {
namespace audio {

struct MixPitchedDirectParams {
    char* dst;
    char* dst_end;
    char* src;
    char* src_end;
    u32 sf16;
    u32 src_step;
    s32 volume16;
};

struct MixLeveledDirectParams {
    char* dst;
    char* dst_end;
    char* src;
    s32 level16;
};

void mix(const Buffer& dst_buffer, const Buffer& src_buffer);
void mix_leveled(const Buffer& dst_buffer, const Buffer& src_buffer, s32 level16);
void mix_dyn_level(const Buffer& dst_buffer, const Buffer& src_buffer, u32 level30,
                   s32 step_level30);
void mix_pitched_direct(const MixPitchedDirectParams& params);

} // namespace audio
} // namespace ply
