/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/io/InStream.h>
#include <ply-runtime/io/OutStream.h>
#include <ply-runtime/string/TextEncoding.h>

namespace ply {

//------------------------------------------------------------------------------------
/*!
The `TextFormat` class provides functionality to load and save text files in various
file formats. It supports UTF-8, UTF-16, optional [byte order marks
(BOM)](https://en.wikipedia.org/wiki/Byte_order_mark), and either Unix or Windows-style
newlines.

All text loaded through `TextFormat` is converted to UTF-8 with Unix-style newlines and
no byte order mark (BOM).

For more information, see [Unicode Support](Unicode).
*/
struct TextFormat {
    enum class NewLine {
        LF,
        CRLF,
    };
    static constexpr u32 NumBytesForAutodetect = 4000;

    UnicodeType encoding = UTF8;
    NewLine newLine = NewLine::LF;
    bool bom = false;

    static TextFormat default_utf8();
    static TextFormat autodetect(InStream& in);

    /*!
    Creates a new `InPipe` that converts the raw contents of `in` to UTF-8 with
    Unix-style newlines and no byte order mark (BOM). The contents of `in` are expected
    to have the format described by the provided `TextFormat` object. Conversion is
    performed on-the-fly while data is being read.
    */
    Owned<InPipe> createImporter(InStream&& in) const;

    /*!
    Creates a new `OutPipe` that writes raw data to `out` in the format described by
    the provided `TextFormat` object. The resulting `OutStream` object expects
    UTF-8-encoded text. The `OutStream` accepts both Windows and Unix-style newlines;
    all newlines will be converted to the format described by the provided `TextFormat`
    object. Conversion is performed on-the-fly while data is written.
    */
    Owned<OutPipe> createExporter(OutStream&& out) const;

    PLY_INLINE bool operator==(const TextFormat& other) const {
        return (this->encoding == other.encoding) && (this->newLine == other.newLine) &&
               (this->bom == other.bom);
    }
};

} // namespace ply
