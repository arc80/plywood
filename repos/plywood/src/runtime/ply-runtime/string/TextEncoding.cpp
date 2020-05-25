/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/string/TextEncoding.h>

namespace ply {

//-------------------------------------------------------------------
// UTF8
//-------------------------------------------------------------------
PLY_NO_INLINE DecodeResult UTF8::decodePointSlowPath(ConstBufferView view) {
    if (view.numBytes == 0) {
        return {};
    }

    DecodeResult result;
    u32 value = 0;
    const u8* bytes = view.bytes;
    u8 first = *bytes++;
    switch ((first >> 3) & 0xf) {
        case 0b1000:
        case 0b1001:
        case 0b1010:
        case 0b1011: {
            if (view.numBytes >= 2) {
                if ((bytes[0] & 0xc0) == 0x80) {
                    result.numBytes = 2;
                    value = first & 0x1f;
                    goto consume1Byte;
                }
            }
            break;
        }

        case 0b1100:
        case 0b1101: {
            if (view.numBytes >= 3) {
                if ((bytes[0] & 0xc0) == 0x80 && (bytes[1] & 0xc0) == 0x80) {
                    result.numBytes = 3;
                    value = first & 0xf;
                    goto consume2Bytes;
                }
            }
            break;
        }

        case 0b1110: {
            if (view.numBytes >= 4) {
                if ((bytes[0] & 0xc0) == 0x80 && (bytes[1] & 0xc0) == 0x80 &&
                    (bytes[2] & 0xc0) == 0x80) {
                    result.numBytes = 4;
                    value = first & 0x7;
                    goto consume3Bytes;
                }
            }
            break;
        }

        default:
            break;
    }

    // Bad encoding; consume just one byte
    result.point = first;
    result.validEncoding = false;
    result.numBytes = 1;
    return result;

consume3Bytes:
    value = (value << 6) | (*bytes & 0x3f);
    bytes++;
consume2Bytes:
    value = (value << 6) | (*bytes & 0x3f);
    bytes++;
consume1Byte:
    value = (value << 6) | (*bytes & 0x3f);
    result.point = value;
    result.validEncoding = true;
    return result;
}

PLY_NO_INLINE u32 UTF8::backNumBytesSlowPath(ConstBufferView view) {
    if (view.numBytes == 0) {
        return 0;
    }
    const u8* tail = (const u8*) view.bytes + view.numBytes;
    if ((tail[-1] & 0xc0) == 0x80 && view.numBytes >= 2) {
        if ((tail[-2] & 0xc0) == 0x80 && view.numBytes >= 3) {
            if ((tail[-3] & 0xc0) == 0x80 && view.numBytes >= 4) {
                if ((tail[-4] & 0xf8) == 0xf0)
                    return 4;
            } else {
                if ((tail[-3] & 0xf0) == 0xe0)
                    return 3;
            }
        } else {
            if ((tail[-2] & 0xe0) == 0xc0)
                return 2;
        }
    }
    return 1;
}

//-------------------------------------------------------------------
// TextEncoding (indirect through function vectors)
//-------------------------------------------------------------------
TextEncoding TextEncoding::Wrapper<Enc_Bytes>::Instance = {
    &Enc_Bytes::decodePoint,
    &Enc_Bytes::encodePoint,
    1,
};

TextEncoding TextEncoding::Wrapper<UTF8>::Instance = {
    &UTF8::decodePoint,
    &UTF8::encodePoint,
    1,
};

TextEncoding TextEncoding::Wrapper<UTF16_LE>::Instance = {
    &UTF16_LE::decodePoint,
    &UTF16_LE::encodePoint,
    2,
};

TextEncoding TextEncoding::Wrapper<UTF16_BE>::Instance = {
    &UTF16_BE::decodePoint,
    &UTF16_BE::encodePoint,
    2,
};

} // namespace ply
