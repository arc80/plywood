/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/text/StringWriter.h>
#include <ply-runtime/string/TextEncoding.h>
#include <math.h>

namespace ply {

//---------------------------------------------------------------
// Primitives for printing numeric types
//----------------------------------------------------------------
PLY_INLINE u8 toDigit(u32 d) {
    return (d <= 35) ? "0123456789abcdefghijklmnopqrstuvwxyz"[d] : '?';
}

PLY_NO_INLINE void printString(OutStream* outs, u64 value, u32 radix) {
    PLY_ASSERT(radix >= 2);
    u8 digitBuffer[64];
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

    outs->write({digitBuffer + digitIndex, (u32) PLY_STATIC_ARRAY_SIZE(digitBuffer) - digitIndex});
}

PLY_NO_INLINE void printString(OutStream* outs, s64 value, u32 radix) {
    if (value >= 0) {
        printString(outs, (u64) value, radix);
    } else {
        outs->writeByte('-');
        printString(outs, (u64) -value, radix);
    }
}

PLY_NO_INLINE void printString(OutStream* outs, double value, u32 radix) {
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
        *outs->strWriter() << "nan";
    } else if (isinf(value)) {
        *outs->strWriter() << "inf";
    } else {
        u32 radix3 = radix * radix * radix;
        u32 radix6 = radix3 * radix3;
        if (value == 0.0 || (value * radix3 > radix && value < radix6)) {
            u64 fixedPoint = u64(value * radix3);
            printString(outs, fixedPoint / radix3, radix);
            outs->writeByte('.');
            u64 fractionalPart = fixedPoint % radix3;
            {
                // Print zeroed
                u8 digitBuffer[3];
                for (s32 i = 2; i >= 0; i--) {
                    u64 quotient = fractionalPart / radix;
                    u32 digit = u32(fractionalPart - quotient * radix);
                    digitBuffer[i] = toDigit(digit);
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
            outs->writeByte(toDigit(digit));
            outs->writeByte('.');
            for (u32 i = 0; i < 3; i++) {
                m = (m - digit) * radix;
                digit = clamp<s32>((s32) floor(m), 0, radix - 1);
                outs->writeByte(toDigit(digit));
            }
            outs->writeByte('e');
            printString(outs, (s64) exponent, radix);
        }
    }
}

//----------------------------------------------------------------
// StringWriter
//----------------------------------------------------------------
PLY_NO_INLINE StringWriter::StringWriter(u32 chunkSizeExp) : OutStream{Type::Mem, chunkSizeExp} {
    this->initFirstChunk();
    new (&this->headChunk) Reference<ChunkListNode>{this->chunk};
}

PLY_NO_INLINE void StringWriter::formatInternal(StringView fmt,
                                                ArrayView<const StringWriter::Arg> args) {
    u32 argIndex = 0;
    ConstBufferView bytes = fmt.bufferView();
    while (bytes.numBytes > 0) {
        if (bytes[0] == '{') {
            bytes.offsetHead(1);
            if (bytes.numBytes == 0) {
                PLY_ASSERT(0); // Invalid format string!
                break;
            }
            if (bytes[0] == '{') {
                this->writeByte('{');
            } else if (bytes[0] == '}') {
                PLY_ASSERT(argIndex <
                           args.numItems); // Not enough arguments provided for format string!
                args[argIndex].formatter(this, args[argIndex].pvalue);
                argIndex++;
            } else {
                PLY_ASSERT(0); // Invalid format string!
            }
        } else if (bytes[0] == '}') {
            bytes.offsetHead(1);
            if (bytes.numBytes == 0) {
                PLY_ASSERT(0); // Invalid format string!
                break;
            }
            if (bytes[0] == '}') {
                this->writeByte('}');
            } else {
                PLY_ASSERT(0); // Invalid format string!
            }
        } else {
            this->writeByte(bytes[0]);
        }
        bytes.offsetHead(1);
    }
    PLY_ASSERT(argIndex == args.numItems); // Too many arguments provided for format string!
}

//----------------------------------------------------
// fmt::TypePrinters
//----------------------------------------------------
PLY_NO_INLINE void fmt::TypePrinter<fmt::WithRadix>::print(StringWriter* sw,
                                                           const fmt::WithRadix& value) {
    switch (value.type) {
        case WithRadix::U64: {
            printString(sw, value.u64_, value.radix);
            break;
        }
        case WithRadix::S64: {
            printString(sw, value.s64_, value.radix);
            break;
        }
        case WithRadix::Double: {
            printString(sw, value.double_, value.radix);
            break;
        }
        default: {
            PLY_ASSERT(0);
            break;
        }
    }
}

PLY_NO_INLINE void fmt::TypePrinter<StringView>::print(StringWriter* sw, StringView value) {
    // FIXME: Do newline conversion here
    sw->write(value.bufferView());
}

PLY_NO_INLINE void fmt::TypePrinter<u64>::print(StringWriter* sw, u64 value) {
    printString(sw, value, 10);
}

PLY_NO_INLINE void fmt::TypePrinter<s64>::print(StringWriter* sw, s64 value) {
    printString(sw, value, 10);
}

PLY_NO_INLINE void fmt::TypePrinter<double>::print(StringWriter* sw, double value) {
    printString(sw, value, 10);
}

PLY_NO_INLINE void fmt::TypePrinter<bool>::print(StringWriter* sw, bool value) {
    *sw << (value ? StringView{"true"} : "false");
}

PLY_NO_INLINE void fmt::TypePrinter<CPUTimer::Duration>::print(StringWriter* sw,
                                                               CPUTimer::Duration value) {
    static CPUTimer::Converter converter;
    double seconds = converter.toSeconds(value);
    u64 s = (u64) seconds;
    u64 us = u64((seconds - s) * 1000000);
    u64 m = s / 60;
    s = s % 60;

    // FIXME: Optimize this, too much intermediate stuff
    // FIXME: Support zero-padding in format string
    sw->format("{}:{}.{}", m, (StringView{"0"} + String::from(s)).right(2),
               (StringView{"00000"} + String::from(us)).right(6));
}

PLY_NO_INLINE void fmt::TypePrinter<fmt::EscapedString>::print(StringWriter* sw,
                                                               const fmt::EscapedString& value) {
    ConstBufferView srcUnits = value.view.bufferView();
    u32 points = 0;
    while (srcUnits.numBytes > 0) {
        if (value.maxPoints > 0 && points >= value.maxPoints) {
            *sw << "...";
            break;
        }
        DecodeResult decoded = UTF8::decodePoint(srcUnits);
        switch (decoded.point) {
            case '"': {
                *sw << "\\\"";
                break;
            }
            case '\\': {
                *sw << "\\\\";
                break;
            }
            case '\r': {
                *sw << "\\r";
                break;
            }
            case '\n': {
                *sw << "\\n";
                break;
            }
            case '\t': {
                *sw << "\\t";
                break;
            }
            default: {
                if (decoded.point >= 32) {
                    // This will preserve badly encoded UTF8 characters exactly as they are in
                    // the source string:
                    sw->write(srcUnits.subView(0, decoded.numBytes));
                } else {
                    static const char* digits = "0123456789abcdef";
                    *sw << '\\' << digits[(decoded.point >> 4) & 0xf]
                        << digits[decoded.point & 0xf];
                }
                break;
            }
        }
        points++;
        srcUnits.offsetHead(decoded.numBytes);
    }
}

PLY_NO_INLINE void fmt::TypePrinter<fmt::XMLEscape>::print(StringWriter* sw,
                                                           const XMLEscape& value) {
    ConstBufferView srcUnits = value.view.bufferView();
    u32 points = 0;
    while (srcUnits.numBytes > 0) {
        if (value.maxPoints > 0 && points >= value.maxPoints) {
            *sw << "...";
            break;
        }
        DecodeResult decoded = UTF8::decodePoint(srcUnits);
        switch (decoded.point) {
            case '<': {
                *sw << "&lt;";
                break;
            }
            case '>': {
                *sw << "&gt;";
                break;
            }
            case '"': {
                *sw << "&quot;";
                break;
            }
            case '&': {
                *sw << "&amp;";
                break;
            }
            default: {
                // This will preserve badly encoded UTF8 characters exactly as they are in
                // the source string:
                sw->write(srcUnits.subView(0, decoded.numBytes));
                break;
            }
        }
        points++;
        srcUnits.offsetHead(decoded.numBytes);
    }
}

PLY_NO_INLINE void
fmt::TypePrinter<fmt::CmdLineArg_WinCrt>::print(StringWriter* sw,
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
        *sw << '"';
        u32 backslashCount = 0;
        for (const char* cur = value.view.bytes; cur < end; cur++) {
            char c = *cur;
            if (c == '\\') {
                backslashCount++;
            } else if (*cur == '"') {
                for (; backslashCount > 0; backslashCount--) {
                    *sw << "\\\\";
                }
                *sw << "\\\"";
            } else {
                for (; backslashCount > 0; backslashCount--) {
                    *sw << '\\';
                }
                *sw << c;
            }
        }
        *sw << '"';
    } else {
        *sw << value.view;
    }
}

} // namespace ply
