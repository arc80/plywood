/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
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

inline bool isPowerOf2(ureg v) {
    return (v & (v - 1)) == 0;
}

inline u32 alignPowerOf2(u32 v, u32 a) {
    PLY_ASSERT(isPowerOf2(a));
    return (v + a - 1) & ~(a - 1);
}

inline u64 alignPowerOf2(u64 v, u64 a) {
    PLY_ASSERT(isPowerOf2(a));
    return (v + a - 1) & ~(a - 1);
}

inline bool isAlignedPowerOf2(ureg v, ureg a) {
    PLY_ASSERT(isPowerOf2(a));
    return (v & (a - 1)) == 0;
}

inline u32 roundUpPowerOf2(u32 v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

inline s32 roundUpPowerOf2(s32 v) {
    return (s32) roundUpPowerOf2((u32) v);
}

inline u64 roundUpPowerOf2(u64 v) {
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

inline s64 roundUpPowerOf2(s64 v) {
    return (s64) roundUpPowerOf2((u64) v);
}

inline ureg countSetBits(u64 mask) {
    ureg count = 0;
    while (mask) {
        count += (mask & 1);
        mask >>= 1;
    }
    return count;
}

template <typename Dst, typename Src>
PLY_INLINE std::enable_if_t<std::is_unsigned<Src>::value, Dst> safeDemote(Src src) {
    // src is unsigned
    PLY_ASSERT(src <= (typename std::make_unsigned_t<Dst>) Limits<Dst>::Max);
    return (Dst) src;
}

template <typename Dst, typename Src>
PLY_INLINE std::enable_if_t<std::is_signed<Src>::value && std::is_signed<Dst>::value, Dst>
safeDemote(Src src) {
    // src and dst are both signed
    PLY_ASSERT(src >= Limits<Dst>::Min);
    PLY_ASSERT(src <= Limits<Dst>::Max);
    return (Dst) src;
}

template <typename Dst, typename Src>
PLY_INLINE std::enable_if_t<std::is_signed<Src>::value && std::is_unsigned<Dst>::value, Dst>
safeDemote(Src src) {
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

template <typename... Ts>
struct make_void {
    typedef void type;
};
template <typename... Ts>
using void_t = typename make_void<Ts...>::type;

template <typename T>
PLY_INLINE void destruct(T& obj) {
    obj.~T();
}

} // namespace ply
