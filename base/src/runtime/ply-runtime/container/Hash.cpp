/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>

namespace ply {

PLY_NO_INLINE u32 Hasher::result() const {
    return finalize(this->accum);
}

PLY_NO_INLINE Hasher& operator<<(Hasher& hasher, u32 value) {
    value *= 0xcc9e2d51u;
    value = (value << 15) | (value >> 17);
    value *= 0x1b873593u;
    hasher.accum ^= value;
    hasher.accum = (hasher.accum << 13) | (hasher.accum >> 19);
    hasher.accum = hasher.accum * 5 + 0xe6546b64u;
    return hasher;
}

PLY_NO_INLINE Hasher& operator<<(Hasher& hasher, StringView buf) {
    // FIXME: More work is needed for platforms that don't support unaligned reads

    while (buf.numBytes >= 4) {
        hasher << *(const u32*) buf.bytes;  // May be unaligned
        buf.bytes += 4;
        buf.numBytes -= 4;
    }
    if (buf.numBytes > 0) {
        // Avoid potential unaligned read across page boundary
        u32 v = 0;
        while (buf.numBytes > 0) {
            v = (v << 8) | *(const u8*) buf.bytes;
            buf.bytes++;
            buf.numBytes--;
        }
        hasher << v;
    }
    return hasher;
}

} // namespace ply
