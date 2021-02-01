/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/InStream.h>
#include <ply-runtime/io/OutStream.h>
#include <math.h> // for pow

namespace ply {

// clang-format off
const u8 fmt::DigitTable[256] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 255, 255, 255, 255, 255, 255,
    255, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 255, 255, 255, 255, 255,
    255, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};
// clang-format on

const u32 fmt::WhitespaceMask[8] = {0x2600, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

PLY_INLINE bool match(u8 c, const u32* mask) {
    u32 bitValue = mask[c >> 5] & (1 << (c & 31));
    return (bitValue != 0);
}

PLY_NO_INLINE void fmt::scanUsingMask(InStream* ins, const u32* mask, bool invert) {
    for (;;) {
        if (!ins->tryMakeBytesAvailable())
            break;
        if (match(ins->peekByte(), mask) == invert)
            break;
        ins->advanceByte();
    }
}

PLY_NO_INLINE void fmt::scanUsingCallback(InStream* ins, const LambdaView<bool(char)>& callback) {
    for (;;) {
        if (!ins->tryMakeBytesAvailable())
            break;
        if (!callback(ins->peekByte()))
            break;
        ins->advanceByte();
    }
}

PLY_NO_INLINE bool fmt::scanUpToAndIncludingSpecial(InStream* ins, StringView special) {
    PLY_ASSERT(special.numBytes > 0);
    PLY_ASSERT(special.subStr(1).findByte(special[0]) < 0); // first letter must not reoccur
    u32 matchedUnits = 0;
    for (;;) {
        if (!ins->tryMakeBytesAvailable())
            break;
        u8 c = ins->peekByte();
        ins->advanceByte();
        if (c == (u8) special.bytes[matchedUnits]) {
            matchedUnits++;
            if (matchedUnits >= special.numBytes)
                return true;
        } else {
            matchedUnits = 0;
        }
    }
    // special wasn't found
    ins->status.parseError = 1;
    return false;
}

//----------------------------------------------------------
// Built-in fmt::TypeParsers
//----------------------------------------------------------
PLY_NO_INLINE u64 fmt::TypeParser<u64>::parse(InStream* ins, Radix radix) {
    // FIXME: 32-bit platforms like WASM would benefit from dedicated u32 parse function instead of
    // parsing a u64 then casting the result to u32.
    PLY_ASSERT(radix.base > 0 && radix.base <= 36);
    u64 result = 0;
    bool anyDigits = false;
    bool overflow = false;
    for (;;) {
        if (!ins->tryMakeBytesAvailable())
            break;
        u8 digit = fmt::DigitTable[(u8) ins->peekByte()];
        if (digit >= radix.base)
            break;
        ins->advanceByte();
        // FIXME: When available, check for (multiplicative & additive) overflow using
        // https://gcc.gnu.org/onlinedocs/gcc/Integer-Overflow-Builtins.html#Integer-Overflow-Builtins
        // and equivalent intrinsics instead of the following.
        // Note: 0x71c71c71c71c71b is the largest value that won't overflow for any radix <= 36. We
        // test against this constant first to avoid the costly integer division.
        if (result > 0x71c71c71c71c71b && result > (Limits<u64>::Max - digit) / radix.base) {
            overflow = true;
        }
        result = result * radix.base + digit;
        anyDigits = true;
    }
    if (!anyDigits || overflow) {
        ins->status.parseError = 1;
        return 0;
    }
    return result;
}

PLY_NO_INLINE s64 fmt::TypeParser<s64>::parse(InStream* ins, Radix radix) {
    bool negate = false;
    if (ins->tryMakeBytesAvailable() && ins->peekByte() == '-') {
        negate = true;
        ins->advanceByte();
    }

    u64 unsignedComponent = fmt::TypeParser<u64>::parse(ins, radix);
    if (negate) {
        s64 result = -(s64) unsignedComponent;
        if (result > 0) {
            ins->status.parseError = 1;
        }
        return result;
    } else {
        s64 result = unsignedComponent;
        if (result < 0) {
            ins->status.parseError = 1;
        }
        return result;
    }
}

struct DoubleComponentOut {
    double result = 0;
    bool anyDigits = false;
};

PLY_NO_INLINE void readDoubleComponent(DoubleComponentOut* compOut, InStream* ins,
                                       fmt::Radix radix) {
    double value = 0.0;
    double dr = (double) radix.base;
    for (;;) {
        if (!ins->tryMakeBytesAvailable())
            break;
        u8 digit = fmt::DigitTable[(u8) ins->peekByte()];
        if (digit >= radix.base)
            break;
        ins->advanceByte();
        value = value * dr + digit;
        compOut->anyDigits = true;
    }
    compOut->result = value;
}

PLY_NO_INLINE double fmt::TypeParser<double>::parse(InStream* ins, Radix radix) {
    PLY_ASSERT(radix.base > 0 && radix.base <= 36);
    DoubleComponentOut comp;

    // Parse the optional minus sign
    bool negate = false;
    if (ins->tryMakeBytesAvailable() && ins->peekByte() == '-') {
        ins->advanceByte();
        negate = true;
    }

    // Parse the mantissa
    readDoubleComponent(&comp, ins, radix);
    double value = comp.result;

    // Parse the optional fractional part
    if (ins->tryMakeBytesAvailable() && ins->peekByte() == '.') {
        ins->advanceByte();
        double significance = 1.0;
        u64 numer = 0;
        u64 denom = 1;
        for (;;) {
            if (!ins->tryMakeBytesAvailable())
                break;
            u8 digit = fmt::DigitTable[(u8) ins->peekByte()];
            if (digit >= radix.base)
                break;
            ins->advanceByte();
            u64 denomWithNextDigit = denom * radix.base;
            if (denomWithNextDigit < denom) {
                // denominator overflowed
                double ooDenom = 1.0 / denom;
                value += significance * numer * ooDenom;
                significance *= ooDenom;
                numer = digit;
                denom = radix.base;
            } else {
                numer = numer * radix.base + digit;
                denom = denomWithNextDigit;
            }
        }
        value += significance * numer / denom;
    }

    // Parse optional exponent suffix
    if (comp.anyDigits && ins->tryMakeBytesAvailable() && (ins->peekByte() | 0x20) == 'e') {
        ins->advanceByte();
        bool negateExp = false;
        if (ins->tryMakeBytesAvailable()) {
            if (ins->peekByte() == '+') {
                ins->advanceByte();
            } else if (ins->peekByte() == '-') {
                ins->advanceByte();
                negateExp = true;
            }
        }
        comp.anyDigits = false;
        readDoubleComponent(&comp, ins, radix);
        value *= pow((double) radix.base, negateExp ? -comp.result : comp.result);
    }

    if (!comp.anyDigits) {
        ins->status.parseError = true;
    }

    return negate ? -value : value;
}

//----------------------------------------------------------
// Built-in fmt::FormatParsers
//----------------------------------------------------------
String fmt::FormatParser<fmt::QuotedString>::parse(InStream* ins, const fmt::QuotedString& format) {
    // Note: It should be possible to modify this function to return a HybridString, and avoid
    // making a copy of the quoted string when reading from a StringView and there are no escape
    // characters. Could be worthwhile.
    auto handleError = [&](fmt::QuotedString::ErrorCode errorCode) {
        ins->status.parseError = 1;
        if (format.errorCallback) {
            format.errorCallback(ins, errorCode);
        }
    };

    // Get opening quote
    if (!ins->tryMakeBytesAvailable()) {
        handleError(fmt::QuotedString::UnexpectedEndOfFile);
        return {};
    }
    u8 quoteType = ins->peekByte();
    if (!(quoteType == '"' || ((format.flags & fmt::AllowSingleQuote) && quoteType == '\''))) {
        handleError(fmt::QuotedString::NoOpeningQuote);
        return {};
    }
    ins->advanceByte();

    // Parse rest of quoted string
    MemOutStream outs;
    u32 quoteRun = 1;
    bool multiline = false;
    for (;;) {
        if (!ins->tryMakeBytesAvailable()) {
            handleError(fmt::QuotedString::UnexpectedEndOfFile);
            break; // end of string
        }

        u8 nextByte = ins->peekByte();
        if (nextByte == quoteType) {
            ins->advanceByte();
            if (quoteRun == 0) {
                if (multiline) {
                    quoteRun++;
                } else {
                    break; // end of string
                }
            } else {
                quoteRun++;
                if (quoteRun == 3) {
                    if (multiline) {
                        break; // end of string
                    } else {
                        multiline = true;
                        quoteRun = 0;
                    }
                }
            }
        } else {
            // FIXME: Check fmt::AllowMultilineWithTriple (and other flags)
            if (quoteRun > 0) {
                if (multiline) {
                    for (u32 i = 0; i < quoteRun; i++) {
                        outs.writeByte(quoteType);
                    }
                } else if (quoteRun == 2) {
                    break; // empty string
                }
                quoteRun = 0;
            }

            switch (nextByte) {
                case '\r':
                case '\n': {
                    if (multiline) {
                        if (nextByte == '\n') {
                            outs.writeByte(nextByte);
                        }
                        ins->advanceByte();
                    } else {
                        handleError(fmt::QuotedString::UnexpectedEndOfLine);
                        goto endOfString;
                    }
                    break;
                }

                case '\\': {
                    // Escape sequence
                    ins->advanceByte();
                    if (!ins->tryMakeBytesAvailable()) {
                        handleError(fmt::QuotedString::UnexpectedEndOfFile);
                        goto endOfString;
                    }
                    u8 code = ins->peekByte();
                    switch (code) {
                        case '\r':
                        case '\n': {
                            handleError(fmt::QuotedString::UnexpectedEndOfLine);
                            goto endOfString;
                        }

                        case '\\':
                        case '\'':
                        case '"': {
                            outs.writeByte(code);
                            break;
                        }

                        case 'r': {
                            outs.writeByte('\r');
                            break;
                        }

                        case 'n': {
                            outs.writeByte('\n');
                            break;
                        }

                        case 't': {
                            outs.writeByte('\t');
                            break;
                        }

                            // FIXME: Implement escape hex codes
                            // case 'x':

                        default: {
                            handleError(fmt::QuotedString::BadEscapeSequence);
                            break;
                        }
                    }
                    ins->advanceByte();
                    break;
                }

                default: {
                    outs.writeByte(nextByte);
                    ins->advanceByte();
                    break;
                }
            }
        }
    }

endOfString:
    return outs.moveToString();
}

bool fmt::FormatParser<fmt::Identifier>::parse(InStream* ins, const fmt::Identifier& format) {
    u32 mask[8] = {0, 0, 0x87fffffe, 0x7fffffe, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
    if ((format.flags & WithDollarSign) != 0) {
        mask[1] |= 0x10; // '$'
    }
    if ((format.flags & WithDash) != 0) {
        mask[1] |= 0x2000; // '-'
    }

    bool first = true;
    for (;;) {
        if (!ins->tryMakeBytesAvailable())
            break;
        if (!match(ins->peekByte(), mask))
            break;
        if (first) {
            mask[1] |= 0x3ff0000; // accept digits after first unit
            first = false;
        };
        ins->advanceByte();
    }

    if (mask[1] == 0) {
        ins->status.parseError = 1;
        return false;
    }
    return true;
}

} // namespace ply
