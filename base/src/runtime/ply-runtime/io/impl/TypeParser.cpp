/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>
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
    u32 bit_value = mask[c >> 5] & (1 << (c & 31));
    return (bit_value != 0);
}

PLY_NO_INLINE void fmt::scan_using_mask(InStream& in, const u32* mask, bool invert) {
    for (;;) {
        if (!in.ensure_readable())
            break;
        if (match(*in.cur_byte, mask) == invert)
            break;
        in.cur_byte++;
    }
}

PLY_NO_INLINE void fmt::scan_using_callback(InStream& in,
                                            const Func<bool(char)>& callback) {
    for (;;) {
        if (!in.ensure_readable())
            break;
        if (!callback(*in.cur_byte))
            break;
        in.cur_byte++;
    }
}

PLY_NO_INLINE bool fmt::scan_up_to_and_including_special(InStream& in,
                                                         StringView special) {
    PLY_ASSERT(special.num_bytes > 0);
    PLY_ASSERT(special.sub_str(1).find_byte(special[0]) <
               0); // first letter must not reoccur
    u32 matched_units = 0;
    for (;;) {
        if (!in.ensure_readable())
            break;
        u8 c = *in.cur_byte;
        in.cur_byte++;
        if (c == (u8) special.bytes[matched_units]) {
            matched_units++;
            if (matched_units >= special.num_bytes)
                return true;
        } else {
            matched_units = 0;
        }
    }
    // special wasn't found
    in.status.parse_error = 1;
    return false;
}

//----------------------------------------------------------
// Built-in fmt::TypeParsers
//----------------------------------------------------------
PLY_NO_INLINE u64 fmt::TypeParser<u64>::parse(InStream& in, Radix radix) {
    // FIXME: 32-bit platforms like WASM would benefit from dedicated u32 parse function
    // instead of parsing a u64 then casting the result to u32.
    PLY_ASSERT(radix.base > 0 && radix.base <= 36);
    u64 result = 0;
    bool any_digits = false;
    bool overflow = false;
    for (;;) {
        if (!in.ensure_readable())
            break;
        u8 digit = fmt::DigitTable[(u8) *in.cur_byte];
        if (digit >= radix.base)
            break;
        in.cur_byte++;
        // FIXME: When available, check for (multiplicative & additive) overflow using
        // https://gcc.gnu.org/onlinedocs/gcc/Integer-Overflow-Builtins.html#Integer-Overflow-Builtins
        // and equivalent intrinsics instead of the following.
        // Note: 0x71c71c71c71c71b is the largest value that won't overflow for any
        // radix <= 36. We test against this constant first to avoid the costly integer
        // division.
        if (result > 0x71c71c71c71c71b &&
            result > (Limits<u64>::Max - digit) / radix.base) {
            overflow = true;
        }
        result = result * radix.base + digit;
        any_digits = true;
    }
    if (!any_digits || overflow) {
        in.status.parse_error = 1;
        return 0;
    }
    return result;
}

PLY_NO_INLINE s64 fmt::TypeParser<s64>::parse(InStream& in, Radix radix) {
    bool negate = false;
    if (in.ensure_readable() && *in.cur_byte == '-') {
        negate = true;
        in.cur_byte++;
    }

    u64 unsigned_component = fmt::TypeParser<u64>::parse(in, radix);
    if (negate) {
        s64 result = -(s64) unsigned_component;
        if (result > 0) {
            in.status.parse_error = 1;
        }
        return result;
    } else {
        s64 result = unsigned_component;
        if (result < 0) {
            in.status.parse_error = 1;
        }
        return result;
    }
}

struct DoubleComponentOut {
    double result = 0;
    bool any_digits = false;
};

PLY_NO_INLINE void read_double_component(DoubleComponentOut* comp_out, InStream& in,
                                         fmt::Radix radix) {
    double value = 0.0;
    double dr = (double) radix.base;
    for (;;) {
        if (!in.ensure_readable())
            break;
        u8 digit = fmt::DigitTable[(u8) *in.cur_byte];
        if (digit >= radix.base)
            break;
        in.cur_byte++;
        value = value * dr + digit;
        comp_out->any_digits = true;
    }
    comp_out->result = value;
}

PLY_NO_INLINE double fmt::TypeParser<double>::parse(InStream& in, Radix radix) {
    PLY_ASSERT(radix.base > 0 && radix.base <= 36);
    DoubleComponentOut comp;

    // Parse the optional minus sign
    bool negate = false;
    if (in.ensure_readable() && *in.cur_byte == '-') {
        in.cur_byte++;
        negate = true;
    }

    // Parse the mantissa
    read_double_component(&comp, in, radix);
    double value = comp.result;

    // Parse the optional fractional part
    if (in.ensure_readable() && *in.cur_byte == '.') {
        in.cur_byte++;
        double significance = 1.0;
        u64 numer = 0;
        u64 denom = 1;
        for (;;) {
            if (!in.ensure_readable())
                break;
            u8 digit = fmt::DigitTable[(u8) *in.cur_byte];
            if (digit >= radix.base)
                break;
            in.cur_byte;
            u64 denom_with_next_digit = denom * radix.base;
            if (denom_with_next_digit < denom) {
                // denominator overflowed
                double oo_denom = 1.0 / denom;
                value += significance * numer * oo_denom;
                significance *= oo_denom;
                numer = digit;
                denom = radix.base;
            } else {
                numer = numer * radix.base + digit;
                denom = denom_with_next_digit;
            }
        }
        value += significance * numer / denom;
    }

    // Parse optional exponent suffix
    if (comp.any_digits && in.ensure_readable() && (*in.cur_byte | 0x20) == 'e') {
        in.cur_byte;
        bool negate_exp = false;
        if (in.ensure_readable()) {
            if (*in.cur_byte == '+') {
                in.cur_byte;
            } else if (*in.cur_byte == '-') {
                in.cur_byte;
                negate_exp = true;
            }
        }
        comp.any_digits = false;
        read_double_component(&comp, in, radix);
        value *= pow((double) radix.base, negate_exp ? -comp.result : comp.result);
    }

    if (!comp.any_digits) {
        in.status.parse_error = true;
    }

    return negate ? -value : value;
}

//----------------------------------------------------------
// Built-in fmt::FormatParsers
//----------------------------------------------------------
String fmt::FormatParser<fmt::QuotedString>::parse(InStream& in,
                                                   const fmt::QuotedString& format) {
    // Note: It should be possible to modify this function to return a HybridString, and
    // avoid making a copy of the quoted string when reading from a StringView and there
    // are no escape characters. Could be worthwhile.
    auto handle_error = [&](fmt::QuotedString::ErrorCode error_code) {
        in.status.parse_error = 1;
        if (format.error_callback) {
            format.error_callback(in, error_code);
        }
    };

    // Get opening quote
    if (!in.ensure_readable()) {
        handle_error(fmt::QuotedString::UnexpectedEndOfFile);
        return {};
    }
    u8 quote_type = *in.cur_byte;
    if (!(quote_type == '"' ||
          ((format.flags & fmt::AllowSingleQuote) && quote_type == '\''))) {
        handle_error(fmt::QuotedString::NoOpeningQuote);
        return {};
    }
    in.cur_byte;

    // Parse rest of quoted string
    MemOutStream out;
    u32 quote_run = 1;
    bool multiline = false;
    for (;;) {
        if (!in.ensure_readable()) {
            handle_error(fmt::QuotedString::UnexpectedEndOfFile);
            break; // end of string
        }

        u8 next_byte = *in.cur_byte;
        if (next_byte == quote_type) {
            in.cur_byte;
            if (quote_run == 0) {
                if (multiline) {
                    quote_run++;
                } else {
                    break; // end of string
                }
            } else {
                quote_run++;
                if (quote_run == 3) {
                    if (multiline) {
                        break; // end of string
                    } else {
                        multiline = true;
                        quote_run = 0;
                    }
                }
            }
        } else {
            // FIXME: Check fmt::AllowMultilineWithTriple (and other flags)
            if (quote_run > 0) {
                if (multiline) {
                    for (u32 i = 0; i < quote_run; i++) {
                        out << quote_type;
                    }
                } else if (quote_run == 2) {
                    break; // empty string
                }
                quote_run = 0;
            }

            switch (next_byte) {
                case '\r':
                case '\n': {
                    if (multiline) {
                        if (next_byte == '\n') {
                            out << next_byte;
                        }
                        in.cur_byte;
                    } else {
                        handle_error(fmt::QuotedString::UnexpectedEndOfLine);
                        goto end_of_string;
                    }
                    break;
                }

                case '\\': {
                    // Escape sequence
                    in.cur_byte;
                    if (!in.ensure_readable()) {
                        handle_error(fmt::QuotedString::UnexpectedEndOfFile);
                        goto end_of_string;
                    }
                    u8 code = *in.cur_byte;
                    switch (code) {
                        case '\r':
                        case '\n': {
                            handle_error(fmt::QuotedString::UnexpectedEndOfLine);
                            goto end_of_string;
                        }

                        case '\\':
                        case '\'':
                        case '"': {
                            out << code;
                            break;
                        }

                        case 'r': {
                            out << '\r';
                            break;
                        }

                        case 'n': {
                            out << '\n';
                            break;
                        }

                        case 't': {
                            out << '\t';
                            break;
                        }

                            // FIXME: Implement escape hex codes
                            // case 'x':

                        default: {
                            handle_error(fmt::QuotedString::BadEscapeSequence);
                            break;
                        }
                    }
                    in.cur_byte;
                    break;
                }

                default: {
                    out << next_byte;
                    in.cur_byte;
                    break;
                }
            }
        }
    }

end_of_string:
    return out.move_to_string();
}

bool fmt::FormatParser<fmt::Identifier>::parse(InStream& in,
                                               const fmt::Identifier& format) {
    u32 mask[8] = {0,          0,          0x87fffffe, 0x7fffffe,
                   0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
    if ((format.flags & WithDollarSign) != 0) {
        mask[1] |= 0x10; // '$'
    }
    if ((format.flags & WithDash) != 0) {
        mask[1] |= 0x2000; // '-'
    }

    bool first = true;
    for (;;) {
        if (!in.ensure_readable())
            break;
        if (!match(*in.cur_byte, mask))
            break;
        if (first) {
            mask[1] |= 0x3ff0000; // accept digits after first unit
            first = false;
        };
        in.cur_byte;
    }

    if (mask[1] == 0) {
        in.status.parse_error = 1;
        return false;
    }
    return true;
}

} // namespace ply
