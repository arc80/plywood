/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/string/StringView.h>
#include <ply-runtime/io/InStream.h>
#include <ply-runtime/io/OutStream.h>
#include <ply-runtime/container/HashMap.h>

namespace ply {

// ┏━━━━━━━━━━━┓
// ┃  Unicode  ┃
// ┗━━━━━━━━━━━┛
enum UnicodeType {
    NotUnicode,
    UTF8,
    UTF16_Native,
    UTF16_Reversed,
#if PLY_IS_BIG_ENDIAN
    UTF16_LE = UTF16_Reversed,
    UTF16_BE = UTF16_Native,
#else
    UTF16_LE = UTF16_Native,
    UTF16_BE = UTF16_Reversed,
#endif
};

struct ExtendedTextParams {
    struct ReverseLUTTraits {
        using Key = u32;
        struct Item {
            u32 key;
            u8 value;
        };
        static bool match(const Item& item, Key key) {
            return item.key == key;
        }
    };

    ArrayView<s32> lut; // Lookup table: byte -> Unicode codepoint.
    HashMap<ReverseLUTTraits> reverse_lut;
    s32 missing_char = 255; // If negative, missing characters are skipped.
};

enum DecodeStatus {
    DS_OK,
    DS_IllFormed, // Not at EOF.
    DS_NotEnoughData, // Can still decode an ill-formed codepoint.
};

struct Unicode {
    UnicodeType type;
    ExtendedTextParams* ext_params = nullptr;
    DecodeStatus status = DS_OK;

    Unicode(UnicodeType type = NotUnicode) : type{type} {
    }

    bool encode_point(OutStream& out, u32 codepoint);
    s32 decode_point(InStream& in); // -1 at EOF
};

// ┏━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃  InPipe_ConvertUnicode  ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━┛
struct InPipe_ConvertUnicode : InPipe {
    InStream in;
    Unicode src_enc;

    // shim_storage is used to split multibyte characters at buffer boundaries.
    FixedArray<char, 4> shim_storage;
    StringView shim_used;

    InPipe_ConvertUnicode(InStream&& in, UnicodeType type = NotUnicode)
        : in{std::move(in)}, src_enc{type} {
    }

    // Fill dst_buf with UTF-8-encoded data.
    virtual u32 read(MutStringView dst_buf) override;
};

// ┏━━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃  OutPipe_ConvertUnicode  ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━━┛
struct OutPipe_ConvertUnicode : OutPipe {
    OutStream out;
    Unicode dst_enc;

    // shim_storage is used to join multibyte characters at buffer boundaries.
    char shim_storage[4];
    u32 shim_used;

    OutPipe_ConvertUnicode(OutStream&& out, UnicodeType type = NotUnicode)
        : out{std::move(out)}, dst_enc{type} {
    }

    // src_buf expects UTF-8-encoded data.
    virtual bool write(StringView src_buf) override;
    virtual void flush(bool hard) override;
};

} // namespace ply
