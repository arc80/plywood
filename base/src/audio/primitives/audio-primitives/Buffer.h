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
    Another way is to use the audio_mixer library, which manages `ReloadableContainer`s
    using the AssetBank.
*/
struct Buffer {
    char* samples; //! Pointer to multichannel sample data
    PLY_REFLECT()
    u32 num_samples; //! Number of multichannel samples in the data
    float
        sample_rate; //! Number of multichannel samples per second for realtime playback
    Format format;   //! Format of each multichannel sample
    // ply reflect off

    Buffer() {
    }
    Buffer(char* samples, u32 num_samples, float sample_rate, Format format)
        : samples(samples), num_samples(num_samples), sample_rate(sample_rate),
          format(format) {
    }

    //! Return the total size of the sample data in bytes.
    u32 get_size_bytes() const {
        return num_samples * format.stride;
    };

    //! Return a pointer to a specific multichannel sample.
    char* get_sample_at(u32 sample_pos) const {
        return samples + sample_pos * format.stride;
    }

    //! Return a pointer to the end of the sample data.
    char* get_end_ptr() const {
        return samples + get_size_bytes();
    }

    //! Make a new `Buffer` structure that starts at a specific multichannel sample and
    //! has the given length.
    Buffer get_subregion(u32 offset_samples, s32 length = -1) const {
        PLY_ASSERT(offset_samples <= num_samples);
        Buffer result{samples + offset_samples * format.stride, (u32) length,
                      sample_rate, format};
        if (length < 0)
            result.num_samples = num_samples - offset_samples;
        else
            PLY_ASSERT(offset_samples + length <= num_samples);
        return result;
    }

    //! Zero the buffer.
    void zero() const {
        memset(samples, 0, num_samples * format.stride);
    }
};

} // namespace audio
} // namespace ply
