/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <audio-primitives/Core.h>

namespace ply {
namespace audio {

//-----------------------------------------------------------------------
/*! Describes the number of channels and format of each multichannel sample in a `Buffer`.
 */
struct Format {
    enum class SampleType : u8 { //! Valid values of `sampleType`
        // ply reflect enum
        S16 = 0,
        Float,
        Num
    };
    static u16 SampleTypeToStride[];

    PLY_REFLECT()
    u8 numChannels;        //! 1 for mono, 2 for stereo
    SampleType sampleType; //! `Format::SampleType::S16`, `Format::SampleType::Float`
    u16 stride; //! Number of bytes for each multichannel sample (numChannels * sizeof the
                //! basic type)
    // ply reflect off

    bool operator==(const Format& other) const {
        return (const u32&) *this == (const u32&) other;
    }

    static Format encode(u8 numChannels, SampleType sampleType) {
        PLY_ASSERT((u8) sampleType < (u8) SampleType::Num);
        return Format{numChannels, sampleType,
                      u16(SampleTypeToStride[(u8) sampleType] * numChannels)};
    }

    static const Format Null;
    static const Format MonoS16;
    static const Format StereoS16;

    void onPostSerialize() {
        PLY_ASSERT((u8) sampleType < (u8) SampleType::Num);
        stride = u16(SampleTypeToStride[(u8) sampleType] * numChannels);
    }
};

PLY_REFLECT_ENUM(, audio::Format::SampleType)

} // namespace audio
} // namespace ply
