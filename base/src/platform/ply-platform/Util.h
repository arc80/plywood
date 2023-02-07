/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

template <u32>
struct SizedInt;
template <>
struct SizedInt<4> {
    using Unsigned = u32;
    using Signed = s32;
};
template <>
struct SizedInt<8> {
    using Unsigned = u64;
    using Signed = s64;
};

template <typename I>
constexpr PLY_INLINE std::enable_if_t<std::is_integral<I>::value, bool>
is_power_of2(I v) {
    return (v & (v - 1)) == 0;
}

template <typename I0, typename I1>
PLY_INLINE
    std::enable_if_t<std::is_integral<I0>::value && std::is_integral<I1>::value, I0>
    align_power_of2(I0 v, I1 a) {
    PLY_ASSERT(is_power_of2(a));
    return (v + a - 1) & ~I0(a - 1);
}

template <typename I0, typename I1>
PLY_INLINE
    std::enable_if_t<std::is_integral<I0>::value && std::is_integral<I1>::value, bool>
    is_aligned_power_of2(I0 v, I1 a) {
    PLY_ASSERT(is_power_of2(a));
    return (v & (a - 1)) == 0;
}

template <typename I>
PLY_INLINE std::enable_if_t<std::is_integral<I>::value && sizeof(I) == 8, I>
round_up_power_of2(I v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    v++;
    return v;
}

template <typename I>
PLY_INLINE std::enable_if_t<std::is_integral<I>::value && sizeof(I) <= 4, I>
round_up_power_of2(I v_) {
    u32 v = (u32) v_ - 1;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return (I) v;
}

template <typename I>
PLY_INLINE std::enable_if_t<std::is_integral<I>::value, u32> count_set_bits(I mask) {
    u32 count = 0;
    while (mask) {
        count += (mask & 1);
        mask >>= 1;
    }
    return count;
}

template <typename Dst, typename Src>
PLY_INLINE constexpr std::enable_if_t<std::is_unsigned<Src>::value, Dst>
check_cast(Src src) {
    // src is unsigned
    PLY_ASSERT(src <= (typename std::make_unsigned_t<Dst>) Limits<Dst>::Max);
    return (Dst) src;
}

template <typename Dst, typename Src>
PLY_INLINE constexpr std::enable_if_t<
    std::is_signed<Src>::value && std::is_signed<Dst>::value, Dst>
check_cast(Src src) {
    // src and dst are both signed
    PLY_ASSERT(src >= Limits<Dst>::Min);
    PLY_ASSERT(src <= Limits<Dst>::Max);
    return (Dst) src;
}

template <typename Dst, typename Src>
PLY_INLINE constexpr std::enable_if_t<
    std::is_signed<Src>::value && std::is_unsigned<Dst>::value, Dst>
check_cast(Src src) {
    // src is signed, dst is unsigned
    PLY_ASSERT(src >= 0);
    PLY_ASSERT((typename std::make_unsigned_t<Src>) src <= Limits<Dst>::Max);
    return (Dst) src;
}

// FIXME: Move to Hash.h
// from code.google.com/p/smhasher/wiki/MurmurHash3
inline u32 avalanche(u32 h) {
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

inline u32 deavalanche(u32 h) {
    h ^= h >> 16;
    h *= 0x7ed1b41d;
    h ^= (h ^ (h >> 13)) >> 13;
    h *= 0xa5cb9243;
    h ^= h >> 16;
    return h;
}

// from code.google.com/p/smhasher/wiki/MurmurHash3
inline u64 avalanche(u64 h) {
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccd;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53;
    h ^= h >> 33;
    return h;
}

inline u64 deavalanche(u64 h) {
    h ^= h >> 33;
    h *= 0x9cb4b2f8129337db;
    h ^= h >> 33;
    h *= 0x4f74430c22a54005;
    h ^= h >> 33;
    return h;
}

template <typename T>
PLY_INLINE void destruct(T& obj) {
    obj.~T();
}

} // namespace ply
