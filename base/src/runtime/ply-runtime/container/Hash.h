/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime.h>

namespace ply {

// Adapted from https://github.com/aappleby/smhasher
// Specifically https://raw.githubusercontent.com/aappleby/smhasher/master/src/PMurHash.c

struct Hasher {
    u32 accum = 0;

    static PLY_INLINE u32 finalize(u32 h) {
        h ^= h >> 16;
        h *= 0x85ebca6bu;
        h ^= h >> 13;
        h *= 0xc2b2ae35u;
        h ^= h >> 16;
        return h;
    }

    PLY_DLL_ENTRY u32 result() const;

    template <typename T>
    static PLY_INLINE u32 hash(const T& obj) {
        Hasher h;
        h << obj;
        return h.result();
    }

    // Special case hash functions
    static PLY_INLINE u32 hash(u64 obj) {
        return (u32) avalanche(obj);
    }
    static PLY_INLINE u32 hash(u32 obj) {
        return avalanche(obj);
    }
    static PLY_INLINE u32 hash(s64 obj) {
        return (u32) avalanche((u64) obj);
    }
    static PLY_INLINE u32 hash(s32 obj) {
        return avalanche((u32) obj);
    }
};

//------------------------------------------
// Built-in hash support
//------------------------------------------
PLY_DLL_ENTRY Hasher& operator<<(Hasher& hasher, u32 value);

PLY_INLINE Hasher& operator<<(Hasher& hasher, float value) {
#if PLY_COMPILER_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
    hasher << (value == 0 ? 0u : *reinterpret_cast<u32*>(&value));
#if PLY_COMPILER_GCC
#pragma GCC diagnostic pop
#endif
    return hasher;
}

PLY_INLINE Hasher& operator<<(Hasher& hasher, u64 value) {
    hasher << u32(value);
    hasher << u32(value >> 32);
    return hasher;
}

// For pointer to class or void*, use pointer value only.
// (Note: char* is not accepted here.)
template <typename T,
          std::enable_if_t<std::is_class<T>::value || std::is_same<T, void>::value, int> = 0>
PLY_INLINE Hasher& operator<<(Hasher& hasher, const T* value) {
    hasher << uptr(value);
    return hasher;
};

PLY_DLL_ENTRY Hasher& operator<<(Hasher& hasher, StringView buf);

} // namespace ply
