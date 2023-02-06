/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>

namespace ply {

bool Unicode::encode_point(OutStream& out, u32 codepoint) {
    if (this->type == NotUnicode) {
        // ┏━━━━━━━━━━━━━━┓
        // ┃  NotUnicode  ┃
        // ┗━━━━━━━━━━━━━━┛
        s32 c;
        if (this->ext_params) {
            // Use lookup table.
            if (u8* value = this->ext_params->reverse_lut.find(codepoint)) {
                c = *value;
            } else {
                c = this->ext_params->missing_char;
            }
        } else {
            // Encode this codepoint directly as a byte.
            c = max<s32>(codepoint, 255);
        }
        if (c < 0)
            return true; // Optionally skip unrepresentable character.
        out << (char) c;

    } else if (this->type == UTF8) {
        // ┏━━━━━━━━┓
        // ┃  UTF8  ┃
        // ┗━━━━━━━━┛
        if (codepoint < 0x80) {
            // 1-byte encoding: 0xxxxxxx
            out << char(codepoint);
        } else if (codepoint < 0x800) {
            // 2-byte encoding: 110xxxxx 10xxxxxx
            out << char(0xc0 | (codepoint >> 6));
            out << char(0x80 | (codepoint & 0x3f));
        } else if (codepoint < 0x10000) {
            // 3-byte encoding: 1110xxxx 10xxxxxx 10xxxxxx
            out << char(0xe0 | (codepoint >> 12));
            out << char(0x80 | ((codepoint >> 6) & 0x3f));
            out << char(0x80 | ((codepoint & 0x3f)));
        } else {
            // 4-byte encoding: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            out << char(0xf0 | (codepoint >> 18));
            out << char(0x80 | ((codepoint >> 12) & 0x3f));
            out << char(0x80 | ((codepoint >> 6) & 0x3f));
            out << char(0x80 | (codepoint & 0x3f));
        }

    } else if (this->type == UTF16_Native) {
        // ┏━━━━━━━━━━━━━━━━┓
        // ┃  UTF16_Native  ┃
        // ┗━━━━━━━━━━━━━━━━┛
        if (codepoint < 0x10000) {
            // Note: 0xd800 to 0xd8ff are invalid Unicode codepoints reserved for UTF-16
            // surrogates. Such codepoints will simply be written as unpaired
            // surrogates.
            out.raw_write(u16(codepoint));
        } else {
            // Codepoints >= 0x10000 are encoded as a pair of surrogate units.
            u32 adjusted = codepoint - 0x10000;
            out.raw_write(u16(0xd800 + ((adjusted >> 10) & 0x3ff)));
            out.raw_write(u16(0xdc00 + (adjusted & 0x3ff)));
        }

    } else if (this->type == UTF16_Reversed) {
        // ┏━━━━━━━━━━━━━━━━━━┓
        // ┃  UTF16_Reversed  ┃
        // ┗━━━━━━━━━━━━━━━━━━┛
        if (codepoint < 0x10000) {
            // Note: 0xd800 to 0xd8ff are invalid Unicode codepoints reserved for UTF-16
            // surrogates. Such codepoints will simply be written as unpaired
            // surrogates.
            out.raw_write(reverse_bytes(u16(codepoint)));
        } else {
            // Codepoints >= 0x10000 are encoded as a pair of surrogate units.
            u32 adjusted = codepoint - 0x10000;
            out.raw_write(reverse_bytes(u16(0xd800 + ((adjusted >> 10) & 0x3ff))));
            out.raw_write(reverse_bytes(u16(0xdc00 + (adjusted & 0x3ff))));
        }

    } else {
        // Shouldn't get here.
        PLY_ASSERT(0);
    }

    return !out.at_eof();
}

s32 Unicode::decode_point(InStream& in) {
    if (!in.ensure_readable()) {
        this->status = DS_NotEnoughData;
        return -1;
    }

    this->status = DS_OK;
    if (this->type == NotUnicode) {
        // ┏━━━━━━━━━━━━━━┓
        // ┃  NotUnicode  ┃
        // ┗━━━━━━━━━━━━━━┛
        u8 b = in.read_byte();
        if (this->ext_params) // Use lookup table if available.
            return this->ext_params->lut[b];
        return b;

    } else if (this->type == UTF8) {
        // ┏━━━━━━━━┓
        // ┃  UTF8  ┃
        // ┗━━━━━━━━┛
        // (Note: Ill-formed encodings are interpreted as sequences of individual
        // bytes.)
        s32 value = 0;
        u32 num_continuation_bytes = 0;
        u8 b = in.read_byte();
        auto save_point = in.get_save_point();

        if (b < 0x80)
            // 1-byte encoding: 0xxxxxxx
            return b;
        else if (b < 0xc0) {
            // Unexpected continuation byte: 10xxxxxx
            this->status = DS_IllFormed;
            return b;
        } else if (b < 0xe0) {
            // 2-byte encoding: 110xxxxx 10xxxxxx
            value = b & 0x1f;
            num_continuation_bytes = 1;
        } else if (b < 0xf0) {
            // 3-byte encoding: 1110xxxx 10xxxxxx 10xxxxxx
            value = b & 0xf;
            num_continuation_bytes = 2;
        } else if (b < 0xf8) {
            // 4-byte encoding: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            value = b & 0x7;
            num_continuation_bytes = 3;
        } else {
            // Illegal byte.
            this->status = DS_IllFormed;
            return b;
        }

        while (num_continuation_bytes > 0) {
            if (!in.ensure_readable()) {
                // Illegal encoding: Not enough continuation bytes.
                in.rewind(save_point);
                this->status = DS_NotEnoughData;
                return b; // Return single byte.
            }
            value = (value << 6) | (in.read_byte() & 0x3f);
            num_continuation_bytes--;
        }

        return value;

    } else if (this->type == UTF16_Native) {
        // ┏━━━━━━━━━━━━━━━━┓
        // ┃  UTF16_Native  ┃
        // ┗━━━━━━━━━━━━━━━━┛
        u16 first = in.raw_read<u16>();
        if (in.at_eof()) {
            this->status = DS_NotEnoughData;
            return -1; // Not enough data.
        }

        auto save_point = in.get_save_point();
        if (first >= 0xd800 && first < 0xdc00) {
            u16 second = in.raw_read<u16>();
            if (in.at_eof()) {
                // A second 16-bit surrogate is expected, but not enough data.
                this->status = DS_NotEnoughData;
                in.rewind(save_point);
                return first;
            }
            if (second >= 0xdc00 && second < 0xe000)
                // We got a valid pair of 16-bit surrogates.
                return 0x10000 + ((first - 0xd800) << 10) + (second - 0xdc00);

            // Unpaired surrogate.
            this->status = DS_IllFormed;
            in.rewind(save_point);
            return first;
        }

        // It's a single 16-bit unit.
        return first;

    } else if (this->type == UTF16_Reversed) {
        // ┏━━━━━━━━━━━━━━━━━━┓
        // ┃  UTF16_Reversed  ┃
        // ┗━━━━━━━━━━━━━━━━━━┛
        u16 first = reverse_bytes(in.raw_read<u16>());
        if (in.at_eof()) {
            this->status = DS_NotEnoughData;
            return -1; // Not enough data.
        }

        auto save_point = in.get_save_point();
        if (first >= 0xd800 && first < 0xdc00) {
            u16 second = reverse_bytes(in.raw_read<u16>());
            if (in.at_eof()) {
                // A second 16-bit surrogate is expected, but not enough data.
                this->status = DS_NotEnoughData;
                in.rewind(save_point);
                return first;
            }
            if (second >= 0xdc00 && second < 0xe000)
                // We got a valid pair of 16-bit surrogates.
                return 0x10000 + ((first - 0xd800) << 10) + (second - 0xdc00);

            // Unpaired surrogate.
            this->status = DS_IllFormed;
            in.rewind(save_point);
            return first;
        }

        // It's a single 16-bit unit.
        return first;
    }

    PLY_ASSERT(0); // Shouldn't get here.
    return 0;
}

// ┏━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃  InPipe_ConvertUnicode  ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━┛
bool copy_from_shim(OutStream& dst_out, StringView& shim_used) {
    if (shim_used) {
        u32 n = min(dst_out.num_writable_bytes(), shim_used.num_bytes);
        dst_out << shim_used;
        shim_used.offset_head(n);
        if (shim_used)
            return true; // Destination buffer is full.
    }
    shim_used = {};
    return false;
}

// Fill dst_buf with UTF-8-encoded data.
u32 InPipe_ConvertUnicode::read(MutStringView dst_buf) {
    ViewOutStream dst_out{dst_buf};

    // If the shim contains data, copy it first.
    if (copy_from_shim(dst_out, this->shim_used))
        return dst_buf.num_bytes; // Destination buffer is full.

    while (true) {
        // Decode a codepoint from input stream.
        s32 codepoint = this->src_enc.decode_point(this->in);
        if (codepoint < 0)
            break; // Reached EOF.

        // Convert codepoint to UTF-8.
        u32 w = dst_out.num_writable_bytes();
        if (w >= 4) {
            Unicode{UTF8}.encode_point(dst_out, codepoint);
        } else {
            // Use shim as an intermediate buffer.
            ViewOutStream s{this->shim_storage.mutable_string_view()};
            Unicode{UTF8}.encode_point(s, codepoint);
            this->shim_used = {s.start_byte,
                               safe_demote<u32>(s.cur_byte - s.start_byte)};
            if (copy_from_shim(dst_out, this->shim_used))
                break; // Destination buffer is full.
        }
    }

    return safe_demote<u32>(dst_out.get_seek_pos());
}

// ┏━━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃  OutPipe_ConvertUnicode  ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━━┛
// src_buf expects UTF-8-encoded data.
bool OutPipe_ConvertUnicode::write(StringView src_buf) {
    ViewInStream src_in{src_buf};

    // If the shim contains data, join it with the source buffer.
    if (this->shim_used > 0) {
        u32 num_bytes_appended = min(src_buf.num_bytes, 4 - this->shim_used);
        memcpy(this->shim_storage + this->shim_used, src_buf.bytes, num_bytes_appended);
        this->shim_used += num_bytes_appended;

        // Decode a codepoint from the shim using UTF-8.
        ViewInStream s{{this->shim_storage, this->shim_used}};
        Unicode decoder{UTF8};
        s32 codepoint = decoder.decode_point(s);
        if (decoder.status == DS_NotEnoughData) {
            PLY_ASSERT(num_bytes_appended == src_buf.num_bytes);
            return true; // Not enough data available in shim.
        }

        // Convert codepoint to the destination encoding.
        this->dst_enc.encode_point(this->child_stream, codepoint);

        // Skip ahead in the source buffer and clear the shim.
        src_in.cur_byte += num_bytes_appended;
        this->shim_used = 0;
    }

    while (!this->child_stream.at_eof()) {
        // Decode a codepoint from the source buffer using UTF-8.
        Unicode decoder{UTF8};
        s32 codepoint = decoder.decode_point(src_in);
        if (decoder.status == DS_NotEnoughData) {
            // Not enough data available. Copy the rest of the source buffer to shim,
            // including the previous byte consumed by decode().
            this->shim_used = src_in.num_bytes_readable() + 1;
            PLY_ASSERT(this->shim_used < 4);
            memcpy(this->shim_storage, src_in.cur_byte - 1, this->shim_used);
            return true;
        }

        // Convert codepoint to the destination encoding.
        this->dst_enc.encode_point(this->child_stream, codepoint);
    }

    return false; // We reached the end of the OutStream.
}

void OutPipe_ConvertUnicode::flush(bool hard) {
    // The shim may still contain an incomplete (thus invalid) UTF-8 sequence.
    for (u32 i = 0; i < this->shim_used; i++) {
        // Interpret each byte as a separate codepoint.
        this->dst_enc.encode_point(this->child_stream, this->shim_storage[i]);
    }
    this->shim_used = 0;

    // Forward flush command down the output chain.
    this->child_stream.flush(hard);
}

} // namespace ply
