/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <audio-primitives/Core.h>

namespace ply {
namespace audio {

#if WIN32
    #include <emmintrin.h>
inline s16 saturatingAdd(s16 a, s16 b) {
    __m128i mma = _mm_cvtsi32_si128(a);
    __m128i mmb = _mm_cvtsi32_si128(b);
    __m128i r = _mm_adds_epi16(mma, mmb);
    return (s16) _mm_cvtsi128_si32(r);
}
#else
// FIXME: Implement on other platforms
inline s16 saturatingAdd(s16 a, s16 b) {
    s32 sum = s32(a) + s32(b);
    return s16(sum >= 32768 ? 32767 : sum < -32768 ? -32768 : sum);
}
#endif

} // namespace audio
} // namespace ply
