/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {
namespace fmt {

extern const u8 DigitTable[256];
extern const u32 WhitespaceMask[8];
PLY_DLL_ENTRY void scanUsingMask(InStream& in, const u32* mask, bool invert);
typedef Func<bool(char)> Callback;
PLY_DLL_ENTRY void scanUsingCallback(InStream& in, const Callback& callback);
PLY_DLL_ENTRY bool scanUpToAndIncludingSpecial(InStream& in, StringView special);

struct Radix {
    u32 base = 10;
};

//----------------------------------------------------------
// Built-in fmt::TypeParsers
//----------------------------------------------------------
template <>
struct TypeParser<u64> {
    static PLY_INLINE Radix defaultFormat() {
        return {10};
    }
    static PLY_DLL_ENTRY u64 parse(InStream& in, Radix radix);
};
template <>
struct TypeParser<s64> {
    static PLY_INLINE Radix defaultFormat() {
        return {10};
    }
    static PLY_DLL_ENTRY s64 parse(InStream& in, Radix radix);
};
template <>
struct TypeParser<double> {
    static PLY_INLINE Radix defaultFormat() {
        return {10};
    }
    static PLY_DLL_ENTRY double parse(InStream& in, Radix radix);
};
template <>
struct TypeParser<float> {
    static PLY_INLINE Radix defaultFormat() {
        return {10};
    }
    static PLY_INLINE float parse(InStream& in, Radix radix) {
        return (float) TypeParser<double>::parse(in, radix);
    }
};

template <typename DstType, typename FullType>
struct TypeParser_Integral {
    static PLY_INLINE Radix defaultFormat() {
        return {10};
    }
    static PLY_NO_INLINE DstType parse(InStream& in, Radix radix) {
        FullType fullValue = TypeParser<FullType>::parse(in, radix);
        DstType result = (DstType) fullValue;
        if (result != fullValue) {
            result = 0;
            in.status.parse_error = 1;
        }
        return result;
    }
};
template <>
struct TypeParser<u32> : TypeParser_Integral<u32, u64> {};
template <>
struct TypeParser<u16> : TypeParser_Integral<u16, u64> {};
template <>
struct TypeParser<u8> : TypeParser_Integral<u8, u64> {};
template <>
struct TypeParser<s32> : TypeParser_Integral<s32, s64> {};
template <>
struct TypeParser<s16> : TypeParser_Integral<s16, s64> {};
template <>
struct TypeParser<s8> : TypeParser_Integral<s8, s64> {};

//----------------------------------------------------------
// Built-in fmt::FormatParsers
//----------------------------------------------------------
// QuotedString
static constexpr u32 AllowSingleQuote = 0x1;
static constexpr u32 EscapeWithBackslash = 0x2;
static constexpr u32 CollapseDoubles = 0x4;
static constexpr u32 AllowMultilineWithTriple = 0x8;
struct QuotedString {
    enum ErrorCode {
        OK,
        NoOpeningQuote,
        UnexpectedEndOfLine,
        UnexpectedEndOfFile,
        BadEscapeSequence,
    };

    u32 flags = 0;
    Func<void(InStream&, ErrorCode)> errorCallback;
};
template <>
struct FormatParser<QuotedString> {
    static PLY_DLL_ENTRY String parse(InStream& in, const QuotedString& format);
};

// Identifier
static constexpr u32 WithDollarSign = 0x1;
static constexpr u32 WithDash = 0x2;
struct Identifier {
    u32 flags = 0;
    PLY_INLINE Identifier(u32 flags = 0) : flags{flags} {
    }
};
template <>
struct FormatParser<Identifier> {
    static PLY_DLL_ENTRY bool parse(InStream& in, const Identifier& format);
};

// Line
struct Line {};
template <>
struct FormatParser<Line> {
    static PLY_NO_INLINE bool parse(InStream& in, const Line&) {
        return fmt::scanUpToAndIncludingSpecial(in, "\n");
    }
};

// Whitespace
struct Whitespace {};
template <>
struct FormatParser<Whitespace> {
    static PLY_NO_INLINE void parse(InStream& in, const Whitespace&) {
        fmt::scanUsingMask(in, fmt::WhitespaceMask, false);
    }
};

// Callback
template <>
struct FormatParser<Callback> {
    static PLY_NO_INLINE void parse(InStream& in, const Callback& cb) {
        fmt::scanUsingCallback(in, cb);
    }
};

// NonWhitespace
struct NonWhitespace {};
template <>
struct FormatParser<NonWhitespace> {
    static PLY_NO_INLINE void parse(InStream& in, const NonWhitespace&) {
        fmt::scanUsingMask(in, fmt::WhitespaceMask, true);
    }
};

} // namespace fmt
} // namespace ply
