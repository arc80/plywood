/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/text/TextFormat.h>
#include <ply-runtime/io/text/TextConverter.h>
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

const TextEncoding* encodingFromEnum(TextFormat::Encoding enc) {
    switch (enc) {
        default:
            PLY_ASSERT(0);
        case TextFormat::Encoding::Bytes:
            return TextEncoding::get<Enc_Bytes>();
        case TextFormat::Encoding::UTF8:
            return TextEncoding::get<UTF8>();
        case TextFormat::Encoding::UTF16_be:
            return TextEncoding::get<UTF16<true>>();
        case TextFormat::Encoding::UTF16_le:
            return TextEncoding::get<UTF16<false>>();
    }
};

PLY_NO_INLINE u32 scanTextFile(TextFileStats* stats, InStream& in,
                               const TextEncoding* encoding, u32 maxBytes) {
    bool prevWasCR = false;
    u32 numBytes = 0;
    while (numBytes < maxBytes) {
        in.ensure_contiguous(4);
        DecodeResult decoded = encoding->decodePoint(in.view_readable());
        if (decoded.status == DecodeResult::Status::Truncated)
            break; // EOF/error
        PLY_ASSERT(decoded.point >= 0 && decoded.numBytes > 0);
        in.cur_byte += decoded.numBytes;
        numBytes += decoded.numBytes;
        stats->numPoints++;
        if (decoded.status == DecodeResult::Status::Valid) {
            stats->numValidPoints++;
            stats->totalPointValue += decoded.point;
            if (decoded.point < 32) {
                if (decoded.point == '\n') {
                    stats->numPlainAscii++;
                    stats->numLines++;
                    stats->numWhitespace++;
                    if (prevWasCR) {
                        stats->numCRLF++;
                    }
                } else if (decoded.point == '\t') {
                    stats->numPlainAscii++;
                    stats->numWhitespace++;
                } else if (decoded.point == '\r') {
                    stats->numPlainAscii++;
                } else {
                    stats->numControl++;
                    if (decoded.point == 0) {
                        stats->numNull++;
                    }
                }
            } else if (decoded.point < 127) {
                stats->numPlainAscii++;
                if (decoded.point == ' ') {
                    stats->numWhitespace++;
                }
            } else if (decoded.point >= 65536) {
                stats->numExtended++;
            }
        }
        prevWasCR = (decoded.point == '\r');
    }
    if (stats->numPoints > 0) {
        stats->ooNumPoints = 1.f / stats->numPoints;
    }
    return numBytes;
}

PLY_NO_INLINE TextFormat guessFileEncoding(InStream& in) {
    TextFileStats stats8;
    BlockList::Ref start = in.getBlockRef();

    // Try UTF8 first:
    u32 numBytesRead = scanTextFile(&stats8, in, TextEncoding::get<UTF8>(),
                                    TextFormat::NumBytesForAutodetect);
    if (numBytesRead == 0) {
        // Empty file
        return {TextFormat::Encoding::UTF8, TextFormat::NewLine::LF, false};
    }
    in.rewind(start);
    if (stats8.numInvalidPoints() == 0 && stats8.numControl == 0) {
        // No UTF-8 encoding errors, and no weird control characters/nulls. Pick UTF-8.
        return {TextFormat::Encoding::UTF8, stats8.getNewLineType(), false};
    }

    // If more than 20% of the high bytes in UTF-8 are encoding errors, reinterpret
    // UTF-8 as just bytes.
    TextFormat::Encoding encoding8 = TextFormat::Encoding::UTF8;
    {
        u32 numHighBytes = numBytesRead - stats8.numPlainAscii - stats8.numControl;
        if (stats8.numInvalidPoints() >= numHighBytes * 0.2f) {
            // Too many UTF-8 errors. Consider it bytes.
            encoding8 = TextFormat::Encoding::Bytes;
            stats8.numPoints = numBytesRead;
            stats8.numValidPoints = numBytesRead;
        }
    }

    // Examine both UTF16 endianness:
    TextFileStats stats16_le;
    scanTextFile(&stats16_le, in, TextEncoding::get<UTF16_LE>(),
                 TextFormat::NumBytesForAutodetect);
    in.rewind(start);

    TextFileStats stats16_be;
    scanTextFile(&stats16_be, in, TextEncoding::get<UTF16_BE>(),
                 TextFormat::NumBytesForAutodetect);
    in.rewind(start);

    // Choose the better UTF16 candidate:
    TextFileStats* stats = &stats16_le;
    TextFormat::Encoding encoding = TextFormat::Encoding::UTF16_le;
    if (stats16_be.getScore() > stats16_le.getScore()) {
        stats = &stats16_be;
        encoding = TextFormat::Encoding::UTF16_be;
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
    BlockList::Ref start = in.getBlockRef();
    u8 h[3] = {0};
    h[0] = in.read_byte();
    h[1] = in.read_byte();
    if (h[0] == 0xef && h[1] == 0xbb) {
        h[2] = in.read_byte();
        if (h[2] == 0xbf) {
            tff.encoding = TextFormat::Encoding::UTF8;
            tff.bom = true;
        }
    } else if (h[0] == 0xfe && h[1] == 0xff) {
        tff.encoding = TextFormat::Encoding::UTF16_be;
        tff.bom = true;
    } else if (h[0] == 0xff && h[1] == 0xfe) {
        tff.encoding = TextFormat::Encoding::UTF16_le;
        tff.bom = true;
    }
    in.rewind(start);
    if (!tff.bom) {
        return guessFileEncoding(in);
    } else {
        // Detect LF or CRLF
        BlockList::Ref start = in.getBlockRef();
        TextFileStats stats;
        scanTextFile(&stats, in, encodingFromEnum(tff.encoding), NumBytesForAutodetect);
        in.rewind(start);
        tff.newLine = stats.getNewLineType();
        return tff;
    }
}

//-----------------------------------------------------------------------

PLY_NO_INLINE InStream TextFormat::createImporter(InStream&& in) const {
    using Enc = TextFormat::Encoding;
    if (this->bom) {
        BlockList::Ref start = in.getBlockRef();
        bool gotBom = false;
        switch (this->encoding) {
            case Enc::Bytes: {
                PLY_ASSERT(0); // Bytes format shouldn't have a BOM
                break;
            }
            case Enc::UTF8: {
                char h[3] = {0};
                bool valid = in.read({h, PLY_STATIC_ARRAY_SIZE(h)});
                gotBom = valid && memcmp(h, "\xef\xbb\xbf", 3) == 0;
                break;
            }
            case Enc::UTF16_be: {
                char h[2] = {0};
                bool valid = in.read({h, PLY_STATIC_ARRAY_SIZE(h)});
                gotBom = valid && memcmp(h, "\xfe\xff", 2) == 0;
                break;
            }
            case Enc::UTF16_le: {
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
    if (this->encoding == TextFormat::Encoding::UTF8) {
        importer = std::move(in);
    } else {
        importer = {new InPipe_TextConverter{std::move(in), TextEncoding::get<UTF8>(),
                                             encodingFromEnum(this->encoding)},
                    true};
    }

    // Install newline filter (basically just eats \r)
    // FIXME: Some caller might want the LFs to be unchanged.
    return {createInNewLineFilter(std::move(importer)), true};
}

PLY_NO_INLINE OutStream TextFormat::createExporter(OutStream&& out) const {
    OutStream exporter = std::move(out);

    switch (this->encoding) {
        case TextFormat::Encoding::Bytes: { // FIXME: Bytes needs to be converted
            break;
        }

        case TextFormat::Encoding::UTF8: {
            if (this->bom) {
                exporter.write({"\xef\xbb\xbf", 3});
            }
            break;
        }

        case TextFormat::Encoding::UTF16_be: {
            if (this->bom) {
                exporter.write({"\xfe\xff", 2});
            }
            exporter = {new OutPipe_TextConverter{std::move(exporter),
                                                  TextEncoding::get<UTF16_BE>(),
                                                  TextEncoding::get<UTF8>()},
                        true};
            break;
        }

        case TextFormat::Encoding::UTF16_le: {
            if (this->bom) {
                exporter.write({"\xff\xfe", 2});
            }
            exporter = {new OutPipe_TextConverter{std::move(exporter),
                                                  TextEncoding::get<UTF16_LE>(),
                                                  TextEncoding::get<UTF8>()},
                        true};
            break;
        }
    }

    // Install newline filter
    return {createOutNewLineFilter(std::move(exporter),
                                   this->newLine == TextFormat::NewLine::CRLF),
            true};
}

} // namespace ply
