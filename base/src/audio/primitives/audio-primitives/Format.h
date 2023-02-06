/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <audio-primitives/Core.h>

namespace ply {
namespace audio {

//-----------------------------------------------------------------------
/*! Describes the number of channels and format of each multichannel sample in a
 * `Buffer`.
 */
struct Format {
    enum class SampleType : u8 { //! Valid values of `sample_type`
        // ply reflect enum
        S16 = 0,
        Float,
        Num
    };
    static u16 SampleTypeToStride[];

    PLY_REFLECT()
    u8 num_channels;        //! 1 for mono, 2 for stereo
    SampleType sample_type; //! `Format::SampleType::S16`, `Format::SampleType::Float`
    u16 stride; //! Number of bytes for each multichannel sample (num_channels * sizeof
                //! the basic type)
    // ply reflect off

    bool operator==(const Format& other) const {
        return (const u32&) *this == (const u32&) other;
    }

    static Format encode(u8 num_channels, SampleType sample_type) {
        PLY_ASSERT((u8) sample_type < (u8) SampleType::Num);
        return Format{num_channels, sample_type,
                      u16(SampleTypeToStride[(u8) sample_type] * num_channels)};
    }

    static const Format Null;
    static const Format MonoS16;
    static const Format StereoS16;

    void on_post_serialize() {
        PLY_ASSERT((u8) sample_type < (u8) SampleType::Num);
        stride = u16(SampleTypeToStride[(u8) sample_type] * num_channels);
    }
};

} // namespace audio

PLY_DECLARE_TYPE_DESCRIPTOR(audio::Format::SampleType)

} // namespace ply
