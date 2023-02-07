/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/text/NewLineFilter.h>

namespace ply {

TextFormat TextFormat::default_utf8() {
    TextFormat tff;
#if PLY_TARGET_WIN32
    tff.new_line = TextFormat::NewLine::CRLF;
#endif
    return tff;
}

struct TextFileStats {
    u32 num_points = 0;
    u32 num_valid_points = 0;
    u32 total_point_value =
        0; // This value won't be accurate if byte encoding is detected
    u32 num_lines = 0;
    u32 num_crlf = 0;
    u32 num_control = 0; // non-whitespace points < 32, including nulls
    u32 num_null = 0;
    u32 num_plain_ascii = 0; // includes whitespace, excludes control characters < 32
    u32 num_whitespace = 0;
    u32 num_extended = 0;
    float oo_num_points = 0.f;

    u32 num_invalid_points() const {
        return this->num_points - this->num_valid_points;
    }
    TextFormat::NewLine get_new_line_type() const {
        PLY_ASSERT(this->num_crlf <= this->num_lines);
        if (this->num_crlf == 0 || this->num_crlf * 2 < this->num_lines) {
            return TextFormat::NewLine::LF;
        } else {
            return TextFormat::NewLine::CRLF;
        }
    }
    float get_score() const {
        return (2.5f * this->num_whitespace + this->num_plain_ascii -
                100.f * this->num_invalid_points() - 50.f * this->num_control +
                5.f * this->num_extended) *
               this->oo_num_points;
    }
};

u32 scan_text_file(TextFileStats* stats, InStream& in, Unicode& decoder,
                   u32 max_bytes) {
    bool prev_was_cr = false;
    while (in.get_seek_pos() < max_bytes) {
        s32 codepoint = decoder.decode_point(in);
        if (codepoint < 0)
            break; // EOF/error
        stats->num_points++;
        if (decoder.status == DS_OK) {
            stats->num_valid_points++;
            stats->total_point_value += codepoint;
            if (codepoint < 32) {
                if (codepoint == '\n') {
                    stats->num_plain_ascii++;
                    stats->num_lines++;
                    stats->num_whitespace++;
                    if (prev_was_cr) {
                        stats->num_crlf++;
                    }
                } else if (codepoint == '\t') {
                    stats->num_plain_ascii++;
                    stats->num_whitespace++;
                } else if (codepoint == '\r') {
                    stats->num_plain_ascii++;
                } else {
                    stats->num_control++;
                    if (codepoint == 0) {
                        stats->num_null++;
                    }
                }
            } else if (codepoint < 127) {
                stats->num_plain_ascii++;
                if (codepoint == ' ') {
                    stats->num_whitespace++;
                }
            } else if (codepoint >= 65536) {
                stats->num_extended++;
            }
        }
        prev_was_cr = (codepoint == '\r');
    }
    if (stats->num_points > 0) {
        stats->oo_num_points = 1.f / stats->num_points;
    }
    return in.get_seek_pos();
}

TextFormat guess_file_encoding(InStream& in) {
    TextFileStats stats8;
    BlockList::Ref start = in.get_save_point();

    // Try UTF8 first:
    u32 num_bytes_read =
        scan_text_file(&stats8, in, Unicode{UTF8}, TextFormat::NumBytesForAutodetect);
    if (num_bytes_read == 0) {
        // Empty file
        return {UTF8, TextFormat::NewLine::LF, false};
    }
    in.rewind(start);
    if (stats8.num_invalid_points() == 0 && stats8.num_control == 0) {
        // No UTF-8 encoding errors, and no weird control characters/nulls. Pick UTF-8.
        return {UTF8, stats8.get_new_line_type(), false};
    }

    // If more than 20% of the high bytes in UTF-8 are encoding errors, reinterpret
    // UTF-8 as just bytes.
    UnicodeType encoding8 = UTF8;
    {
        u32 num_high_bytes =
            num_bytes_read - stats8.num_plain_ascii - stats8.num_control;
        if (stats8.num_invalid_points() >= num_high_bytes * 0.2f) {
            // Too many UTF-8 errors. Consider it bytes.
            encoding8 = NotUnicode;
            stats8.num_points = num_bytes_read;
            stats8.num_valid_points = num_bytes_read;
        }
    }

    // Examine both UTF16 endianness:
    TextFileStats stats16_le;
    scan_text_file(&stats16_le, in, Unicode{UTF16_LE},
                   TextFormat::NumBytesForAutodetect);
    in.rewind(start);

    TextFileStats stats16_be;
    scan_text_file(&stats16_be, in, Unicode{UTF16_BE},
                   TextFormat::NumBytesForAutodetect);
    in.rewind(start);

    // Choose the better UTF16 candidate:
    TextFileStats* stats = &stats16_le;
    UnicodeType encoding = UTF16_LE;
    if (stats16_be.get_score() > stats16_le.get_score()) {
        stats = &stats16_be;
        encoding = UTF16_BE;
    }

    // Choose between the UTF16 and 8-bit encoding:
    if (stats8.get_score() >= stats->get_score()) {
        stats = &stats8;
        encoding = encoding8;
    }

    // Return best guess
    return {encoding, stats->get_new_line_type(), false};
}

TextFormat TextFormat::autodetect(InStream& in) {
    TextFormat tff;
    BlockList::Ref start = in.get_save_point();
    u8 h[3] = {0};
    h[0] = in.read_byte();
    h[1] = in.read_byte();
    if (h[0] == 0xef && h[1] == 0xbb) {
        h[2] = in.read_byte();
        if (h[2] == 0xbf) {
            tff.encoding = UTF8;
            tff.bom = true;
        }
    } else if (h[0] == 0xfe && h[1] == 0xff) {
        tff.encoding = UTF16_BE;
        tff.bom = true;
    } else if (h[0] == 0xff && h[1] == 0xfe) {
        tff.encoding = UTF16_LE;
        tff.bom = true;
    }
    in.rewind(start);
    if (!tff.bom) {
        return guess_file_encoding(in);
    } else {
        // Detect LF or CRLF
        BlockList::Ref start = in.get_save_point();
        TextFileStats stats;
        scan_text_file(&stats, in, Unicode{tff.encoding}, NumBytesForAutodetect);
        in.rewind(start);
        tff.new_line = stats.get_new_line_type();
        return tff;
    }
}

//-----------------------------------------------------------------------

Owned<InPipe> TextFormat::create_importer(InStream&& in) const {
    if (this->bom) {
        BlockList::Ref start = in.get_save_point();
        bool got_bom = false;
        switch (this->encoding) {
            case NotUnicode: {
                PLY_ASSERT(0); // Bytes format shouldn't have a BOM
                break;
            }
            case UTF8: {
                char h[3] = {0};
                bool valid = in.read({h, PLY_STATIC_ARRAY_SIZE(h)});
                got_bom = valid && memcmp(h, "\xef\xbb\xbf", 3) == 0;
                break;
            }
            case UTF16_BE: {
                char h[2] = {0};
                bool valid = in.read({h, PLY_STATIC_ARRAY_SIZE(h)});
                got_bom = valid && memcmp(h, "\xfe\xff", 2) == 0;
                break;
            }
            case UTF16_LE: {
                char h[2] = {0};
                bool valid = in.read({h, PLY_STATIC_ARRAY_SIZE(h)});
                got_bom = valid && memcmp(h, "\xff\xfe", 2) == 0;
                break;
            }
        }
        if (!got_bom) {
            // Expected a BOM, but didn't actually encounter one
            // FIXME: Some callers may want to know about this
            in.rewind(start);
        }
    }

    // Install converter from UTF-16 if needed
    InStream importer;
    if (this->encoding == UTF8) {
        importer = std::move(in);
    } else {
        importer = {new InPipe_ConvertUnicode{std::move(in), this->encoding}, true};
    }

    // Install newline filter (basically just eats \r)
    // FIXME: Some caller might want the LFs to be unchanged.
    return create_in_new_line_filter(std::move(importer));
}

Owned<OutPipe> TextFormat::create_exporter(OutStream&& out) const {
    OutStream exporter = std::move(out);

    switch (this->encoding) {
        case NotUnicode: { // FIXME: Bytes needs to be converted
            break;
        }

        case UTF8: {
            if (this->bom) {
                exporter << StringView{"\xef\xbb\xbf", 3};
            }
            break;
        }

        case UTF16_BE: {
            if (this->bom) {
                exporter << StringView{"\xfe\xff", 2};
            }
            exporter = {new OutPipe_ConvertUnicode{std::move(exporter), UTF16_BE},
                        true};
            break;
        }

        case UTF16_LE: {
            if (this->bom) {
                exporter << StringView{"\xff\xfe", 2};
            }
            exporter = {new OutPipe_ConvertUnicode{std::move(exporter), UTF16_LE},
                        true};
            break;
        }
    }

    return create_out_new_line_filter(std::move(exporter),
                                      this->new_line == NewLine::CRLF);
}

} // namespace ply
