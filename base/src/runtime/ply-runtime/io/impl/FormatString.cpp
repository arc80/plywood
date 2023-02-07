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
inline char to_digit(u32 d) {
    const char* digit_table = "0123456789abcdefghijklmnopqrstuvwxyz";
    return (d <= 35) ? digit_table[d] : '?';
}

void print_string(OutStream& out, u64 value, u32 radix) {
    PLY_ASSERT(radix >= 2);
    char digit_buffer[64];
    s32 digit_index = PLY_STATIC_ARRAY_SIZE(digit_buffer);

    if (value == 0) {
        digit_buffer[--digit_index] = '0';
    } else {
        while (value > 0) {
            u64 quotient = value / radix;
            u32 digit = u32(value - quotient * radix);
            PLY_ASSERT(digit_index > 0);
            digit_buffer[--digit_index] = to_digit(digit);
            value = quotient;
        }
    }

    out << StringView{digit_buffer + digit_index,
                      (u32) PLY_STATIC_ARRAY_SIZE(digit_buffer) - digit_index};
}

void print_string(OutStream& out, s64 value, u32 radix) {
    if (value >= 0) {
        print_string(out, (u64) value, radix);
    } else {
        out << '-';
        print_string(out, (u64) -value, radix);
    }
}

void print_string(OutStream& out, double value, u32 radix) {
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
            u64 fixed_point = u64(value * radix3);
            print_string(out, fixed_point / radix3, radix);
            out << '.';
            u64 fractional_part = fixed_point % radix3;
            {
                // Print zeroed
                char digit_buffer[3];
                for (s32 i = 2; i >= 0; i--) {
                    u64 quotient = fractional_part / radix;
                    u32 digit = u32(fractional_part - quotient * radix);
                    digit_buffer[i] = to_digit(digit);
                    fractional_part = quotient;
                }
                out << StringView{digit_buffer, PLY_STATIC_ARRAY_SIZE(digit_buffer)};
            }
        } else {
            // Scientific notation
            double log_base = log(value) / log(radix);
            double exponent = floor(log_base);
            double m = value / pow(radix, exponent); // mantissa (initially)
            s32 digit = clamp<s32>((s32) floor(m), 1, radix - 1);
            out << to_digit(digit);
            out << '.';
            for (u32 i = 0; i < 3; i++) {
                m = (m - digit) * radix;
                digit = clamp<s32>((s32) floor(m), 0, radix - 1);
                out << to_digit(digit);
            }
            out << 'e';
            print_string(out, (s64) exponent, radix);
        }
    }
}

//----------------------------------------------------------------
// OutStream
//----------------------------------------------------------------
void OutStream::format_args(StringView fmt, ArrayView<const FormatArg> args) {
    u32 arg_index = 0;
    while (fmt.num_bytes > 0) {
        if (fmt[0] == '{') {
            fmt.offset_head(1);
            if (fmt.num_bytes == 0) {
                PLY_ASSERT(0); // Invalid format string!
                break;
            }
            if (fmt[0] == '{') {
                *this << '{';
            } else if (fmt[0] == '}') {
                PLY_ASSERT(
                    arg_index <
                    args.num_items); // Not enough arguments provided for format string!
                args[arg_index].print(*this);
                arg_index++;
            } else {
                PLY_ASSERT(0); // Invalid format string!
            }
        } else if (fmt[0] == '}') {
            fmt.offset_head(1);
            if (fmt.num_bytes == 0) {
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
        fmt.offset_head(1);
    }
    PLY_ASSERT(arg_index ==
               args.num_items); // Too many arguments provided for format string!
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
            print_string(out, arg.s64_, arg.radix);
            break;

        case U64:
            print_string(out, arg.u64_, arg.radix);
            break;

        case Double:
            print_string(out, arg.double_, arg.radix);
            break;
    }
}

void escape::do_print(OutStream& out, const FormatArg& arg) {
    PLY_ASSERT(arg.type == View);
    StringView src_units = arg.view;
    u32 points = 0;
    while (src_units.num_bytes > 0) {
        char c = src_units.bytes[0];
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
        src_units.offset_head(1);
    }
}

void xml_escape::do_print(OutStream& out, const FormatArg& arg) {
    PLY_ASSERT(arg.type == View);
    StringView src_units = arg.view;
    u32 points = 0;
    while (src_units.num_bytes > 0) {
        char c = src_units.bytes[0];
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
        src_units.offset_head(1);
    }
}

void CmdLineArg_WinCrt::do_print(OutStream& out, const FormatArg& arg) {
    PLY_ASSERT(arg.type == View);
    bool needs_quote = false;
    const char* end = arg.view.end();
    for (const char* cur = arg.view.bytes; cur < end; cur++) {
        if (is_white(*cur) || *cur == '"') {
            needs_quote = true;
            break;
        }
    }
    if (needs_quote) {
        out << '"';
        u32 backslash_count = 0;
        for (const char* cur = arg.view.bytes; cur < end; cur++) {
            char c = *cur;
            if (c == '\\') {
                backslash_count++;
            } else if (*cur == '"') {
                for (; backslash_count > 0; backslash_count--) {
                    out << "\\\\";
                }
                out << "\\\"";
            } else {
                for (; backslash_count > 0; backslash_count--) {
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
