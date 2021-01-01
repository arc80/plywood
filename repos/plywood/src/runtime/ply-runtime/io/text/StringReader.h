/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/io/InStream.h>
#include <ply-runtime/string/String.h>
#include <ply-runtime/container/Tuple.h>
#include <ply-runtime/container/HiddenArgFunctor.h>
#include <ply-runtime/container/Subst.h>

namespace ply {

namespace fmt {
extern const u8 DigitTable[256];
extern const u32 WhitespaceMask[8];
PLY_DLL_ENTRY void scanUsingMask(InStream* ins, const u32* mask, bool invert);
PLY_DLL_ENTRY void scanUsingCallback(InStream* ins, const LambdaView<bool(char)>& callback);
PLY_DLL_ENTRY bool scanUpToAndIncludingSpecial(InStream* ins, const StringView special);
} // namespace fmt

//------------------------------------------------------------------------------------------------
/*!
`StringReader` is a subclass of `InStream` with additional member functions for parsing text.

The `InStream` and `StringReader` classes are, in fact, interchangeable. You can cast an `InStream`
to a `StringReader` at any time by calling `InStream::strReader()`. The main reason why `InStream`
and `StringReader` are separate classes is to help express intention in the code. `InStream`s are
mainly intended to read binary data, and `StringReader`s are mainly intended to read text encoded in
an 8-bit format compatible with ASCII, such as UTF-8, ISO 8859-1, Windows-1252 or ASCII itself.

Just like `InStream`, you can create a `StringReader` directly from an `InPipe`, such as `stdIn`.
However, such a `StringReader` will not perform any automatic newline conversion:

    StringReader sr{stdIn};

To create a `StringReader` directly from a `String` or a `StringView`, use `StringViewReader`
instead.

Most `StringReader` parse functions only recognize characters in the ASCII character set, and will
work with text in any 8-bit encoding compatible with ASCII such as UTF-8, ISO 8859-1 or
Windows-1252. If you have text encoded as UTF-16, you can create an adapter to convert it to UTF-8
automatically using `createImporter()`.

`StringReader` itself does not perform any newline conversion. Most `StringReader` parse functions
consider `'\r'` as whitespace and `'\n'` as newline. In particular,
`StringReader::readString<fmt::Line>()` always returns a string that ends with `'\n'` and will
preserve `'\r'` characters unchanged. If you expect Windows-style newlines in your input, and you
want them automatically converted to Unix-style newlines, use `createImporter()` to strip out `'\r'`
characters automatically.

For more information, see [Unicode Support](Unicode).
*/
struct StringReader : InStream {
protected:
    PLY_INLINE StringReader(Type type, u32 chunkSizeExp = DefaultChunkSizeExp)
        : InStream{type, chunkSizeExp} {
    }

public:
    /*!
    Constructs an empty `StringReader`. You can replace it with a valid `StringReader` later using
    move assignment.
    */
    PLY_INLINE StringReader() = default;

    /*!
    Move constructor.
    */
    PLY_INLINE StringReader(StringReader&& other) : InStream{std::move(other)} {
    }

    /*!
    Constructs a `StringReader` from an `InPipe`. If `inPipe` is an owned pointer, the
    `StringReader` takes ownership of the `InPipe` and will automatically destroy it in its
    destructor. If `inPipe` is a borrowed pointer, the `StringReader` does not take ownership of the
    `InPipe`.

    [FIXME: Link to text conversion streams here]
    */
    PLY_INLINE StringReader(OptionallyOwned<InPipe>&& inPipe,
                            u32 chunkSizeExp = DefaultChunkSizeExp)
        : InStream{std::move(inPipe), chunkSizeExp} {
    }

    /*!
    Returns `true` if an error occurred in a previously called parse function.
    */
    PLY_INLINE bool anyParseError() const {
        return this->status.parseError != 0;
    }

    /*!
    A template function to parse the data type given by _`Type`_. It currently supports `s8`, `s16`,
    `s32`, `s64`, `u8`, `u16`, `u32`, `u64`, `float` and `double`. You can extend it to support
    additional types by specializing the `fmt::TypeParser` class template.

        u32 a = strReader.parse<u32>();         // parse an integer such as "123"
        double b = strReader.parse<double>();   // parse a floating-point number such as "-123.456"

    This function accepts an optional argument `format` whose type depends on the data type being
    parsed. For `s8`, `s16`, `s32`, `s64`, `u8`, `u16`, `u32`, `u64`, `float` and `double`, the
    expected type of this argument is `fmt::Radix`.

        u32 a = strReader.parse<u32>(fmt::Radix{16});   // parse hex integer such as "badf00d"
        u32 b = strReader.parse<u32>(fmt::Radix{2});    // parse binary integer such as "1101101"

    For more information, see [Parsing Text](ParsingText).
    */
    template <typename Type>
    PLY_INLINE Type parse(const decltype(fmt::TypeParser<Type>::defaultFormat())& format =
                              fmt::TypeParser<Type>::defaultFormat()) {
        return fmt::TypeParser<Type>::parse(this, format);
    }

    /*!
    A template function to parse text in the format specified by _`Format`_. The return type depends
    on the format being parsed. It currently supports the following built-in formats:

    * `fmt::QuotedString`
    * `fmt::Identifier`
    * `fmt::Line`
    * `fmt::WhiteSpace`
    * `fmt::NonWhiteSpace`

        strReader.parse<fmt::WhiteSpace>();     // returns nothing; whitespace is skipped

    This function accepts an optional argument `format` of type _`Format`_. When this argument is
    passed, you can leave out the function template argument and let the compiler deduce it from the
    argument type:

        bool success = strReader.parse(fmt::QuotedString{fmt::AllowSingleQuote});

    You can extend this function to support additional formats by specializing the
    `fmt::FormatParser` class template.

    For more information, see [Parsing Text](ParsingText).
    */
    template <typename Format, typename = void_t<decltype(fmt::FormatParser<Format>::parse)>>
    PLY_INLINE auto parse(const Format& format = {}) {
        return fmt::FormatParser<Format>::parse(this, format);
    }

    /*!
    \beginGroup
    These functions are similar to the family of `parse()` functions, except that they return a
    `String` containing the input that was consumed by the parse operation. For example,
    `readString<fmt::Line>()` returns a single line of text terminated by `'\n'`.

        String line = strReader.readString<fmt::Line>();

    Internally, `readString()` uses `getCursor()` to return a new `String`. If you would like both
    the `String` and the value returned by the parse function, you can use `getCursor()` yourself:

        ChunkCursor startCursor = strReader.getCursor();
        u32 value = strReader.parse<u32>();
        String str = String::fromChunks(std::move(startCursor), strReader.getCursor());
    */
    template <typename Type>
    PLY_INLINE String readString(const decltype(fmt::TypeParser<Type>::defaultFormat())& format =
                                     fmt::TypeParser<Type>::defaultFormat()) {
        ChunkCursor startCursor = this->getCursor();
        fmt::TypeParser<Type>::parse(this, format); // ignore return value
        return String::fromChunks(std::move(startCursor), this->getCursor());
    }

    template <typename Format, typename = void_t<decltype(fmt::FormatParser<Format>::parse)>>
    PLY_INLINE String readString(const Format& format = {}) {
        ChunkCursor startCursor = this->getCursor();
        fmt::FormatParser<Format>::parse(this, format); // ignore return value
        return String::fromChunks(std::move(startCursor), this->getCursor());
    }
    /*!
    \endGroup
    */

    /*!
    Reads all the remaining data from the input stream and returns the contents as a `String`.
    */
    PLY_INLINE String readRemainingContents() {
        return String::moveFromBuffer(this->InStream::readRemainingContents());
    }

    /*!
    Reads all the remaining data from the input stream and returns the contents as a `String`.
    */
    PLY_INLINE StringView viewAvailable() const {
        return StringView::fromBufferView(this->InStream::viewAvailable());
    }
};

//------------------------------------------------------------------------------------------------
/*!
`StringViewReader` is a subclass of `StringReader` that reads from a fixed-size memory buffer.

`StringViewReader` contains the same parse functions as `StringReader` as well as an additional
family of functions, `readView()`, that take advantage of the underlying input source being a
fixed-sized memory buffer.

One way to parse large files efficiently is to load the entire file in memory using
`TextFormat::createImporter()`, create a `StringViewReader` to read from the loaded file, then call
`readView()` instead of `readString()` to parse the contents. `readView()` avoids making additional
string copies by returning `StringView`s into the already-loaded file. This is the approach taken,
for example, by Plywood's built-in C++ parser.
*/
struct StringViewReader : StringReader {
    /*!
    Constructs an empty `StringViewReader`. You can replace it with a valid `StringViewReader` later
    using copy assignment.
    */
    PLY_INLINE StringViewReader() = default;

    /*!
    Constructs a `StringViewReader` that reads the contents of `view`. `view` must remain valid for
    the lifetime of the `StringViewReader`.
    */
    PLY_INLINE StringViewReader(const StringView view) : StringReader{Type::View, 0u} {
        this->startByte = (u8*) view.bytes;
        this->curByte = (u8*) view.bytes;
        this->endByte = (u8*) view.bytes + view.numBytes;
    }

    /*!
    Copy assignment operator.
    */
    PLY_INLINE void operator=(const StringViewReader& other) {
        *this->asViewInStream() = *other.asViewInStream();
    }

    PLY_INLINE ~StringViewReader() {
        PLY_ASSERT(this->status.type == (u32) InStream::Type::View);
        // By setting this flag, the compiler is able to optimize out the call to the InStream
        // destructor:
        this->status.type = (u32) InStream::Type::View;
    }

    PLY_INLINE ViewInStream::SavePoint savePoint() const {
        return this->asViewInStream()->savePoint();
    }
    PLY_INLINE StringView getViewFrom(const ViewInStream::SavePoint& savePoint) const {
        return StringView::fromBufferView(this->asViewInStream()->getViewFrom(savePoint));
    }
    PLY_INLINE void restore(const ViewInStream::SavePoint& savePoint) {
        return this->asViewInStream()->restore(savePoint);
    }
    PLY_INLINE const u8* getStartByte() const {
        return this->asViewInStream()->getStartByte();
    }

    /*!
    \beginGroup
    These functions are similar to `StringReader`'s family of `readString()` functions except that
    they return a `StringView` into the existing memory buffer instead of copying data to a new
    `String`. For example, `readView<fmt::Line>()` returns a `StringView` that contains a single
    line of text terminated by `'\n'`.

        StringView line = strViewReader.readView<fmt::Line>();
    */
    template <typename Type>
    PLY_INLINE StringView readView(const decltype(fmt::TypeParser<Type>::defaultFormat())& format =
                                       fmt::TypeParser<Type>::defaultFormat()) {
        PLY_ASSERT(this->isView()); // prevent bad casts
        const char* startByte = (const char*) this->curByte;
        fmt::TypeParser<Type>::parse(this, format); // ignore return value
        return StringView::fromRange(startByte, (const char*) this->curByte);
    }

    template <typename Format, typename = void_t<decltype(fmt::FormatParser<Format>::parse)>>
    PLY_INLINE StringView readView(const Format& format = {}) {
        PLY_ASSERT(this->isView()); // prevent bad casts
        const char* startByte = (const char*) this->curByte;
        fmt::FormatParser<Format>::parse(this, format); // ignore return value
        return StringView::fromRange(startByte, (const char*) this->curByte);
    }
    /*!
    \endGroup
    */
};

PLY_INLINE StringReader* InStream::asStringReader() {
    return static_cast<StringReader*>(this);
}

PLY_INLINE StringViewReader* InStream::asStringViewReader() {
    PLY_ASSERT(this->isView());
    return static_cast<StringViewReader*>(this);
}

template <typename T>
PLY_NO_INLINE T StringView::to(const T& defaultValue) const {
    StringViewReader svr{this->trim(isWhite)};
    T value = svr.parse<T>();
    if (svr.atEOF() && !svr.anyParseError()) {
        return value;
    }
    return defaultValue;
}

namespace fmt {

struct Radix {
    u32 base = 10;
};

//----------------------------------------------------------
// Built-in fmt::TypeParsers
//----------------------------------------------------------
template <>
struct TypeParser<u64> {
    static PLY_INLINE Radix defaultFormat() {
        return {10};
    }
    static PLY_DLL_ENTRY u64 parse(InStream* ins, Radix radix);
};
template <>
struct TypeParser<s64> {
    static PLY_INLINE Radix defaultFormat() {
        return {10};
    }
    static PLY_DLL_ENTRY s64 parse(InStream* ins, Radix radix);
};
template <>
struct TypeParser<double> {
    static PLY_INLINE Radix defaultFormat() {
        return {10};
    }
    static PLY_DLL_ENTRY double parse(InStream* ins, Radix radix);
};
template <>
struct TypeParser<float> {
    static PLY_INLINE Radix defaultFormat() {
        return {10};
    }
    static PLY_INLINE float parse(InStream* ins, Radix radix) {
        return (float) TypeParser<double>::parse(ins, radix);
    }
};

template <typename DstType, typename FullType>
struct TypeParser_Integral {
    static PLY_INLINE Radix defaultFormat() {
        return {10};
    }
    static PLY_NO_INLINE DstType parse(InStream* ins, Radix radix) {
        FullType fullValue = TypeParser<FullType>::parse(ins, radix);
        DstType result = (DstType) fullValue;
        if (result != fullValue) {
            result = 0;
            ins->status.parseError = 1;
        }
        return result;
    }
};
template <>
struct TypeParser<u32> : TypeParser_Integral<u32, u64> {};
template <>
struct TypeParser<u16> : TypeParser_Integral<u16, u64> {};
template <>
struct TypeParser<u8> : TypeParser_Integral<u8, u64> {};
template <>
struct TypeParser<s32> : TypeParser_Integral<s32, s64> {};
template <>
struct TypeParser<s16> : TypeParser_Integral<s16, s64> {};
template <>
struct TypeParser<s8> : TypeParser_Integral<s8, s64> {};

//----------------------------------------------------------
// Built-in fmt::FormatParsers
//----------------------------------------------------------
// QuotedString
static constexpr u32 AllowSingleQuote = 0x1;
static constexpr u32 EscapeWithBackslash = 0x2;
static constexpr u32 CollapseDoubles = 0x4;
static constexpr u32 AllowMultilineWithTriple = 0x8;
struct QuotedString {
    enum ErrorCode {
        OK,
        NoOpeningQuote,
        UnexpectedEndOfLine,
        UnexpectedEndOfFile,
        BadEscapeSequence,
    };

    u32 flags = 0;
    HiddenArgFunctor<void(InStream*, ErrorCode)> errorCallback;
};
template <>
struct FormatParser<QuotedString> {
    static PLY_DLL_ENTRY String parse(InStream* ins, const QuotedString& format);
};

// Identifier
static constexpr u32 WithDollarSign = 0x1;
static constexpr u32 WithDash = 0x2;
struct Identifier {
    u32 flags = 0;
    PLY_INLINE Identifier(u32 flags = 0) : flags{flags} {
    }
};
template <>
struct FormatParser<Identifier> {
    static PLY_DLL_ENTRY bool parse(InStream* ins, const Identifier& format);
};

// Line
struct Line {};
template <>
struct FormatParser<Line> {
    static PLY_NO_INLINE bool parse(InStream* ins, const Line&) {
        return fmt::scanUpToAndIncludingSpecial(ins, "\n");
    }
};

// Whitespace
struct Whitespace {};
template <>
struct FormatParser<Whitespace> {
    static PLY_NO_INLINE void parse(InStream* ins, const Whitespace&) {
        fmt::scanUsingMask(ins, fmt::WhitespaceMask, false);
    }
};

// Callback
struct Callback {
    LambdaView<bool(char)> cb;
};
template <>
struct FormatParser<Callback> {
    static PLY_NO_INLINE void parse(InStream* ins, const Callback& cb) {
        fmt::scanUsingCallback(ins, cb.cb);
    }
};

// NonWhitespace
struct NonWhitespace {};
template <>
struct FormatParser<NonWhitespace> {
    static PLY_NO_INLINE void parse(InStream* ins, const NonWhitespace&) {
        fmt::scanUsingMask(ins, fmt::WhitespaceMask, true);
    }
};

} // namespace fmt
} // namespace ply
