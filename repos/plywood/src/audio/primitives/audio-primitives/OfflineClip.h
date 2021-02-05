/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
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

    static OfflineClipNode* create(u32 numSamples, float sampleRate, Format format) {
        OfflineClipNode* node = new OfflineClipNode;
        node->samples.resize(numSamples * format.stride);
        node->buffer = Buffer{node->samples.bytes, numSamples, sampleRate, format};
        return node;
    }

    void onPostSerialize() {
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
