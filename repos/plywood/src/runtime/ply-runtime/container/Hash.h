/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

// Adapted from https://github.com/aappleby/smhasher
// Specifically https://raw.githubusercontent.com/aappleby/smhasher/master/src/PMurHash.c

class Hasher {
private:
    u32 m_accum = 0;

public:
    static PLY_INLINE u32 finalize(u32 h) {
        h ^= h >> 16;
        h *= 0x85ebca6bu;
        h ^= h >> 13;
        h *= 0xc2b2ae35u;
        h ^= h >> 16;
        return h;
    }

    PLY_DLL_ENTRY void append(u32 value);

    PLY_INLINE void append(float value) {
#if PLY_COMPILER_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
        append(value == 0 ? (u32) 0 : *(u32*) &value);
#if PLY_COMPILER_GCC
#pragma GCC diagnostic pop
#endif
    }

    PLY_INLINE void append(u64 value) {
        append(u32(value));
        append(u32(value >> 32));
    }

    PLY_INLINE void appendPtr(const void* ptr) {
        append(uptr(ptr));
    }

    PLY_DLL_ENTRY void appendBuffer(const void* data, u32 len);

    PLY_DLL_ENTRY u32 result() const;
};

} // namespace ply
