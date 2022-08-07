/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {
namespace details {

struct LabelEncoder {
    static PLY_INLINE u32 decodeValue(const char*& ptr) {
        u32 value = 0;
        for (;;) {
            u8 c = *ptr;
            ptr++;
            value += (c & 127);
            if ((c >> 7) == 0)
                break;
            value <<= 7;
        }
        return value;
    }

    static PLY_INLINE u32 getEncLen(u32 value) {
        u32 encLen = 0;
        do {
            encLen++;
            value >>= 7;
        } while (value > 0);
        return encLen;
    }

    static PLY_NO_INLINE void encodeValue(char*& ptr, u32 value) {
        u32 shift = (getEncLen(value) - 1) * 7;
        for (;;) {
            *ptr = u8(((value >> shift) & 127) | ((shift != 0) << 7));
            ptr++;
            if (shift == 0)
                break;
            shift -= 7;
        }
    }
};

} // namespace details
} // namespace ply
