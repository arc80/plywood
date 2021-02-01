<% title "Unicode Support" %>
<% synopsis 
Unicode support.
%>

Plywood encourages the use of UTF-8-encoded text and is generally forgiving of encoding errors.

Plywood provides the `TextFormat` class for compatibility with other encodings. The `createImporter()` and `createExporter()` functions convert UTF-8-encoded text to and from other formats such as UTF-16, ISO 8859-1 or Windows-1252, allowing you to work exclusively with UTF-8 at runtime. There's also a `TextFormat::autodetect()` function to detect source encodings at load time.

Plywood's `String` and `StringView` classes often contain UTF-8-encoded text, but there's nothing to prevent you from storing other kinds of data in them as well. The exact encoding requirements depend on the operations performed on the string. For example, the `findByte()` and `trim()` functions will work with any 8-bit text encoding, including UTF-8, ISO 8859-1, Windows-1252 and ASCII. On the other hand, `reversedUTF8()` does expect UTF-8.

The `StringReader` and `OutStream` classes, which do things like read alphanumeric identifiers and convert numbers to text, will generally work with any 8-bit encoding that is compatible with ASCII, including UTF-8, ISO 8859-1 and Windows-1252.

At the lowest level, Plywood provides functions to encode and decode text one character (code point) at time through a variety of encodings. These functions reside in `TextEncoding.h` and include `UTF8::encodePoint()` and `UTF8::decodePoint()`. A decoded character is always represented as a Unicode point stored in a 32-bit integer. These functions are tolerant of encoding errors as described in the following section.

## How Plywood Handles Encoding Errors

In UTF-8, many byte sequences are considered invalid. For example, a byte with a hexadecimal value between `0xc0` and `0xdf` (inclusive) is expected to be followed by a byte with a hexadecimal value between `0x80` and `0xbf` (inclusive). Anything else is considered an error according to the Unicode standard.

[FIXME: add diagrams]

In general, Plywood's character decoding functions, such as `UTF8::decodePoint()`, take the following approach to encoding errors: Invalid byte sequences are always decoded _one code unit at a time_, with each code unit converted to a Unicode point of the same value. (In UTF-8, a code unit is a byte.) That way, if encoding errors are ignored, invalid UTF-8 sequences will simply be interpreted as ISO-8859-1. When using the low-level decoding functions, such errors can be detected by checking the `validEncoding` member of `decodePoint()`. 

In UTF-16, there are many invalid code point sequences as well. (In UTF-16, a code point is a 16-bit integer.) For example, a code point with a hexadecimal value between `0xd800` and `0xdbff` (inclusive) is expected to be followed by a code point with a hexadecimal value between `0xdc00` and `0xdfff` (inclusive). Anything else is considered an error according to the Unicode standard. In Plywood, the `UTF16_Native::decodePoint()` function decodes such errors one code unit at a time, which means that a Unicode point between `0xd800` and `0xdbff` is returned, and the error can be detected by checking the `validEncoding` member of the return value. The Unicode standard says that Unicode points between `0xd800` and `0xdbff` are not valid, but Plywood returns them anyway, since such values can be trivially encoded back to UTF-16 to reconstruct the original string exactly.

Note that when importing a UTF-8 file through `TextFormat`, no decoding step is performed at all, and errors won't be detected. For example, if you read a line of text from a UTF-8 file using `StringReader::readString<fmt::Line>()`, Plywood preserves the original UTF-8 text byte-for-byte, with any encoding errors left unchanged. That way, if the text is written back to a UTF-8 file, the original contents are preserved exactly.

Finally, note that for many Plywood functions, UTF-8 encoding errors are irrelevant. For example, most of the `String` and `StringView` member functions, such as `trim()`, simply view the string as a sequence of bytes, and are therefore indifferent to UTF-8 encoding errors. Most of the `StringReader` parse functions only give special treatment to ASCII code points, which are represented as a single byte in UTF-8 and therefore can't be encoded incorrectly. And if any incorrectly-encoded UTF-8 sequences are passed to `String::format()`, either in the format string on in an argument, they are simply passed through unchanged.

## Path Manipulation

In Plywood, all `FileSystem` functions, such as `listDir()` and `openForWrite()`, work with UTF-8 paths and filenames, even on Windows.

On Windows, the NTFS filesystem stores filenames using 16-bit characters. Plywood currently interprets such filenames as UTF-16 and converts them to and from UTF-8 according to the above rules. This approach has the convenient property that all possible NTFS filenames have a unique UTF-8 representation that converts back to the original filename exactly.

See [Manipulating Paths](ManipulatingPaths) for more information.
