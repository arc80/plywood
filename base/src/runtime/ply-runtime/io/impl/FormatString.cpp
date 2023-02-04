/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>
#include <math.h>

namespace ply {

//---------------------------------------------------------------
// Primitives for printing numeric types
//----------------------------------------------------------------
PLY_INLINE char toDigit(u32 d) {
    const char* digitTable = "0123456789abcdefghijklmnopqrstuvwxyz";
    return (d <= 35) ? digitTable[d] : '?';
}

PLY_NO_INLINE void printString(OutStream& out, u64 value, u32 radix) {
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
            digitBuffer[--digitIndex] = toDigit(digit);
            value = quotient;
        }
    }

    out << StringView{digitBuffer + digitIndex,
                      (u32) PLY_STATIC_ARRAY_SIZE(digitBuffer) - digitIndex};
}

PLY_NO_INLINE void printString(OutStream& out, s64 value, u32 radix) {
    if (value >= 0) {
        printString(out, (u64) value, radix);
    } else {
        out << '-';
        printString(out, (u64) -value, radix);
    }
}

PLY_NO_INLINE void printString(OutStream& out, double value, u32 radix) {
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
            printString(out, fixedPoint / radix3, radix);
            out << '.';
            u64 fractionalPart = fixedPoint % radix3;
            {
                // Print zeroed
                char digitBuffer[3];
                for (s32 i = 2; i >= 0; i--) {
                    u64 quotient = fractionalPart / radix;
                    u32 digit = u32(fractionalPart - quotient * radix);
                    digitBuffer[i] = toDigit(digit);
                    fractionalPart = quotient;
                }
                out << StringView{digitBuffer, PLY_STATIC_ARRAY_SIZE(digitBuffer)};
            }
        } else {
            // Scientific notation
            double logBase = log(value) / log(radix);
            double exponent = floor(logBase);
            double m = value / pow(radix, exponent); // mantissa (initially)
            s32 digit = clamp<s32>((s32) floor(m), 1, radix - 1);
            out << toDigit(digit);
            out << '.';
            for (u32 i = 0; i < 3; i++) {
                m = (m - digit) * radix;
                digit = clamp<s32>((s32) floor(m), 0, radix - 1);
                out << toDigit(digit);
            }
            out << 'e';
            printString(out, (s64) exponent, radix);
        }
    }
}

//----------------------------------------------------------------
// OutStream
//----------------------------------------------------------------
void OutStream::format_args(StringView fmt, ArrayView<const FormatArg> args) {
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
                args[argIndex].print(*this);
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
void FormatArg::default_print(OutStream& out, const FormatArg& arg) {
    switch (arg.type) {
        case View:
            out << arg.view;
            break;

        case Bool:
            out << (arg.bool_ ? StringView{"true"} : "false");
            break;

        case S64:
            printString(out, arg.s64_, arg.radix);
            break;

        case U64:
            printString(out, arg.u64_, arg.radix);
            break;

        case Double:
            printString(out, arg.double_, arg.radix);
            break;
    }
}

void escape::do_print(OutStream& out, const FormatArg& arg) {
    PLY_ASSERT(arg.type == View);
    StringView srcUnits = arg.view;
    u32 points = 0;
    while (srcUnits.numBytes > 0) {
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
                    const char* digits = "0123456789abcdef";
                    out << '\\' << digits[(c >> 4) & 0xf] << digits[c & 0xf];
                }
                break;
            }
        }
        points++;
        srcUnits.offsetHead(1);
    }
}

void xml_escape::do_print(OutStream& out, const FormatArg& arg) {
    PLY_ASSERT(arg.type == View);
    StringView srcUnits = arg.view;
    u32 points = 0;
    while (srcUnits.numBytes > 0) {
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

void CmdLineArg_WinCrt::do_print(OutStream& out, const FormatArg& arg) {
    PLY_ASSERT(arg.type == View);
    bool needsQuote = false;
    const char* end = arg.view.end();
    for (const char* cur = arg.view.bytes; cur < end; cur++) {
        if (isWhite(*cur) || *cur == '"') {
            needsQuote = true;
            break;
        }
    }
    if (needsQuote) {
        out << '"';
        u32 backslashCount = 0;
        for (const char* cur = arg.view.bytes; cur < end; cur++) {
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
        out << arg.view;
    }
}

} // namespace ply
