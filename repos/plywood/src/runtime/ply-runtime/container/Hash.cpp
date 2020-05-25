/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/container/Hash.h>

namespace ply {

PLY_NO_INLINE void Hasher::append(u32 value) {
    value *= 0xcc9e2d51u;
    value = (value << 15) | (value >> 17);
    value *= 0x1b873593u;
    m_accum ^= value;
    m_accum = (m_accum << 13) | (m_accum >> 19);
    m_accum = m_accum * 5 + 0xe6546b64u;
}

PLY_NO_INLINE u32 Hasher::result() const {
    return finalize(m_accum);
}

PLY_NO_INLINE void Hasher::appendBuffer(const void* data, u32 len) {
    // Note: This may need to be adapted for platforms that don't support unaligned reads
    while (len >= 4) {
        append(*(const u32*) data);
        data = (const void*) PLY_PTR_OFFSET(data, 4);
        len -= 4;
    }
    if (len > 0) {
        u32 v = 0;
        while (len > 0) {
            v = (v << 8) | *(const u8*) data;
            data = (const void*) PLY_PTR_OFFSET(data, 1);
            len--;
        }
        append(len);
    }
}

} // namespace ply
