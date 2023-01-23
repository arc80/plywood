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

    /*!
    Possible values are `Bytes`, `UTF8`, `UTF16_be` and `UTF16_le`.
    */
    UnicodeType encoding = UTF8;

    /*!
    Possible values are `LF` and `CRLF`.
    */
    NewLine newLine = NewLine::LF;

    /*!
    Whether the text begins with a byte order mark (BOM).
    */
    bool bom = false;

    /*!
    Returns a `TextFormat` with `UTF8`, `LF` and no BOM.
    */
    static PLY_INLINE TextFormat unixUTF8() {
        return {UTF8, TextFormat::NewLine::LF, false};
    }

    /*!
    Returns a default `TextFormat` according to the host platform. On Windows, the
    default is `UTF8`, `CRLF` and no BOM. On Linux and macOS, the default is `UTF8`,
    `LF` and no BOM.
    */
    static TextFormat platformPreference();

    /*!
    Attempts to guess the text file format of the contents of `ins`. This function reads
    up to 4 KB of data from `ins`, then rewinds it back to the beginning using
    `InStream::rewind()`.
    */
    static TextFormat autodetect(InStream& in);

    /*!
    Creates a new `InStream` that converts the raw contents of `in` to UTF-8 with
    Unix-style newlines and no byte order mark (BOM). The contents of `in` are expected
    to have the format described by the provided `TextFormat` object. Conversion is
    performed on-the-fly while data is being read.
    */
    InStream createImporter(InStream&& in) const;

    /*!
    Creates a new `OutStream` that writes raw data to `out` in the format described by
    the provided `TextFormat` object. The resulting `OutStream` object expects
    UTF-8-encoded text. The `OutStream` accepts both Windows and Unix-style newlines;
    all newlines will be converted to the format described by the provided `TextFormat`
    object. Conversion is performed on-the-fly while data is written.
    */
    OutStream createExporter(OutStream&& out) const;

    /*!
    Returns `true` if the `TextFormat`s are identical.
    */
    PLY_INLINE bool operator==(const TextFormat& other) const {
        return (this->encoding == other.encoding) && (this->newLine == other.newLine) &&
               (this->bom == other.bom);
    }
};

} // namespace ply
