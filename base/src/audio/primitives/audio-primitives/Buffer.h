/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <audio-primitives/Core.h>
#include <audio-primitives/Format.h>

namespace ply {
namespace audio {

//-----------------------------------------------------------------------
/*! A `Buffer` just points to some multichannel sample data and describes its length and
   format. They're used to pass input into the render functions.

    A `Buffer` doesn't "own" the data it points to.
    You're meant to manage the data lifetime some other way.
    `Container` gives you one way to do that.
    Another way is to use the audioMixer library, which manages `ReloadableContainer`s
    using the AssetBank.
*/
struct Buffer {
    char* samples; //! Pointer to multichannel sample data
    PLY_REFLECT()
    u32 numSamples;   //! Number of multichannel samples in the data
    float sampleRate; //! Number of multichannel samples per second for realtime playback
    Format format;    //! Format of each multichannel sample
    // ply reflect off

    Buffer() {
    }
    Buffer(char* samples, u32 numSamples, float sampleRate, Format format)
        : samples(samples), numSamples(numSamples), sampleRate(sampleRate), format(format) {
    }

    //! Return the total size of the sample data in bytes.
    u32 getSizeBytes() const {
        return numSamples * format.stride;
    };

    //! Return a pointer to a specific multichannel sample.
    char* getSampleAt(u32 samplePos) const {
        return samples + samplePos * format.stride;
    }

    //! Return a pointer to the end of the sample data.
    char* getEndPtr() const {
        return samples + getSizeBytes();
    }

    //! Make a new `Buffer` structure that starts at a specific multichannel sample and has the
    //! given length.
    Buffer getSubregion(u32 offsetSamples, s32 length = -1) const {
        PLY_ASSERT(offsetSamples <= numSamples);
        Buffer result{samples + offsetSamples * format.stride, (u32) length, sampleRate, format};
        if (length < 0)
            result.numSamples = numSamples - offsetSamples;
        else
            PLY_ASSERT(offsetSamples + length <= numSamples);
        return result;
    }

    //! Zero the buffer.
    void zero() const {
        memset(samples, 0, numSamples * format.stride);
    }
};

} // namespace audio
} // namespace ply
