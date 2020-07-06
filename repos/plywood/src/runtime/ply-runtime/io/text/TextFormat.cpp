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

namespace ply {

PLY_NO_INLINE TextFormat TextFormat::platformPreference() {
    TextFormat tff;
#if PLY_TARGET_WIN32
    tff.newLine = TextFormat::NewLine::CRLF;
#endif
    return tff;
}

struct TextFileStats {
    u32 numPoints = 0; // FIXME: Change this to bytes
    u32 numValidPoints = 0;
    u32 totalPointValue = 0; // This value won't be accurate if byte encoding is detected
    u32 numLines = 0;
    u32 numCRLF = 0;
    u32 numControl = 0; // non-CRLF points < 32, including nulls
    u32 numNull = 0;
    u32 numPlainAscii = 0; // includes whitespace
    u32 numWhitespace = 0;
    float ooNumPoints = 0.f;

    PLY_INLINE float getAsciiRate() const {
        return this->numPlainAscii * this->ooNumPoints;
    }
    PLY_INLINE float getWhitespaceRate() const {
        return this->numWhitespace * this->ooNumPoints;
    }
    PLY_INLINE u32 numInvalidPoints() const {
        return this->numPoints - this->numValidPoints;
    }
    PLY_INLINE float getControlCodeRate() const {
        return float(this->numControl) / this->numPoints;
    }
};

PLY_NO_INLINE u32 scanTextFile(TextFileStats* stats, InStream* ins, const TextEncoding* encoding,
                               u32 maxBytes) {
    bool prevWasCR = false;
    u32 numBytes = 0;
    while (numBytes < maxBytes) {
        u32 numBytesAvailable = ins->tryMakeBytesAvailable(4); // returns < 4 only EOF/error *ONLY*
        if (numBytesAvailable == 0)
            break;
        DecodeResult decoded = encoding->decodePoint(ins->viewAvailable());
        PLY_ASSERT(decoded.numBytes > 0);
        ins->curByte += decoded.numBytes;
        numBytes += decoded.numBytes;
        stats->numPoints++;
        if (decoded.status == DecodeResult::Status::Valid) {
            stats->numValidPoints++;
            stats->totalPointValue += decoded.point;
            if (decoded.point < 32) {
                if (decoded.point == '\n') {
                    stats->numLines++;
                    stats->numWhitespace++;
                    if (prevWasCR) {
                        stats->numCRLF++;
                    }
                } else if (decoded.point == '\t') {
                    stats->numPlainAscii++;
                    stats->numWhitespace++;
                } else if (decoded.point != '\r') {
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
            }
        }
        prevWasCR = (decoded.point == '\r');
    }
    if (stats->numPoints > 0) {
        stats->ooNumPoints = 1.f / stats->numPoints;
    }
    return numBytes;
}

struct TextFileSurvey {
    TextFileStats utf8;
    TextFileStats utf16_le;
    TextFileStats utf16_be;
};

PLY_NO_INLINE TextFormat::Encoding guessFileEncoding(InStream* ins) {
    TextFileStats stats_;
    TextFileStats* stats = &stats_;
    ChunkCursor start = ins->getCursor();

    // Try UTF8 first:
    u32 numBytesRead = scanTextFile(stats, ins, TextEncoding::get<UTF8>(), 4000);
    if (numBytesRead == 0) {
        // Empty file
        return TextFormat::Encoding::UTF8;
    }
    ins->rewind(start);
    if (stats->numInvalidPoints() == 0 && stats->numControl == 0) {
        // No UTF-8 encoding errors, and no weird control characters/nulls. Pick UTF-8.
        return TextFormat::Encoding::UTF8;
    }

    // Examine both UTF16 endianness:
    TextFileStats utf16_le;
    scanTextFile(&utf16_le, ins, TextEncoding::get<UTF16_LE>(), 4000);
    ins->rewind(start);

    TextFileStats utf16_be;
    scanTextFile(&utf16_be, ins, TextEncoding::get<UTF16_BE>(), 4000);
    ins->rewind(start);

    // Choose the better UTF16 candidate:
    TextFileStats* utf16 = nullptr;
    s32 validPointsDiff = utf16_le.numValidPoints - utf16_be.numValidPoints;
    if (validPointsDiff > 0) {
        utf16 = &utf16_le;
    } else if (validPointsDiff < 0) {
        utf16 = &utf16_be;
    } else {
        // In case of a tie, if either encoding has more than 80% ASCII characters, choose that
        // one.
        if (utf16_le.getAsciiRate() > 0.8f) {
            utf16 = &utf16_le;
        } else if (utf16_be.getAsciiRate() > 0.8f) {
            utf16 = &utf16_be;
        } else {
            // Both have a significant number of non-ASCII characters.
            // Choose the one with more whitespace characters.
            s32 fmtDiff = utf16_le.numWhitespace - utf16_be.numWhitespace;
            if (fmtDiff > 0) {
                utf16 = &utf16_le;
            } else if (fmtDiff < 0) {
                utf16 = &utf16_be;
            } else {
                // In case of a tie, choose the one with fewer control codes.
                s32 ccDiff = utf16_le.numControl - utf16_be.numControl;
                if (ccDiff > 0) {
                    utf16 = &utf16_le;
                } else if (ccDiff < 0) {
                    utf16 = &utf16_be;
                } else {
                    // This could be eg. a bunch of non-ASCII characters on an unbroken line.
                    // In this case, the tie is broken by the encoding with lower average code
                    // point. totalPointValue is a proxy for the average code point value.
                    s32 totalDiff = utf16_le.totalPointValue;
                    if (totalDiff > 0) {
                        utf16 = &utf16_le;
                    } else {
                        utf16 = &utf16_be;
                    }
                }
            }
        }
    }

    // Decide whether to interpret UTF-8 as just bytes:
    // If it's mostly ASCII, and more than 50% of the high bytes were encoding errors, don't
    // consider it UTF-8. (The caller can always override this decision and just use UTF-8 when
    // bytes is detected.)
    TextFormat::Encoding encoding = TextFormat::Encoding::UTF8;
    float utf8AsciiRate = float(stats->numPlainAscii) /
                          numBytesRead; // FIXME: Change numPoints to numBytes and use that here
    u32 numHighBytes = stats->numPlainAscii + stats->numControl;
    if (utf8AsciiRate > 0.8f && float(stats->numInvalidPoints()) > numHighBytes * 0.5f) {
        encoding = TextFormat::Encoding::Bytes;
        stats->numPoints = numBytesRead;
        stats->numValidPoints = numBytesRead;
    }

    // Choose between the UTF16 and 8-bit encoding:
    bool useUTF16 = false;
    if (utf8AsciiRate > 0.9f) {
        // If it's mostly ASCII, and less than 50% of non-ASCII are control codes, stick with
        // 8-bit
        if (stats->numControl > stats->numPlainAscii * 0.5f) {
            useUTF16 = true;
        }
    } else {
        // If it isn't mostly ASCII, and there are any control codes at all, use UTF16.
        // This codepath will get hit when ASCII characters are encoded as UTF16.
        if (stats->numControl > 0) {
            useUTF16 = true;
        } else {
            // Choose UTF16 if it has a lower error rate.
            if (utf16->numInvalidPoints() * 2 < stats->numInvalidPoints()) {
                useUTF16 = true;
            }
        }
    }

    // Set return values if UTF16 was chosen:
    if (useUTF16) {
        *stats = *utf16;
        encoding =
            (utf16 == &utf16_le) ? TextFormat::Encoding::UTF16_le : TextFormat::Encoding::UTF16_be;
    }

    return encoding;
}

PLY_NO_INLINE TextFormat TextFormat::autodetect(InStream* ins) {
    TextFormat tff;
    ChunkCursor start = ins->getCursor();
    u8 h[3] = {0};
    h[0] = ins->readByte();
    h[1] = ins->readByte();
    if (h[0] == 0xef && h[1] == 0xbb) {
        h[2] = ins->readByte();
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
    ins->rewind(start);
    if (!tff.bom) {
        TextFormat::Encoding enc = guessFileEncoding(ins); // automatically rewinds it
        tff.encoding = enc;
        // FIXME CRLF
    }
    // Detect LF or CRLF
    // *******FIXME*******
#if PLY_TARGET_WIN32
    tff.newLine = TextFormat::NewLine::CRLF; // default in case no CRLF appears
#endif
    return tff;
}

//-----------------------------------------------------------------------

PLY_NO_INLINE Owned<StringReader>
TextFormat::createImporter(OptionallyOwned<InStream>&& ins) const {
    using Enc = TextFormat::Encoding;
    if (this->bom) {
        ChunkCursor start = ins->getCursor();
        bool gotBom = false;
        switch (this->encoding) {
            case Enc::Bytes: {
                PLY_ASSERT(0); // Bytes format shouldn't have a BOM
                break;
            }
            case Enc::UTF8: {
                u8 h[3] = {0};
                bool valid = ins->read(h);
                gotBom = valid && memcmp(h, "\xef\xbb\xbf", 3) == 0;
                break;
            }
            case Enc::UTF16_be: {
                u8 h[2] = {0};
                bool valid = ins->read(h);
                gotBom = valid && memcmp(h, "\xfe\xff", 2) == 0;
                break;
            }
            case Enc::UTF16_le: {
                u8 h[2] = {0};
                bool valid = ins->read(h);
                gotBom = valid && memcmp(h, "\xff\xfe", 2) == 0;
                break;
            }
        }
        if (!gotBom) {
            // Expected a BOM, but didn't actually encounter one
            // FIXME: Some callers may want to know about this
            ins->rewind(start);
        }
    }

    // Install converter from UTF-16 if needed
    OptionallyOwned<InStream> importer;
    switch (this->encoding) {
        case TextFormat::Encoding::Bytes: // FIXME: Bytes needs to be converted
        case TextFormat::Encoding::UTF8: {
            importer = std::move(ins);
            break;
        }

        case TextFormat::Encoding::UTF16_be: {
            importer = Owned<InStream>::create(Owned<InPipe_TextConverter>::create(
                std::move(ins), TextEncoding::get<UTF8>(), TextEncoding::get<UTF16_BE>()));
            break;
        }

        case TextFormat::Encoding::UTF16_le: {
            importer = Owned<InStream>::create(Owned<InPipe_TextConverter>::create(
                std::move(ins), TextEncoding::get<UTF8>(), TextEncoding::get<UTF16_LE>()));
            break;
        }
    }

    // Install newline filter (basically just eats \r)
    // FIXME: Some caller might want the LFs to be unchanged.
    return Owned<StringReader>::create(createInNewLineFilter(std::move(importer)));
}

PLY_NO_INLINE Owned<StringWriter>
TextFormat::createExporter(OptionallyOwned<OutStream>&& outs) const {
    OptionallyOwned<OutStream> exporter = std::move(outs);

    switch (this->encoding) {
        case TextFormat::Encoding::Bytes: { // FIXME: Bytes needs to be converted
            break;
        }

        case TextFormat::Encoding::UTF8: {
            if (this->bom) {
                exporter->write({"\xef\xbb\xbf", 3});
            }
            break;
        }

        case TextFormat::Encoding::UTF16_be: {
            if (this->bom) {
                exporter->write({"\xfe\xff", 2});
            }
            exporter = Owned<OutStream>::create(Owned<OutPipe_TextConverter>::create(
                std::move(exporter), TextEncoding::get<UTF16_BE>(), TextEncoding::get<UTF8>()));
            break;
        }

        case TextFormat::Encoding::UTF16_le: {
            if (this->bom) {
                exporter->write({"\xff\xfe", 2});
            }
            exporter = Owned<OutStream>::create(Owned<OutPipe_TextConverter>::create(
                std::move(exporter), TextEncoding::get<UTF16_LE>(), TextEncoding::get<UTF8>()));
            break;
        }
    }

    // Install newline filter
    return Owned<StringWriter>::create(
        createOutNewLineFilter(std::move(exporter), this->newLine == TextFormat::NewLine::CRLF));
}

} // namespace ply
