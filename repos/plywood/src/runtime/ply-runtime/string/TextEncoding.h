/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/ArrayView.h>

namespace ply {

struct String;
struct WString;

struct DecodeResult {
    s32 point = -1;
    bool validEncoding = false;
    u8 numBytes = 0;
};

//-------------------------------------------------------------------
// Enc_Bytes
//-------------------------------------------------------------------
struct Enc_Bytes {
    using Container = String;

    static PLY_INLINE DecodeResult decodePoint(ConstBufferView view) {
        if (view.numBytes == 0) {
            return {};
        } else {
            return {(u8) view.bytes[0], true, 1};
        }
    }

    static PLY_INLINE u32 backNumBytes(ConstBufferView view) {
        if (view.numBytes == 0) {
            return 0;
        } else {
            return 1;
        }
    }

    static PLY_INLINE u32 numBytes(u32 point) {
        return 1;
    }

    static PLY_INLINE u32 encodePoint(BufferView view, u32 point) {
        if (view.numBytes > 0) {
            view[0] = point < 256 ? (u8) point : 0x95;
            return 1;
        }
        PLY_ASSERT(false); // Passed buffer was too small
        return 0;
    }
};

//-------------------------------------------------------------------
// UTF8
//-------------------------------------------------------------------
struct UTF8 {
    typedef u32 BadUnitHandler(u8 badUnit);
    using Container = String;

    static PLY_DLL_ENTRY DecodeResult decodePointSlowPath(ConstBufferView view);

    static PLY_INLINE DecodeResult decodePoint(ConstBufferView view) {
        if (view.numBytes > 0) {
            u8 first = view.bytes[0];
            if (first < 0x80) {
                return {first, true, 1};
            }
        }
        return decodePointSlowPath(view);
    }

    static PLY_DLL_ENTRY u32 backNumBytesSlowPath(ConstBufferView view);

    static PLY_INLINE u32 backNumBytes(ConstBufferView view) {
        if (view.numBytes > 0) {
            u8 last = view.bytes[view.numBytes - 1];
            if (last < 0x80) {
                return 1;
            }
        }
        return backNumBytesSlowPath(view);
    }

    static PLY_INLINE u32 numBytes(u32 point) {
        if (point < 0x80)
            return 1;
        else if (point < 0x800)
            return 2;
        else if (point < 0x10000)
            return 3;
        else
            return 4;
    }

    static PLY_INLINE u32 encodePoint(BufferView view, u32 point) {
        if (point < 0x80) {
            if (view.numBytes >= 1) {
                view.bytes[0] = u8(point);
                return 1;
            }
        } else if (point < 0x800) {
            if (view.numBytes >= 2) {
                view.bytes[0] = u8(0xc0 | (point >> 6));
                view.bytes[1] = u8(0x80 | (point & 0x3f));
                return 2;
            }
        } else if (point < 0x10000) {
            if (view.numBytes >= 3) {
                view.bytes[0] = u8(0xe0 | (point >> 12));
                view.bytes[1] = u8(0x80 | ((point >> 6) & 0x3f));
                view.bytes[2] = u8(0x80 | ((point & 0x3f)));
                return 3;
            }
        } else {
            if (view.numBytes >= 4) {
                view.bytes[0] = u8(0xf0 | (point >> 18));
                view.bytes[1] = u8(0x80 | ((point >> 12) & 0x3f));
                view.bytes[2] = u8(0x80 | ((point >> 6) & 0x3f));
                view.bytes[3] = u8(0x80 | (point & 0x3f));
                return 4;
            }
        }
        PLY_ASSERT(false); // Passed buffer was too small
        return 0;
    }
};

//-------------------------------------------------------------------
// UTF16
//-------------------------------------------------------------------
template <bool BigEndian>
struct UTF16 {
    using Container = WString;

    static PLY_INLINE char16_t getUnit(const u8* src) {
        if (BigEndian) {
            return (char16_t(src[0]) << 8) | src[1];
        } else {
            return (src[0] | char16_t(src[1]) << 8);
        }
    }

    static PLY_INLINE void putUnit(u8* src, char16_t u) {
        if (BigEndian) {
            src[0] = u8(u >> 8);
            src[1] = u8(u);
        } else {
            src[0] = u8(u);
            src[1] = u8(u >> 8);
        }
    }

    static PLY_INLINE DecodeResult decodePoint(ConstBufferView view) {
        if (view.numBytes < 2) {
            return {};
        }
        u16 first = getUnit(view.bytes);
        if (view.numBytes >= 4 && first >= 0xd800 && first < 0xdc00) {
            u16 second = getUnit(view.bytes + 2);
            if (second >= 0xdc00 && second < 0xe000) {
                u32 value = 0x10000 + ((first - 0xd800) << 10) + (second - 0xdc00);
                return {(s32) value, true, 4};
            }
        }
        return {first, true, 2};
    }

    static PLY_INLINE u32 backNumBytes(ConstBufferView view) {
        if (view.numBytes < 2) {
            return 0;
        }
        const u8* tail = (const u8*) view.bytes + view.numBytes;
        if (view.numBytes >= 4) {
            u16 first = getUnit(tail - 4);
            u16 second = getUnit(tail - 2);
            if (first >= 0xd800 && first < 0xdc00 && second >= 0xdc00 && second < 0xe000)
                return 4;
        }
        return 2;
    }

    static PLY_INLINE u32 numBytes(u32 point) {
        if (point < 0x10000)
            return 2;
        else
            return 4;
    }

    static PLY_INLINE u32 encodePoint(BufferView view, u32 point) {
        if (point < 0x10000) {
            if (view.numBytes >= 2) {
                putUnit(view.bytes, u16(point));
                return 2;
            }
        } else {
            if (view.numBytes >= 4) {
                point -= 0x10000;
                putUnit(view.bytes, u16(0xd800 + ((point >> 10) & 0x3ff)));
                putUnit(view.bytes + 2, u16(0xdc00 + (point & 0x3ff)));
                return 4;
            }
        }
        PLY_ASSERT(false); // Passed buffer was too small
        return 0;
    }
};

using UTF16_LE = UTF16<false>;
using UTF16_BE = UTF16<true>;
using UTF16_Native = UTF16<PLY_IS_BIG_ENDIAN>;

//-------------------------------------------------------------------
// TextEncoding helper objects
//-------------------------------------------------------------------
struct TextEncoding {
    DecodeResult (*decodePoint)(ConstBufferView view) = nullptr;
    u32 (*encodePoint)(BufferView view, u32 point) = nullptr;
    u32 unitSize = 0;

    template <typename>
    struct Wrapper;
    template <typename Enc>
    PLY_INLINE static const TextEncoding* get() {
        return &TextEncoding::Wrapper<Enc>::Instance;
    }
};

template <>
struct TextEncoding::Wrapper<Enc_Bytes> {
    static PLY_DLL_ENTRY TextEncoding Instance;
};
template <>
struct TextEncoding::Wrapper<UTF8> {
    static PLY_DLL_ENTRY TextEncoding Instance;
};
template <>
struct TextEncoding::Wrapper<UTF16_LE> {
    static PLY_DLL_ENTRY TextEncoding Instance;
};
template <>
struct TextEncoding::Wrapper<UTF16_BE> {
    static PLY_DLL_ENTRY TextEncoding Instance;
};

} // namespace ply
