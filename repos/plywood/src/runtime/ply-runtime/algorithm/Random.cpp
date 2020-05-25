/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/algorithm/Random.h>
#include <ply-runtime/time/UTCTime.h>
#include <ply-runtime/time/CPUTimer.h>
#include <ply-platform/Util.h>
#include <ply-runtime/thread/TID.h>

namespace ply {

PLY_NO_INLINE Random::Random() {
    // Seed using misc. information from the environment
    u64 t = getCurrentUTCTime();
    t = avalanche(t);
    s[0] = avalanche(t | 1);

    t = u64(CPUTimer::get() - CPUTimer::Point{0});
    t = avalanche(t) + (avalanche(TID::getCurrentThreadID()) << 1);
    s[1] = avalanche(t | 1);

    for (ureg i = 0; i < 10; i++)
        next64();
}

PLY_NO_INLINE Random::Random(u64 seed) {
    s[0] = avalanche(seed + 1);
    s[1] = avalanche(s[0] + 1);
    next64();
    next64();
}

PLY_NO_INLINE u64 Random::next64() {
    u64 s1 = s[0];
    const u64 s0 = s[1];
    s[0] = s0;
    s1 ^= s1 << 23;                                           // a
    return (s[1] = (s1 ^ s0 ^ (s1 >> 17) ^ (s0 >> 26))) + s0; // b, c
}

} // namespace ply
