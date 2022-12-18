/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/OutStream.h>
#include <ply-runtime/string/TextEncoding.h>
#include <math.h>

namespace ply {

//---------------------------------------------------------------
// Primitives for printing numeric types
//----------------------------------------------------------------
PLY_INLINE char toDigit(u32 d, bool capitalize = false) {
    const char* digitTable = capitalize ? "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        : "0123456789abcdefghijklmnopqrstuvwxyz";
    return (d <= 35) ? digitTable[d] : '?';
}

PLY_NO_INLINE void printString(OutStream* outs, u64 value, u32 radix, bool capitalize) {
    PLY_ASSERT(radix >= 2);
    char digitBuffer[64];
    s32 digitIndex = PLY_STATIC_ARRAY_SIZE(digitBuffer);

    if (value == 0) {
        digitBuffer[--digitIndex] = '0';
    } else {
        while (value > 0) {
            u64 quotient = value / radix;
            u32 digit = u32(value - quotient * radix);
            PLY_ASSERT(digitIndex > 0);
            digitBuffer[--digitIndex] = toDigit(digit, capitalize);
            value = quotient;
        }
    }

    outs->write({digitBuffer + digitIndex, (u32) PLY_STATIC_ARRAY_SIZE(digitBuffer) - digitIndex});
}

PLY_NO_INLINE void printString(OutStream* outs, s64 value, u32 radix, bool capitalize) {
    if (value >= 0) {
        printString(outs, (u64) value, radix, capitalize);
    } else {
        outs->writeByte('-');
        printString(outs, (u64) -value, radix, capitalize);
    }
}

PLY_NO_INLINE void printString(OutStream* outs, double value, u32 radix, bool capitalize) {
    PLY_ASSERT(radix >= 2);

#if PLY_COMPILER_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
    if (*(s64*) &value < 0) {
#if PLY_COMPILER_GCC
#pragma GCC diagnostic pop
#endif
        value = -value;
        outs->writeByte('-');
    }
    if (isnan(value)) {
        *outs << "nan";
    } else if (isinf(value)) {
        *outs << "inf";
    } else {
        u32 radix3 = radix * radix * radix;
        u32 radix6 = radix3 * radix3;
        if (value == 0.0 || (value * radix3 > radix && value < radix6)) {
            u64 fixedPoint = u64(value * radix3);
            printString(outs, fixedPoint / radix3, radix, capitalize);
            outs->writeByte('.');
            u64 fractionalPart = fixedPoint % radix3;
            {
                // Print zeroed
                char digitBuffer[3];
                for (s32 i = 2; i >= 0; i--) {
                    u64 quotient = fractionalPart / radix;
                    u32 digit = u32(fractionalPart - quotient * radix);
                    digitBuffer[i] = toDigit(digit, capitalize);
                    fractionalPart = quotient;
                }
                outs->write({digitBuffer, PLY_STATIC_ARRAY_SIZE(digitBuffer)});
            }
        } else {
            // Scientific notation
            double logBase = log(value) / log(radix);
            double exponent = floor(logBase);
            double m = value / pow(radix, exponent); // mantissa (initially)
            s32 digit = clamp<s32>((s32) floor(m), 1, radix - 1);
            outs->writeByte(toDigit(digit, capitalize));
            outs->writeByte('.');
            for (u32 i = 0; i < 3; i++) {
                m = (m - digit) * radix;
                digit = clamp<s32>((s32) floor(m), 0, radix - 1);
                outs->writeByte(toDigit(digit, capitalize));
            }
            outs->writeByte('e');
            printString(outs, (s64) exponent, radix, capitalize);
        }
    }
}

//----------------------------------------------------------------
// OutStream
//----------------------------------------------------------------
PLY_NO_INLINE void OutStream::formatInternal(StringView fmt, ArrayView<const FormatArg> args) {
    u32 argIndex = 0;
    while (fmt.numBytes > 0) {
        if (fmt[0] == '{') {
            fmt.offsetHead(1);
            if (fmt.numBytes == 0) {
                PLY_ASSERT(0); // Invalid format string!
                break;
            }
            if (fmt[0] == '{') {
                this->writeByte('{');
            } else if (fmt[0] == '}') {
                PLY_ASSERT(argIndex <
                           args.numItems); // Not enough arguments provided for format string!
                args[argIndex].func(this, args[argIndex].value);
                argIndex++;
            } else {
                PLY_ASSERT(0); // Invalid format string!
            }
        } else if (fmt[0] == '}') {
            fmt.offsetHead(1);
            if (fmt.numBytes == 0) {
                PLY_ASSERT(0); // Invalid format string!
                break;
            }
            if (fmt[0] == '}') {
                this->writeByte('}');
            } else {
                PLY_ASSERT(0); // Invalid format string!
            }
        } else {
            this->writeByte(fmt[0]);
        }
        fmt.offsetHead(1);
    }
    PLY_ASSERT(argIndex == args.numItems); // Too many arguments provided for format string!
}

//----------------------------------------------------
// fmt::TypePrinters
//----------------------------------------------------
PLY_NO_INLINE void fmt::TypePrinter<fmt::WithRadix>::print(OutStream* outs,
                                                           const fmt::WithRadix& value) {
    switch (value.type) {
        case WithRadix::U64: {
            printString(outs, value.u64_, value.radix, value.capitalize);
            break;
        }
        case WithRadix::S64: {
            printString(outs, value.s64_, value.radix, value.capitalize);
            break;
        }
        case WithRadix::Double: {
            printString(outs, value.double_, value.radix, value.capitalize);
            break;
        }
        default: {
            PLY_ASSERT(0);
            break;
        }
    }
}

PLY_NO_INLINE void fmt::TypePrinter<StringView>::print(OutStream* outs, StringView value) {
    // FIXME: Do newline conversion here
    outs->write(value);
}

PLY_NO_INLINE void fmt::TypePrinter<u64>::print(OutStream* outs, u64 value) {
    printString(outs, value, 10, false);
}

PLY_NO_INLINE void fmt::TypePrinter<s64>::print(OutStream* outs, s64 value) {
    printString(outs, value, 10, false);
}

PLY_NO_INLINE void fmt::TypePrinter<double>::print(OutStream* outs, double value) {
    printString(outs, value, 10, false);
}

PLY_NO_INLINE void fmt::TypePrinter<bool>::print(OutStream* outs, bool value) {
    *outs << (value ? StringView{"true"} : "false");
}

PLY_NO_INLINE void fmt::TypePrinter<CPUTimer::Duration>::print(OutStream* outs,
                                                               CPUTimer::Duration value) {
    static CPUTimer::Converter converter;
    double seconds = converter.toSeconds(value);
    u64 s = (u64) seconds;
    u64 us = u64((seconds - s) * 1000000);
    u64 m = s / 60;
    s = s % 60;

    // FIXME: Optimize this, too much intermediate stuff
    // FIXME: Support zero-padding in format string
    outs->format("{}:{}.{}", m, (StringView{"0"} + String::from(s)).right(2),
               (StringView{"00000"} + String::from(us)).right(6));
}

PLY_NO_INLINE void fmt::TypePrinter<fmt::EscapedString>::print(OutStream* outs,
                                                               const fmt::EscapedString& value) {
    StringView srcUnits = value.view;
    u32 points = 0;
    while (srcUnits.numBytes > 0) {
        if (value.maxPoints > 0 && points >= value.maxPoints) {
            *outs << "...";
            break;
        }
        DecodeResult decoded = UTF8::decodePoint(srcUnits);
        switch (decoded.point) {
            case '"': {
                *outs << "\\\"";
                break;
            }
            case '\\': {
                *outs << "\\\\";
                break;
            }
            case '\r': {
                *outs << "\\r";
                break;
            }
            case '\n': {
                *outs << "\\n";
                break;
            }
            case '\t': {
                *outs << "\\t";
                break;
            }
            default: {
                if (decoded.point >= 32) {
                    // This will preserve badly encoded UTF8 characters exactly as they are in
                    // the source string:
                    outs->write(srcUnits.left(decoded.numBytes));
                } else {
                    static const char* digits = "0123456789abcdef";
                    *outs << '\\' << digits[(decoded.point >> 4) & 0xf]
                        << digits[decoded.point & 0xf];
                }
                break;
            }
        }
        points++;
        srcUnits.offsetHead(decoded.numBytes);
    }
}

PLY_NO_INLINE void fmt::TypePrinter<fmt::XMLEscape>::print(OutStream* outs, const XMLEscape& value) {
    StringView srcUnits = value.view;
    u32 points = 0;
    while (srcUnits.numBytes > 0) {
        if (value.maxPoints > 0 && points >= value.maxPoints) {
            *outs << "...";
            break;
        }
        DecodeResult decoded = UTF8::decodePoint(srcUnits);
        switch (decoded.point) {
            case '<': {
                *outs << "&lt;";
                break;
            }
            case '>': {
                *outs << "&gt;";
                break;
            }
            case '"': {
                *outs << "&quot;";
                break;
            }
            case '&': {
                *outs << "&amp;";
                break;
            }
            default: {
                // This will preserve badly encoded UTF8 characters exactly as they are in
                // the source string:
                outs->write(srcUnits.left(decoded.numBytes));
                break;
            }
        }
        points++;
        srcUnits.offsetHead(decoded.numBytes);
    }
}

PLY_NO_INLINE void
fmt::TypePrinter<fmt::CmdLineArg_WinCrt>::print(OutStream* outs,
                                                const fmt::CmdLineArg_WinCrt& value) {
    bool needsQuote = false;
    const char* end = value.view.end();
    for (const char* cur = value.view.bytes; cur < end; cur++) {
        if (isWhite(*cur) || *cur == '"') {
            needsQuote = true;
            break;
        }
    }
    if (needsQuote) {
        *outs << '"';
        u32 backslashCount = 0;
        for (const char* cur = value.view.bytes; cur < end; cur++) {
            char c = *cur;
            if (c == '\\') {
                backslashCount++;
            } else if (*cur == '"') {
                for (; backslashCount > 0; backslashCount--) {
                    *outs << "\\\\";
                }
                *outs << "\\\"";
            } else {
                for (; backslashCount > 0; backslashCount--) {
                    *outs << '\\';
                }
                *outs << c;
            }
        }
        *outs << '"';
    } else {
        *outs << value.view;
    }
}

} // namespace ply
