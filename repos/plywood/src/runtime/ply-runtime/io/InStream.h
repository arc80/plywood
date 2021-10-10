/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/ChunkList.h>
#include <ply-runtime/io/Pipe.h>
#include <ply-runtime/container/Owned.h>

namespace ply {

struct ViewInStream;

//------------------------------------------------------------------------------------------------
/*!
`InStream` performs buffered input from an arbitrary input source.

`InStream` is similar to `FILE` in C or `std::istream` in C++. It maintains an internal input buffer
on the heap, and provides functions to quickly read small amounts of data incrementally, making it
suitable for application-level parsing of text and binary formats. `InPipe`, on the other hand, is a
lower-level class more akin to a Unix file descriptor.

The `parse` and `readString` member functions are intended for reading text in an 8-bit format
compatible with ASCII, such as UTF-8, ISO 8859-1 or Windows-1252. For more information, see
[Parsing Text](ParsingText).

Some `InStream` objects contain adapters that perform conversions. For example, the `InStream`
returned by `FileSystem::openTextForRead` can normalize endings and convert from UTF-16 to UTF-8.
For more information, see [Unicode Support](UnicodeSupport).

If the `InStream`'s underlying input comes from an `InPipe`, the internal input buffer is managed by
a `ChunkListNode`, and you can call `getCursor()` at any time to create a `ChunkCursor`. You can
then rewind the `InStream` to an earlier point using `rewind()`, or copy some data between two
`ChunkCursor`s into a single contiguous memory buffer.

To create an `InStream` that reads from a contiguous memory block, use `ViewInStream`.
*/
struct InStream {
    static const u32 DefaultChunkSizeExp = 12;

    enum class Type : u32 {
        View = 0,
        Pipe,
        Mem,
    };

    struct Status {
        u32 chunkSizeExp : 27;
        u32 type : 2;
        u32 isPipeOwner : 1;
        u32 eof : 1;
        u32 parseError : 1;

        PLY_INLINE Status(Type type, u32 chunkSizeExp = InStream::DefaultChunkSizeExp)
            : chunkSizeExp{chunkSizeExp}, type{(u32) type}, isPipeOwner{0}, eof{0}, parseError{0} {
        }
    };

    /*!
    A pointer to the next byte in the input buffer. If this pointer is equal to `endByte`, it is not
    safe to read from the pointer. Use member functions such as `tryMakeBytesAvailable()` to ensure
    that this pointer can be read safely.
    */
    const char* curByte = nullptr;

    /*!
    A pointer to the last byte in the input buffer. This pointer does not necessarily represent the
    end of the input stream; for example, it still might be possible to read more data from the
    underlying `InPipe`, or to advance to the next chunk in a `ChunkList`, by calling
    `tryMakeBytesAvailable()`.
    */
    const char* endByte = nullptr;

    union {
        const char* startByte;          // if Type::View
        Reference<ChunkListNode> chunk; // if Type::Pipe or Type::Mem
    };
    union {
        InPipe* inPipe; // only if Type::Pipe
        void* reserved; // if Type::Mem or Type::View
    };
    Status status;

protected:
    PLY_DLL_ENTRY void destructInternal();
    PLY_DLL_ENTRY u32 tryMakeBytesAvailableInternal(u32 numBytes);

    PLY_INLINE InStream(Type type, u32 chunkSizeExp = DefaultChunkSizeExp)
        : status{type, chunkSizeExp} {
    }

public:
    /*!
    Constructs an empty `InStream`. You can replace it with a valid `InStream` later using move
    assignment.
    */
    PLY_INLINE InStream() : startByte{nullptr}, reserved{nullptr}, status{Type::View, 0} {
    }

    /*!
    Move constructor.
    */
    PLY_DLL_ENTRY InStream(InStream&& other);

    /*!
    Constructs an `InStream` from an `InPipe`. If `inPipe` is an owned pointer, the
    `InStream` takes ownership of the `InPipe` and will automatically destroy it in its
    destructor. If `inPipe` is a borrowed pointer, the `InStream` does not take ownership of the
    `InPipe`.
    */
    PLY_DLL_ENTRY InStream(OptionallyOwned<InPipe>&& inPipe,
                           u32 chunkSizeExp = DefaultChunkSizeExp);

    PLY_INLINE ~InStream() {
        if (this->status.type != (u32) Type::View) {
            this->destructInternal();
        }
    }

    /*!
    Move assignment operator.
    */
    PLY_DLL_ENTRY void operator=(InStream&& other);

    /*!
    Return `true` if the `InStream` is a `ViewInStream`.
    */
    PLY_INLINE bool isView() const {
        return this->status.type == (u32) Type::View;
    }

    PLY_INLINE u32 getChunkSize() {
        return 1 << this->status.chunkSizeExp;
    }

    /*!
    Returns `true` if no further data can be read from the stream, such as when the end of-file is
    encountered. This function also returns `true` if there's an error in the underlying `InPipe`
    that prevents further reading, such as when a network socket is closed prematurely.
    */
    PLY_INLINE bool atEOF() const {
        return this->status.eof != 0;
    }

    /*!
    Returns the number of bytes between `curByte` and `endByte`. Equivalent to
    `viewAvailable().numBytes`.
    */
    PLY_INLINE s32 numBytesAvailable() const {
        return safeDemote<s32>(this->endByte - this->curByte);
    }

    /*!
    Returns the memory region between `curByte` and `endByte` as a `StringView`.
    */
    PLY_INLINE StringView viewAvailable() const {
        return {this->curByte, safeDemote<u32>(this->endByte - this->curByte)};
    }

    /*!
    Returns a number that increases each time a byte is read from the `InStream`. If the underlying
    `InPipe` is a file, this number typically corresponds to the file offset.
    */
    PLY_DLL_ENTRY u64 getSeekPos() const;

    /*!
    Returns a `ChunkCursor` at the current input position. The `ChunkCursor` increments the
    reference count of the `InStream`'s internal `ChunkListNode`, preventing it from being destroyed
    when reading past the end of the chunk. This function is used internally by
    `StringReader::readString` in order to copy some region of the input to a `String`. In
    particular, `StringReader::readString<fmt::Line>` returns a single line of input as a `String`
    even if it originally spanned multiple `ChunkListNode`s.
    */
    PLY_DLL_ENTRY ChunkCursor getCursor() const;

    /*!
    Rewinds the `InStream` back to a previously saved position. This also clears the `InStream`'s
    end-of-file status.
    */
    PLY_DLL_ENTRY void rewind(ChunkCursor cursor);

    /*!
    Attempts to make at least `numBytes` available to read contiguously at `curByte`. Returns the
    number of bytes actually made available. If the underlying `InPipe` is waiting for data, this
    function will block until at least `numBytes` bytes arrive. If EOF/error is encountered, the
    return value will be less than `numBytes`; otherwise, it will be greater than or equal to
    `numBytes`.
    */
    PLY_INLINE u32 tryMakeBytesAvailable(u32 numBytes = 1) {
        if (uptr(this->endByte - this->curByte) < numBytes)
            return this->tryMakeBytesAvailableInternal(numBytes);
        return numBytes;
    }

    /*!
    Equivalent to `curByte[index]`, with bounds checking performed on `index` at runtime. The caller
    is responsible for ensuring that it's safe to read from the input buffer at this index by
    calling `tryMakeBytesAvailable()` beforehand.

        if (ins->tryMakeBytesAvailable() && ins->peekByte() == '.') {
            // Handle this input character...
            ins->advanceByte();
        }
    */
    PLY_INLINE char peekByte(u32 index = 0) const {
        PLY_ASSERT(index < (uptr) (this->endByte - this->curByte));
        return this->curByte[index];
    }

    /*!
    Equivalent to `curByte += numBytes`, with bounds checking performed on `numBytes` at runtime.
    The caller is responsible for ensuring that there are actually `numBytes` available in the input
    buffer by calling `tryMakeBytesAvailable()` beforehand.
    */
    PLY_INLINE void advanceByte(u32 numBytes = 1) {
        PLY_ASSERT(numBytes <= (uptr) (this->endByte - this->curByte));
        this->curByte += numBytes;
    }

    /*!
    Reads and returns the next byte from the input stream, or `0` if no more bytes are available.
    Use `atEOF()` to determine whether the read was actually successful.
    */
    PLY_INLINE u8 readByte() {
        if (!tryMakeBytesAvailable())
            return 0;
        return *this->curByte++;
    }

    PLY_DLL_ENTRY bool readSlowPath(MutableStringView dst);

    /*!
    Attempts to fill `dst` with data from the input stream. If the underlying `InPipe` is waiting
    for data, this function will block until `dst` is filled. Returns `true` if the buffer is filled
    successfully. If EOF/error is encountered before `dst` can be filled, the remainder of `dst` is
    filled with zeros and `false` is returned.
    */
    PLY_INLINE bool read(MutableStringView dst) {
        if (dst.numBytes > safeDemote<u32>(this->endByte - this->curByte)) {
            return this->readSlowPath(dst);
        }
        // If dst.numBytes is small and known at compile time, we count on the compiler to
        // implement memcpy() as a MOV:
        memcpy(dst.bytes, this->curByte, dst.numBytes);
        this->curByte += dst.numBytes;
        return this->status.eof == 0;
    }

    PLY_DLL_ENTRY bool skipSlowPath(u32 numBytes);

    /*!
    Attempts to skip `numBytes` bytes the input stream. If the underlying `InPipe` is waiting for
    data, this function will block until the specified number of bytes arrive. Returns `true` if
    successful. Returns `false` if EOF/error is encountered before the specified number of bytes are
    skipped.
    */
    PLY_INLINE bool skip(u32 numBytes) {
        if (numBytes > safeDemote<u32>(this->endByte - this->curByte)) {
            return this->skipSlowPath(numBytes);
        }
        this->curByte += numBytes;
        return this->status.eof == 0;
    }

    /*!
    Reads all the remaining data from the input stream and returns the contents as a `String`.
    */
    PLY_DLL_ENTRY String readRemainingContents();

    /*!
    \beginGroup
    Casts the `InStream` to various other types. It's always legal to call `asStringReader()`, but
    it's only legal to call the other functions if `isView()` returns `true`.
    */
    PLY_INLINE ViewInStream* asViewInStream();
    PLY_INLINE const ViewInStream* asViewInStream() const;
    /*!
    \endGroup
    */

    /*!
    A template function to parse the data type given by _`Type`_. It currently supports `s8`, `s16`,
    `s32`, `s64`, `u8`, `u16`, `u32`, `u64`, `float` and `double`. You can extend it to support
    additional types by specializing the `fmt::TypeParser` class template.

        u32 a = ins.parse<u32>();         // parse an integer such as "123"
        double b = ins.parse<double>();   // parse a floating-point number such as "-123.456"

    This function accepts an optional argument `format` whose type depends on the data type being
    parsed. For `s8`, `s16`, `s32`, `s64`, `u8`, `u16`, `u32`, `u64`, `float` and `double`, the
    expected type of this argument is `fmt::Radix`.

        u32 a = ins.parse<u32>(fmt::Radix{16});   // parse hex integer such as "badf00d"
        u32 b = ins.parse<u32>(fmt::Radix{2});    // parse binary integer such as "1101101"

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

    Example:

        ins.parse<fmt::WhiteSpace>();     // returns nothing; whitespace is skipped

    This function accepts an optional argument `format` of type _`Format`_. When this argument is
    passed, you can leave out the function template argument and let the compiler deduce it from the
    argument type:

        bool success = ins.parse(fmt::QuotedString{fmt::AllowSingleQuote});

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

        String line = ins.readString<fmt::Line>();

    Internally, `readString()` uses `getCursor()` to return a new `String`. If you would like both
    the `String` and the value returned by the parse function, you can use `getCursor()` yourself:

        ChunkCursor startCursor = ins.getCursor();
        u32 value = ins.parse<u32>();
        String str = ChunkCursor::toString(std::move(startCursor), ins.getCursor());
    */
    template <typename Type>
    PLY_INLINE String readString(const decltype(fmt::TypeParser<Type>::defaultFormat())& format =
                                     fmt::TypeParser<Type>::defaultFormat()) {
        ChunkCursor startCursor = this->getCursor();
        fmt::TypeParser<Type>::parse(this, format); // ignore return value
        return ChunkCursor::toString(std::move(startCursor), this->getCursor());
    }

    template <typename Format, typename = void_t<decltype(fmt::FormatParser<Format>::parse)>>
    PLY_INLINE String readString(const Format& format = {}) {
        ChunkCursor startCursor = this->getCursor();
        fmt::FormatParser<Format>::parse(this, format); // ignore return value
        return ChunkCursor::toString(std::move(startCursor), this->getCursor());
    }
    /*!
    \endGroup
    */

    /*!
    Returns `true` if an error occurred in a previously called parse function.
    */
    PLY_INLINE bool anyParseError() const {
        return this->status.parseError != 0;
    }
};

//------------------------------------------------------------------
// ViewInStream
//------------------------------------------------------------------
struct ViewInStream : InStream {
    PLY_INLINE ViewInStream() = default;

    PLY_INLINE ViewInStream(StringView view) : InStream{Type::View, 0} {
        this->startByte = view.bytes;
        this->curByte = view.bytes;
        this->endByte = view.bytes + view.numBytes;
        this->reserved = nullptr;
    }

    PLY_INLINE ~ViewInStream() {
        PLY_ASSERT(this->isView());
        // This lets the compiler optimize away the call to destructInternal():
        this->status.type = (u32) Type::View;
    }

    PLY_INLINE void operator=(const ViewInStream& other) {
        PLY_ASSERT(this->isView() && other.isView());
        this->startByte = other.startByte;
        this->curByte = other.curByte;
        this->endByte = other.endByte;
        this->status = other.status;
    }

    struct SavePoint {
        const char* startByte = nullptr;

        PLY_INLINE SavePoint(const InStream* ins) : startByte{ins->curByte} {
        }
    };

    PLY_INLINE SavePoint savePoint() const {
        PLY_ASSERT(this->isView());
        return {this};
    }

    PLY_INLINE StringView getViewFrom(const SavePoint& savePoint) const {
        PLY_ASSERT(this->isView());
        PLY_ASSERT(uptr(this->curByte - savePoint.startByte) <=
                   uptr(this->endByte - this->startByte));
        return StringView::fromRange(savePoint.startByte, this->curByte);
    }

    PLY_INLINE void restore(const SavePoint& savePoint) {
        PLY_ASSERT(this->isView());
        PLY_ASSERT(uptr(this->curByte - savePoint.startByte) <=
                   uptr(this->endByte - this->startByte));
        this->curByte = savePoint.startByte;
    }

    PLY_INLINE const char* getStartByte() const {
        PLY_ASSERT(this->isView());
        return this->startByte;
    }

    /*!
    \beginGroup
    These functions are similar to `StringReader`'s family of `readString()` functions except that
    they return a `StringView` into the existing memory buffer instead of copying data to a new
    `String`. For example, `readView<fmt::Line>()` returns a `StringView` that contains a single
    line of text terminated by `'\n'`.

        StringView line = vins.readView<fmt::Line>();
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

PLY_INLINE ViewInStream* InStream::asViewInStream() {
    PLY_ASSERT(this->isView());
    return static_cast<ViewInStream*>(this);
}

PLY_INLINE const ViewInStream* InStream::asViewInStream() const {
    PLY_ASSERT(this->isView());
    return static_cast<const ViewInStream*>(this);
}

//------------------------------------------------------------------
// NativeEndianReader
//------------------------------------------------------------------
class NativeEndianReader {
public:
    InStream* ins;

    PLY_INLINE NativeEndianReader(InStream* ins) : ins{ins} {
    }

    template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
    PLY_INLINE T read() {
        T value;
        ins->read({(char*) &value, sizeof(value)});
        return value;
    }
};

template <typename T>
PLY_NO_INLINE T StringView::to(const T& defaultValue) const {
    ViewInStream vins{this->trim(isWhite)};
    T value = vins.parse<T>();
    if (vins.atEOF() && !vins.anyParseError()) {
        return value;
    }
    return defaultValue;
}

} // namespace ply

#include <ply-runtime/io/impl/TypeParser.h>
