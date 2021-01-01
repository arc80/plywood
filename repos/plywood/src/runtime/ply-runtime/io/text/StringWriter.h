/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/string/String.h>
#include <ply-runtime/io/InStream.h>
#include <ply-runtime/io/OutStream.h>
#include <ply-runtime/container/FixedArray.h>
#include <ply-runtime/time/CPUTimer.h>

namespace ply {

namespace fmt {
template <typename T>
struct TypePrinter;
} // namespace fmt

//------------------------------------------------------------------------------------------------
/*!
`StringWriter` is a subclass of `OutStream` with additional member functions for writing text.

The `OutStream` and `StringWriter` classes are, in fact, interchangeable. You can cast an
`OutStream` to a `StringWriter` at any time by calling `OutStream::strWriter()`. The main reason why
`OutStream` and `StringWriter` are separate classes is to help express intention in the code.
`OutStream`s are mainly intended to write binary data, and `StringWriter`s are mainly intended to
write text encoded in an 8-bit format compatible with ASCII, such as UTF-8, ISO 8859-1, Windows-1252
or ASCII itself.

Use `TextFormat::createExporter()` to automatically convert UTF-8-encoded text to other formats.

Just like `OutStream`, you can create a `StringWriter` directly from an `OutPipe` such as `stdOut`.
However, such a `StringWriter` will not perform automatic newline conversion:

    StringWriter sw{stdOut};

For more information, see [Unicode Support](Unicode).
*/
struct StringWriter : OutStream {
private:
    struct Arg {
        void (*formatter)(StringWriter*, const void*) = nullptr;
        const void* pvalue = nullptr;
        template <typename T>
        static PLY_NO_INLINE void formatFunc(StringWriter* sw, const void* arg) {
            fmt::TypePrinter<T>::print(sw, *(const T*) arg);
        }
    };

    // Convert variable number of arguments into array of Arg:
    static PLY_INLINE void prepareArgList(Arg*) {
    }
    template <typename T, typename... Rest>
    static PLY_INLINE void prepareArgList(Arg* argList, const T& arg, const Rest&... rest) {
        argList->formatter = Arg::formatFunc<T>;
        argList->pvalue = &arg;
        prepareArgList(argList + 1, rest...);
    }

    PLY_DLL_ENTRY void formatInternal(const StringView fmt, ArrayView<const Arg> args);

public:
    /*!
    Constructs a `StringWriter` that writes to a chunk list in memory.
    */
    PLY_DLL_ENTRY StringWriter(u32 chunkSizeExp = OutStream::DefaultChunkSizeExp);

    /*!
    Move constructor. `other` is reset to a null stream. This constructor is safe to call even when
    `other` is an instance of `MemOutStream` or `ViewOutStream`.
    */
    PLY_INLINE StringWriter(StringWriter&& other) : OutStream{std::move(other)} {
    }

    /*!
    Constructs an `OutStream` that writes to an `OutPipe`. If `outPipe` is an owned pointer, the
    `OutStream` takes ownership of the `OutPipe` and will automatically destroy it in its
    destructor. If `outPipe` is a borrowed pointer, the `OutStream` does not take ownership of the
    `OutPipe`. See `OptionallyOwned`.
    */
    PLY_INLINE StringWriter(OptionallyOwned<OutPipe>&& outPipe,
                            u32 chunkSizeExp = DefaultChunkSizeExp)
        : OutStream{std::move(outPipe), chunkSizeExp} {
    }

    /*!
    Move assignment operator. `other` is reset to a null stream.
    */
    PLY_INLINE void operator=(StringWriter&& other) {
        this->~StringWriter();
        new (this) StringWriter{std::move(other)};
    }

    /*!
    If the `StringWriter` writes to a chunk list in memory, this function returns a `String`
    containing all the data that was written. The `StringWriter` is reset to a null stream as result
    of this call, which means that no further data can be written.

    For small strings (default fewer than 4096 bytes), no new memory is allocated and no data is
    copied as a result of this call; instead, the memory allocation containing the output buffer is
    resized and the returned `String` takes ownership of it directly.
    */
    PLY_INLINE String moveToString() {
        return ((MemOutStream*) this)->moveToString();
    }

    /*!
    Template function that expands the format string `fmt` using the given arguments and writes the
    result to the output stream.

        StringWriter sw;
        sw.format("The answer is {}.\n", 42);
        return sw.moveToString();

    For more information, see [Converting Values to Text](ConvertingValuesToText).
    */
    template <typename... Args>
    PLY_NO_INLINE void format(const StringView fmt, const Args&... args) {
        FixedArray<Arg, sizeof...(args)> argList;
        prepareArgList(argList.items, args...);
        this->formatInternal(fmt, argList.view());
    }

    /*!
    Template function that writes the the default text representation of `value` to the output
    stream.

        StringWriter sw;
        sw << "The answer is " << 42 << ".\n";
        return sw.moveToString();

    For more information, see [Converting Values to Text](ConvertingValuesToText).
    */
    template <typename T>
    PLY_NO_INLINE StringWriter& operator<<(const T& value) {
        fmt::TypePrinter<T>::print(this, value);
        return *this;
    }

    PLY_INLINE StringWriter& operator<<(char c) {
        this->writeByte(c);
        return *this;
    }
};

PLY_INLINE StringWriter* OutStream::strWriter() {
    return static_cast<StringWriter*>(this);
}

namespace fmt {

//----------------------------------------------------
// fmt::WithRadix, fmt::Hex
//----------------------------------------------------
struct WithRadix {
    enum Type {
        U64 = 0,
        S64,
        Double,
    };
    union {
        double double_;
        u64 u64_;
        s64 s64_;
    };
    u32 type : 2;
    u32 capitalize : 1;
    u32 radix : 29;

    template <typename SrcType,
              typename std::enable_if_t<std::is_floating_point<SrcType>::value, int> = 0>
    inline WithRadix(SrcType v, u32 radix, bool capitalize = false) {
        this->double_ = v;
        this->type = U64;
        this->capitalize = capitalize ? 1 : 0;
        this->radix = radix;
    }
    template <typename SrcType,
              typename std::enable_if_t<std::is_unsigned<SrcType>::value, int> = 0>
    inline WithRadix(SrcType v, u32 radix, bool capitalize = false) {
        this->u64_ = v;
        this->type = U64;
        this->capitalize = capitalize ? 1 : 0;
        this->radix = radix;
    }
    template <typename SrcType, typename std::enable_if_t<std::is_signed<SrcType>::value, int> = 0>
    inline WithRadix(SrcType v, u32 radix, bool capitalize = false) {
        this->s64_ = v;
        this->type = S64;
        this->capitalize = capitalize ? 1 : 0;
        this->radix = radix;
    }
};

struct Hex : WithRadix {
    template <typename SrcType,
              typename std::enable_if_t<std::is_arithmetic<SrcType>::value, int> = 0>
    inline Hex(SrcType v, bool capitalize = false) : WithRadix{v, 16, capitalize} {
    }
};

//-----------------------------------------------------------
// fmt::EscapedString
//-----------------------------------------------------------
struct EscapedString {
    StringView view;
    u32 maxPoints = 0;
    PLY_INLINE EscapedString(StringView view, u32 maxPoints = 0)
        : view{view}, maxPoints{maxPoints} {
    }
};

//-----------------------------------------------------------
// fmt::XMLEscape
//-----------------------------------------------------------
struct XMLEscape {
    StringView view;
    u32 maxPoints = 0;
    PLY_INLINE XMLEscape(StringView view, u32 maxPoints = 0) : view{view}, maxPoints{maxPoints} {
    }
};

//-----------------------------------------------------------
// fmt::CmdLineArg_WinCrt
//-----------------------------------------------------------
struct CmdLineArg_WinCrt {
    StringView view;
    PLY_INLINE CmdLineArg_WinCrt(StringView view) : view{view} {
    }
};

//----------------------------------------------------
// TypePrinters
//----------------------------------------------------
template <typename DstType, typename SrcType>
struct TypePrinter_Cast {
    static PLY_INLINE void print(StringWriter* sw, const SrcType& value) {
        TypePrinter<DstType>::print(sw, value);
    }
};

template <>
struct TypePrinter<u32> : TypePrinter_Cast<u64, u32> {};
template <>
struct TypePrinter<u16> : TypePrinter_Cast<u64, u16> {};
template <>
struct TypePrinter<u8> : TypePrinter_Cast<u64, u8> {};
template <>
struct TypePrinter<s32> : TypePrinter_Cast<s64, s32> {};
template <>
struct TypePrinter<s16> : TypePrinter_Cast<s64, s16> {};
template <>
struct TypePrinter<s8> : TypePrinter_Cast<s64, s8> {};
template <>
struct TypePrinter<float> : TypePrinter_Cast<double, float> {};
template <>
struct TypePrinter<String> : TypePrinter_Cast<StringView, String> {};
template <>
struct TypePrinter<HybridString> : TypePrinter_Cast<StringView, HybridString> {};
template <>
struct TypePrinter<char> {
    static PLY_INLINE void print(StringWriter* sw, char c) {
        sw->writeByte(c);
    }
};
template <>
struct TypePrinter<StringView> {
    static PLY_DLL_ENTRY void print(StringWriter* sw, const StringView value);
};
template <int N>
struct TypePrinter<char[N]> {
    static PLY_INLINE void print(StringWriter* sw, const char* c) {
        PLY_ASSERT(c[N - 1] == 0); // must be a null-terminated string literal
        TypePrinter<StringView>::print(sw, {c, N - 1});
    }
};
template <>
struct TypePrinter<char*> {
    static PLY_INLINE void print(StringWriter* sw, const char* c) {
        TypePrinter<StringView>::print(sw, c);
    }
};
template <>
struct TypePrinter<const char*> {
    static PLY_INLINE void print(StringWriter* sw, const char* c) {
        TypePrinter<StringView>::print(sw, c);
    }
};

template <>
struct TypePrinter<WithRadix> {
    static PLY_DLL_ENTRY void print(StringWriter* sw, const WithRadix& value);
};
template <>
struct TypePrinter<Hex> {
    static PLY_INLINE void print(StringWriter* sw, const Hex& value) {
        TypePrinter<WithRadix>::print(sw, value);
    }
};
template <>
struct TypePrinter<u64> {
    static PLY_DLL_ENTRY void print(StringWriter* sw, u64 value);
};
template <>
struct TypePrinter<s64> {
    static PLY_DLL_ENTRY void print(StringWriter* sw, s64 value);
};
template <>
struct TypePrinter<double> {
    static PLY_DLL_ENTRY void print(StringWriter* sw, double value);
};
template <>
struct TypePrinter<bool> {
    static PLY_DLL_ENTRY void print(StringWriter* sw, bool value);
};
template <>
struct TypePrinter<CPUTimer::Duration> {
    static PLY_DLL_ENTRY void print(StringWriter* sw, CPUTimer::Duration value);
};
template <>
struct TypePrinter<EscapedString> {
    static PLY_DLL_ENTRY void print(StringWriter* sw, const EscapedString& value);
};
template <>
struct TypePrinter<XMLEscape> {
    static PLY_DLL_ENTRY void print(StringWriter* sw, const XMLEscape& value);
};
template <>
struct TypePrinter<CmdLineArg_WinCrt> {
    static PLY_DLL_ENTRY void print(StringWriter* sw, const CmdLineArg_WinCrt& value);
};

} // namespace fmt

//-----------------------------------------------------------
// String member functions
//-----------------------------------------------------------
template <typename... Args>
PLY_INLINE String String::format(const StringView fmt, const Args&... args) {
    StringWriter sw;
    sw.format(fmt, args...);
    return sw.moveToString();
}

template <typename T>
PLY_INLINE String String::from(const T& value) {
    StringWriter sw;
    sw << value;
    return sw.moveToString();
}

} // namespace ply
