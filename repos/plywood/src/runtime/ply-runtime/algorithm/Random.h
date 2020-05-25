/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

//-------------------------------------
//  xorshift128+ generator seeded using misc. information from the environment.
//  Based on http://xorshift.di.unimi.it/
//-------------------------------------
class Random {
private:
    u64 s[2];

public:
    PLY_DLL_ENTRY Random();
    PLY_DLL_ENTRY Random(u64 seed); // Explicit seed
    PLY_DLL_ENTRY u64 next64();
    PLY_INLINE u32 next32() {
        return (u32) next64();
    }
    PLY_INLINE u16 next16() {
        return (u16) next64();
    }
    PLY_INLINE u8 next8() {
        return (u8) next64();
    }
    PLY_INLINE float nextFloat() {
        return next32() / 4294967295.f;
    }
};

} // namespace ply
