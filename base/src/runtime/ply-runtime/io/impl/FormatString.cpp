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

PLY_NO_INLINE void printString(OutStream& out, u64 value, u32 radix, bool capitalize) {
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

    out.write({digitBuffer + digitIndex,
               (u32) PLY_STATIC_ARRAY_SIZE(digitBuffer) - digitIndex});
}

PLY_NO_INLINE void printString(OutStream& out, s64 value, u32 radix, bool capitalize) {
    if (value >= 0) {
        printString(out, (u64) value, radix, capitalize);
    } else {
        out << '-';
        printString(out, (u64) -value, radix, capitalize);
    }
}

PLY_NO_INLINE void printString(OutStream& out, double value, u32 radix,
                               bool capitalize) {
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
        out << '-';
    }
    if (isnan(value)) {
        out << "nan";
    } else if (isinf(value)) {
        out << "inf";
    } else {
        u32 radix3 = radix * radix * radix;
        u32 radix6 = radix3 * radix3;
        if (value == 0.0 || (value * radix3 > radix && value < radix6)) {
            u64 fixedPoint = u64(value * radix3);
            printString(out, fixedPoint / radix3, radix, capitalize);
            out << '.';
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
                out.write({digitBuffer, PLY_STATIC_ARRAY_SIZE(digitBuffer)});
            }
        } else {
            // Scientific notation
            double logBase = log(value) / log(radix);
            double exponent = floor(logBase);
            double m = value / pow(radix, exponent); // mantissa (initially)
            s32 digit = clamp<s32>((s32) floor(m), 1, radix - 1);
            out << toDigit(digit, capitalize);
            out << '.';
            for (u32 i = 0; i < 3; i++) {
                m = (m - digit) * radix;
                digit = clamp<s32>((s32) floor(m), 0, radix - 1);
                out << toDigit(digit, capitalize);
            }
            out << 'e';
            printString(out, (s64) exponent, radix, capitalize);
        }
    }
}

//----------------------------------------------------------------
// OutStream
//----------------------------------------------------------------
PLY_NO_INLINE void OutStream::format_args(StringView fmt,
                                          ArrayView<const FormatArg> args) {
    u32 argIndex = 0;
    while (fmt.numBytes > 0) {
        if (fmt[0] == '{') {
            fmt.offsetHead(1);
            if (fmt.numBytes == 0) {
                PLY_ASSERT(0); // Invalid format string!
                break;
            }
            if (fmt[0] == '{') {
                *this << '{';
            } else if (fmt[0] == '}') {
                PLY_ASSERT(
                    argIndex <
                    args.numItems); // Not enough arguments provided for format string!
                args[argIndex].func(*this, args[argIndex].value);
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
                *this << '}';
            } else {
                PLY_ASSERT(0); // Invalid format string!
            }
        } else {
            *this << fmt[0];
        }
        fmt.offsetHead(1);
    }
    PLY_ASSERT(argIndex ==
               args.numItems); // Too many arguments provided for format string!
}

//----------------------------------------------------
// fmt::TypePrinters
//----------------------------------------------------
PLY_NO_INLINE void
fmt::TypePrinter<fmt::WithRadix>::print(OutStream& out, const fmt::WithRadix& value) {
    switch (value.type) {
        case WithRadix::U64: {
            printString(out, value.u64_, value.radix, value.capitalize);
            break;
        }
        case WithRadix::S64: {
            printString(out, value.s64_, value.radix, value.capitalize);
            break;
        }
        case WithRadix::Double: {
            printString(out, value.double_, value.radix, value.capitalize);
            break;
        }
        default: {
            PLY_ASSERT(0);
            break;
        }
    }
}

PLY_NO_INLINE void fmt::TypePrinter<StringView>::print(OutStream& out,
                                                       StringView value) {
    // FIXME: Do newline conversion here
    out.write(value);
}

PLY_NO_INLINE void fmt::TypePrinter<u64>::print(OutStream& out, u64 value) {
    printString(out, value, 10, false);
}

PLY_NO_INLINE void fmt::TypePrinter<s64>::print(OutStream& out, s64 value) {
    printString(out, value, 10, false);
}

PLY_NO_INLINE void fmt::TypePrinter<double>::print(OutStream& out, double value) {
    printString(out, value, 10, false);
}

PLY_NO_INLINE void fmt::TypePrinter<bool>::print(OutStream& out, bool value) {
    out << (value ? StringView{"true"} : "false");
}

PLY_NO_INLINE void
fmt::TypePrinter<CPUTimer::Duration>::print(OutStream& out, CPUTimer::Duration value) {
    static CPUTimer::Converter converter;
    double seconds = converter.toSeconds(value);
    u64 s = (u64) seconds;
    u64 us = u64((seconds - s) * 1000000);
    u64 m = s / 60;
    s = s % 60;

    // FIXME: Optimize this, too much intermediate stuff
    // FIXME: Support zero-padding in format string
    out.format("{}:{}.{}", m, (StringView{"0"} + String::from(s)).right(2),
               (StringView{"00000"} + String::from(us)).right(6));
}

PLY_NO_INLINE void
fmt::TypePrinter<fmt::EscapedString>::print(OutStream& out,
                                            const fmt::EscapedString& value) {
    StringView srcUnits = value.view;
    u32 points = 0;
    while (srcUnits.numBytes > 0) {
        if (value.maxPoints > 0 && points >= value.maxPoints) {
            out << "...";
            break;
        }
        char c = srcUnits.bytes[0];
        switch (c) {
            case '"': {
                out << "\\\"";
                break;
            }
            case '\\': {
                out << "\\\\";
                break;
            }
            case '\r': {
                out << "\\r";
                break;
            }
            case '\n': {
                out << "\\n";
                break;
            }
            case '\t': {
                out << "\\t";
                break;
            }
            default: {
                if (c >= 32) {
                    // This will preserve badly encoded UTF8 characters exactly as they
                    // are in the source string:
                    out << c;
                } else {
                    static const char* digits = "0123456789abcdef";
                    out << '\\' << digits[(c >> 4) & 0xf] << digits[c & 0xf];
                }
                break;
            }
        }
        points++;
        srcUnits.offsetHead(1);
    }
}

PLY_NO_INLINE void fmt::TypePrinter<fmt::XMLEscape>::print(OutStream& out,
                                                           const XMLEscape& value) {
    StringView srcUnits = value.view;
    u32 points = 0;
    while (srcUnits.numBytes > 0) {
        if (value.maxPoints > 0 && points >= value.maxPoints) {
            out << "...";
            break;
        }
        char c = srcUnits.bytes[0];
        switch (c) {
            case '<': {
                out << "&lt;";
                break;
            }
            case '>': {
                out << "&gt;";
                break;
            }
            case '"': {
                out << "&quot;";
                break;
            }
            case '&': {
                out << "&amp;";
                break;
            }
            default: {
                out << c;
                break;
            }
        }
        points++;
        srcUnits.offsetHead(1);
    }
}

PLY_NO_INLINE void
fmt::TypePrinter<fmt::CmdLineArg_WinCrt>::print(OutStream& out,
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
        out << '"';
        u32 backslashCount = 0;
        for (const char* cur = value.view.bytes; cur < end; cur++) {
            char c = *cur;
            if (c == '\\') {
                backslashCount++;
            } else if (*cur == '"') {
                for (; backslashCount > 0; backslashCount--) {
                    out << "\\\\";
                }
                out << "\\\"";
            } else {
                for (; backslashCount > 0; backslashCount--) {
                    out << '\\';
                }
                out << c;
            }
        }
        out << '"';
    } else {
        out << value.view;
    }
}

} // namespace ply
