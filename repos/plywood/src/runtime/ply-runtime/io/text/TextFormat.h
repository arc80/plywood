/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/io/text/StringReader.h>
#include <ply-runtime/io/text/StringWriter.h>

namespace ply {

//------------------------------------------------------------------------------------------------
/*!
The `TextFormat` class provides functionality to load and save text files in various file formats.
It supports UTF-8, UTF-16, optional [byte order marks
(BOM)](https://en.wikipedia.org/wiki/Byte_order_mark), and either Unix or Windows-style newlines.

All text loaded through `TextFormat` is converted to UTF-8 with Unix-style newlines and no byte
order mark (BOM).

For more information, see [Unicode Support](Unicode).
*/
struct TextFormat {
    // FIXME: Maybe remove this Encoding enum and just use TextEncoding
    enum class Encoding {
        Bytes,
        UTF8,
        UTF16_be,
        UTF16_le,
    };
    enum class NewLine {
        LF,
        CRLF,
    };

    /*!
    Possible values are `Bytes`, `UTF8`, `UTF16_be` and `UTF16_le`.
    */
    Encoding encoding = Encoding::UTF8;

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
        return {TextFormat::Encoding::UTF8, TextFormat::NewLine::LF, false};
    }

    /*!
    Returns a default `TextFormat` according to the host platform. On Windows, the default is
    `UTF8`, `CRLF` and no BOM. On Linux and macOS, the default is `UTF8`, `LF` and no BOM.
    */
    static PLY_DLL_ENTRY TextFormat platformPreference();

    /*!
    Attempts to guess the text file format of the contents of `ins`. This function reads up to 4 KB
    of data from `ins`, then rewinds it back to the beginning using `InStream::rewind()`.
    */
    static PLY_DLL_ENTRY TextFormat autodetect(InStream* ins);

    /*!
    Creates a new `StringReader` that converts the raw contents of `ins` to UTF-8 with Unix-style
    newlines and no byte order mark (BOM). The contents of `ins` are expected to have the format
    described by the provided `TextFormat` object. Conversion is performed on-the-fly while data is
    being read.

    [FIXME: Say something here about OptionallyOwned.]
    */
    PLY_DLL_ENTRY Owned<StringReader> createImporter(OptionallyOwned<InStream>&& ins) const;

    /*!
    Creates a new `StringWriter` that writes raw data to `outs` in the format described by the
    provided `TextFormat` object. The resulting `StringWriter` object expects UTF-8-encoded text.
    The `StringWriter` accepts both Windows and Unix-style newlines; all newlines will be converted
    to the format described by the provided `TextFormat` object. Conversion is performed on-the-fly
    while data is written.
    */
    PLY_DLL_ENTRY Owned<StringWriter> createExporter(OptionallyOwned<OutStream>&& outs) const;
};

} // namespace ply
