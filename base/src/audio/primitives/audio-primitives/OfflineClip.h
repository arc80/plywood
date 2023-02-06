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

struct OfflineClipNode {
    PLY_REFLECT()
    Buffer buffer;
    String samples;
    // ply reflect off

    static OfflineClipNode* create(u32 num_samples, float sample_rate, Format format) {
        OfflineClipNode* node = new OfflineClipNode;
        node->samples.resize(num_samples * format.stride);
        node->buffer = Buffer{node->samples.bytes, num_samples, sample_rate, format};
        return node;
    }

    void on_post_serialize() {
        // When object is loaded, force buffer.samples to point to the loaded Buffer:
        buffer.samples = samples.bytes;
    }
};

struct OfflineClip {
    PLY_REFLECT()
    Owned<OfflineClipNode> node;
    // ply reflect off
};

} // namespace audio
} // namespace ply
