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
void scan_using_mask(InStream& in, const u32* mask, bool invert);
typedef Func<bool(char)> Callback;
void scan_using_callback(InStream& in, const Callback& callback);
bool scan_up_to_and_including_special(InStream& in, StringView special);

struct Radix {
    u32 base = 10;
};

//----------------------------------------------------------
// Built-in fmt::TypeParsers
//----------------------------------------------------------
template <>
struct TypeParser<u64> {
    static Radix default_format() {
        return {10};
    }
    static u64 parse(InStream& in, Radix radix);
};
template <>
struct TypeParser<s64> {
    static Radix default_format() {
        return {10};
    }
    static s64 parse(InStream& in, Radix radix);
};
template <>
struct TypeParser<double> {
    static Radix default_format() {
        return {10};
    }
    static double parse(InStream& in, Radix radix);
};
template <>
struct TypeParser<float> {
    static Radix default_format() {
        return {10};
    }
    static float parse(InStream& in, Radix radix) {
        return (float) TypeParser<double>::parse(in, radix);
    }
};

template <typename DstType, typename FullType>
struct TypeParser_Integral {
    static Radix default_format() {
        return {10};
    }
    static DstType parse(InStream& in, Radix radix) {
        FullType full_value = TypeParser<FullType>::parse(in, radix);
        DstType result = (DstType) full_value;
        if (result != full_value) {
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
    Func<void(InStream&, ErrorCode)> error_callback;
};
template <>
struct FormatParser<QuotedString> {
    static String parse(InStream& in, const QuotedString& format);
};

// Identifier
static constexpr u32 WithDollarSign = 0x1;
static constexpr u32 WithDash = 0x2;
struct Identifier {
    u32 flags = 0;
    Identifier(u32 flags = 0) : flags{flags} {
    }
};
template <>
struct FormatParser<Identifier> {
    static bool parse(InStream& in, const Identifier& format);
};

// Line
struct Line {};
template <>
struct FormatParser<Line> {
    static bool parse(InStream& in, const Line&) {
        return fmt::scan_up_to_and_including_special(in, "\n");
    }
};

// Whitespace
struct Whitespace {};
template <>
struct FormatParser<Whitespace> {
    static void parse(InStream& in, const Whitespace&) {
        fmt::scan_using_mask(in, fmt::WhitespaceMask, false);
    }
};

// Callback
template <>
struct FormatParser<Callback> {
    static void parse(InStream& in, const Callback& cb) {
        fmt::scan_using_callback(in, cb);
    }
};

// NonWhitespace
struct NonWhitespace {};
template <>
struct FormatParser<NonWhitespace> {
    static void parse(InStream& in, const NonWhitespace&) {
        fmt::scan_using_mask(in, fmt::WhitespaceMask, true);
    }
};

} // namespace fmt
} // namespace ply
