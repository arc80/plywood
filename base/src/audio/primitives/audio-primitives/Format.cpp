/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <audio-primitives/Core.h>
#include <audio-primitives/Format.h>

namespace ply {
namespace audio {

const Format Format::Null{0, Format::SampleType::S16, 0};
const Format Format::MonoS16{1, Format::SampleType::S16, 2};
const Format Format::StereoS16{2, Format::SampleType::S16, 4};

u16 Format::SampleTypeToStride[(u8) Format::SampleType::Num] = {(u16) sizeof(s16),
                                                                (u16) sizeof(float)};

} // namespace audio
} // namespace ply

#include "codegen/Format.inl" //%%
