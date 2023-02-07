/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime.h>
#include <ply-platform/Util.h>

namespace ply {

Random::Random() {
    // Seed using misc. information from the environment
    u64 t = DateTime::get_current_epoch_microseconds();
    t = avalanche(t);
    s[0] = avalanche(t | 1);

    t = u64(CPUTimer::get() - CPUTimer::Point{0});
    t = avalanche(t) + (avalanche(get_current_thread_id()) << 1);
    s[1] = avalanche(t | 1);

    for (ureg i = 0; i < 10; i++)
        next64();
}

Random::Random(u64 seed) {
    s[0] = avalanche(seed + 1);
    s[1] = avalanche(s[0] + 1);
    next64();
    next64();
}

static inline u64 rotl(const u64 x, int k) {
    return (x << k) | (x >> (64 - k));
}

u64 Random::next64() {
    const u64 s0 = s[0];
    u64 s1 = s[1];
    const u64 result = rotl(s0 * 5, 7) * 9;

    s1 ^= s0;
    s[0] = rotl(s0, 24) ^ s1 ^ (s1 << 16); // a, b
    s[1] = rotl(s1, 37);                   // c

    return result;
}

} // namespace ply
