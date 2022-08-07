/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/HiddenArgFunctor.h>

namespace ply {
namespace fmt {

extern const u8 DigitTable[256];
extern const u32 WhitespaceMask[8];
PLY_DLL_ENTRY void scanUsingMask(InStream* ins, const u32* mask, bool invert);
PLY_DLL_ENTRY void scanUsingCallback(InStream* ins, const LambdaView<bool(char)>& callback);
PLY_DLL_ENTRY bool scanUpToAndIncludingSpecial(InStream* ins, StringView special);

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
    static PLY_DLL_ENTRY u64 parse(InStream* ins, Radix radix);
};
template <>
struct TypeParser<s64> {
    static PLY_INLINE Radix defaultFormat() {
        return {10};
    }
    static PLY_DLL_ENTRY s64 parse(InStream* ins, Radix radix);
};
template <>
struct TypeParser<double> {
    static PLY_INLINE Radix defaultFormat() {
        return {10};
    }
    static PLY_DLL_ENTRY double parse(InStream* ins, Radix radix);
};
template <>
struct TypeParser<float> {
    static PLY_INLINE Radix defaultFormat() {
        return {10};
    }
    static PLY_INLINE float parse(InStream* ins, Radix radix) {
        return (float) TypeParser<double>::parse(ins, radix);
    }
};

template <typename DstType, typename FullType>
struct TypeParser_Integral {
    static PLY_INLINE Radix defaultFormat() {
        return {10};
    }
    static PLY_NO_INLINE DstType parse(InStream* ins, Radix radix) {
        FullType fullValue = TypeParser<FullType>::parse(ins, radix);
        DstType result = (DstType) fullValue;
        if (result != fullValue) {
            result = 0;
            ins->status.parseError = 1;
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
    HiddenArgFunctor<void(InStream*, ErrorCode)> errorCallback;
};
template <>
struct FormatParser<QuotedString> {
    static PLY_DLL_ENTRY String parse(InStream* ins, const QuotedString& format);
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
    static PLY_DLL_ENTRY bool parse(InStream* ins, const Identifier& format);
};

// Line
struct Line {};
template <>
struct FormatParser<Line> {
    static PLY_NO_INLINE bool parse(InStream* ins, const Line&) {
        return fmt::scanUpToAndIncludingSpecial(ins, "\n");
    }
};

// Whitespace
struct Whitespace {};
template <>
struct FormatParser<Whitespace> {
    static PLY_NO_INLINE void parse(InStream* ins, const Whitespace&) {
        fmt::scanUsingMask(ins, fmt::WhitespaceMask, false);
    }
};

// Callback
struct Callback {
    LambdaView<bool(char)> cb;
};
template <>
struct FormatParser<Callback> {
    static PLY_NO_INLINE void parse(InStream* ins, const Callback& cb) {
        fmt::scanUsingCallback(ins, cb.cb);
    }
};

// NonWhitespace
struct NonWhitespace {};
template <>
struct FormatParser<NonWhitespace> {
    static PLY_NO_INLINE void parse(InStream* ins, const NonWhitespace&) {
        fmt::scanUsingMask(ins, fmt::WhitespaceMask, true);
    }
};

} // namespace fmt
} // namespace ply
