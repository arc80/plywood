/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/text/TextFormat.h>
#include <ply-runtime/io/text/NewLineFilter.h>
#include <ply-runtime/io/InStream.h>
#include <ply-runtime/io/OutStream.h>

#include <ply-runtime/io/StdIO.h>

namespace ply {

PLY_NO_INLINE TextFormat TextFormat::platformPreference() {
    TextFormat tff;
#if PLY_TARGET_WIN32
    tff.newLine = TextFormat::NewLine::CRLF;
#endif
    return tff;
}

struct TextFileStats {
    u32 numPoints = 0;
    u32 numValidPoints = 0;
    u32 totalPointValue =
        0; // This value won't be accurate if byte encoding is detected
    u32 numLines = 0;
    u32 numCRLF = 0;
    u32 numControl = 0; // non-whitespace points < 32, including nulls
    u32 numNull = 0;
    u32 numPlainAscii = 0; // includes whitespace, excludes control characters < 32
    u32 numWhitespace = 0;
    u32 numExtended = 0;
    float ooNumPoints = 0.f;

    PLY_INLINE u32 numInvalidPoints() const {
        return this->numPoints - this->numValidPoints;
    }
    PLY_INLINE TextFormat::NewLine getNewLineType() const {
        PLY_ASSERT(this->numCRLF <= this->numLines);
        if (this->numCRLF == 0 || this->numCRLF * 2 < this->numLines) {
            return TextFormat::NewLine::LF;
        } else {
            return TextFormat::NewLine::CRLF;
        }
    }
    PLY_INLINE float getScore() const {
        return (2.5f * this->numWhitespace + this->numPlainAscii -
                100.f * this->numInvalidPoints() - 50.f * this->numControl +
                5.f * this->numExtended) *
               this->ooNumPoints;
    }
};

PLY_NO_INLINE u32 scanTextFile(TextFileStats* stats, InStream& in, Unicode& decoder,
                               u32 maxBytes) {
    bool prevWasCR = false;
    while (in.get_seek_pos() < maxBytes) {
        s32 codepoint = decoder.decode_point(in);
        if (codepoint < 0)
            break; // EOF/error
        stats->numPoints++;
        if (decoder.status == DS_OK) {
            stats->numValidPoints++;
            stats->totalPointValue += codepoint;
            if (codepoint < 32) {
                if (codepoint == '\n') {
                    stats->numPlainAscii++;
                    stats->numLines++;
                    stats->numWhitespace++;
                    if (prevWasCR) {
                        stats->numCRLF++;
                    }
                } else if (codepoint == '\t') {
                    stats->numPlainAscii++;
                    stats->numWhitespace++;
                } else if (codepoint == '\r') {
                    stats->numPlainAscii++;
                } else {
                    stats->numControl++;
                    if (codepoint == 0) {
                        stats->numNull++;
                    }
                }
            } else if (codepoint < 127) {
                stats->numPlainAscii++;
                if (codepoint == ' ') {
                    stats->numWhitespace++;
                }
            } else if (codepoint >= 65536) {
                stats->numExtended++;
            }
        }
        prevWasCR = (codepoint == '\r');
    }
    if (stats->numPoints > 0) {
        stats->ooNumPoints = 1.f / stats->numPoints;
    }
    return in.get_seek_pos();
}

PLY_NO_INLINE TextFormat guessFileEncoding(InStream& in) {
    TextFileStats stats8;
    BlockList::Ref start = in.get_save_point();

    // Try UTF8 first:
    u32 numBytesRead =
        scanTextFile(&stats8, in, Unicode{UTF8}, TextFormat::NumBytesForAutodetect);
    if (numBytesRead == 0) {
        // Empty file
        return {UTF8, TextFormat::NewLine::LF, false};
    }
    in.rewind(start);
    if (stats8.numInvalidPoints() == 0 && stats8.numControl == 0) {
        // No UTF-8 encoding errors, and no weird control characters/nulls. Pick UTF-8.
        return {UTF8, stats8.getNewLineType(), false};
    }

    // If more than 20% of the high bytes in UTF-8 are encoding errors, reinterpret
    // UTF-8 as just bytes.
    UnicodeType encoding8 = UTF8;
    {
        u32 numHighBytes = numBytesRead - stats8.numPlainAscii - stats8.numControl;
        if (stats8.numInvalidPoints() >= numHighBytes * 0.2f) {
            // Too many UTF-8 errors. Consider it bytes.
            encoding8 = NotUnicode;
            stats8.numPoints = numBytesRead;
            stats8.numValidPoints = numBytesRead;
        }
    }

    // Examine both UTF16 endianness:
    TextFileStats stats16_le;
    scanTextFile(&stats16_le, in, Unicode{UTF16_LE}, TextFormat::NumBytesForAutodetect);
    in.rewind(start);

    TextFileStats stats16_be;
    scanTextFile(&stats16_be, in, Unicode{UTF16_BE}, TextFormat::NumBytesForAutodetect);
    in.rewind(start);

    // Choose the better UTF16 candidate:
    TextFileStats* stats = &stats16_le;
    UnicodeType encoding = UTF16_LE;
    if (stats16_be.getScore() > stats16_le.getScore()) {
        stats = &stats16_be;
        encoding = UTF16_BE;
    }

    // Choose between the UTF16 and 8-bit encoding:
    if (stats8.getScore() >= stats->getScore()) {
        stats = &stats8;
        encoding = encoding8;
    }

    // Return best guess
    return {encoding, stats->getNewLineType(), false};
}

PLY_NO_INLINE TextFormat TextFormat::autodetect(InStream& in) {
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
        return guessFileEncoding(in);
    } else {
        // Detect LF or CRLF
        BlockList::Ref start = in.get_save_point();
        TextFileStats stats;
        scanTextFile(&stats, in, Unicode{tff.encoding}, NumBytesForAutodetect);
        in.rewind(start);
        tff.newLine = stats.getNewLineType();
        return tff;
    }
}

//-----------------------------------------------------------------------

Owned<InPipe> TextFormat::createImporter(InStream&& in) const {
    if (this->bom) {
        BlockList::Ref start = in.get_save_point();
        bool gotBom = false;
        switch (this->encoding) {
            case NotUnicode: {
                PLY_ASSERT(0); // Bytes format shouldn't have a BOM
                break;
            }
            case UTF8: {
                char h[3] = {0};
                bool valid = in.read({h, PLY_STATIC_ARRAY_SIZE(h)});
                gotBom = valid && memcmp(h, "\xef\xbb\xbf", 3) == 0;
                break;
            }
            case UTF16_BE: {
                char h[2] = {0};
                bool valid = in.read({h, PLY_STATIC_ARRAY_SIZE(h)});
                gotBom = valid && memcmp(h, "\xfe\xff", 2) == 0;
                break;
            }
            case UTF16_LE: {
                char h[2] = {0};
                bool valid = in.read({h, PLY_STATIC_ARRAY_SIZE(h)});
                gotBom = valid && memcmp(h, "\xff\xfe", 2) == 0;
                break;
            }
        }
        if (!gotBom) {
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
    return createInNewLineFilter(std::move(importer));
}

PLY_NO_INLINE Owned<OutPipe> TextFormat::createExporter(OutStream&& out) const {
    OutStream exporter = std::move(out);

    switch (this->encoding) {
        case NotUnicode: { // FIXME: Bytes needs to be converted
            break;
        }

        case UTF8: {
            if (this->bom) {
                exporter.write({"\xef\xbb\xbf", 3});
            }
            break;
        }

        case UTF16_BE: {
            if (this->bom) {
                exporter.write({"\xfe\xff", 2});
            }
            exporter = {new OutPipe_ConvertUnicode{std::move(exporter), UTF16_BE},
                        true};
            break;
        }

        case UTF16_LE: {
            if (this->bom) {
                exporter.write({"\xff\xfe", 2});
            }
            exporter = {new OutPipe_ConvertUnicode{std::move(exporter), UTF16_LE},
                        true};
            break;
        }
    }

    return createOutNewLineFilter(std::move(exporter), this->newLine == NewLine::CRLF);
}

} // namespace ply
