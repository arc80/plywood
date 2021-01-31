<% title "Converting Values to Text" %>
<% synopsis 
Converting values to text.
%>

Plywood provides several functions to convert arbitrary values to text.

    String a = String::format("{}\n", 123);
    String b = String::from(123.0);
    MemOutStream mout;
    mout.format("\"{}\"\n", fmt::EscapedString{"C:\\plywood"});
    mout << fmt::Hex{0xbadf00d};
    String c = mout.moveToString();

The `String::format()` and `OutStream::format()` template functions accept a format string and a variable number of additional arguments. The number of additional arguments must match the number of occurrences of `"{}"` in the format string. Each argument will be converted to text according to its type. Plywood provides built-in support for the types listed in the following section. Support for additional types can be added by specializing the `fmt::TypePrinter` class template.

The `String::from()` template function accepts a single argument. The argument will be converted to text according to its type and returned as a `String`. This function supports the same types as `String::format()`.

The `OutStream::operator<<()` template function also accepts a single argument. The argument will be converted to text according to its type and written to the `OutStream`. This function supports the same types as `String::format()`.

## Built-In Type Support

Plywood provides built-in support for converting the following types to text:

* Integer types `u8`, `u16`, `u32`, `u64`, `s8`, `s16`, `s32` and `s64` are converted to their decimal representations.
* Floating-point types `float` and `double` are converted to their decimal representations.
* Text types `String`, `StringView` and `HybridString` are written byte-for-byte.
* `char` is written as-is.
* `const char*` must be null-terminated and is written byte-for-byte without the null terminator.
* `CPUTimer::Duration` is written in _mm:ss.uuuuuu_ format.
* The special types `fmt::Hex`, `fmt::WithRadix` and `fmt::EscapedString` are converted to text as described below.

<% member [strong fmt::Hex]([em value]) %>

The `fmt::Hex` constructor accepts any integer or floating-point type. When converted to text, the `fmt::Hex` object prints the hexadecimal representation of its argument.

    String::from(fmt::Hex{255});     // returns "ff"

<% member [strong fmt::WithRadix]([em value], u32 radix) %>

The `fmt::WithRadix` constructor accepts any integer or floating-point type as its first argument and an integer `radix` as its second argument. `radix` must be between 2 and 35 inclusive.

    String::from(fmt::WithRadix{4095, 8});  // returns "7777"
    String::from(fmt::WithRadix{109, 2});   // returns "1101101"

<% member [strong fmt::EscapedString](StringView view, u32 maxPoints = 0) %>

The `fmt::EscapedString` accepts a `StringView` and an optional `maxPoints` argument. When converted to text, the `fmt::EscapedString` object prints its `view` argument with special characters escaped.

    String::from(fmt::EscapedString{"\t\"\n"});    // returns "\\t\\\"\\n"

<% endMembers %>

## Adding Support for Additional Types

You can add support for conversion to text from other types by specializing the `fmt::TypePrinter` class template. For example, suppose you have a type `Foo`. Add the following declaration to a header file somewhere:

    template <>
    struct ply::fmt::TypePrinter<Foo> {
        static void print(ply::OutStream* outs, const Foo& value);
    };

Then, in a source file somewhere, implement the `print` function as follows:

    PLY_NO_INLINE void ply::fmt::TypePrinter<Foo>::print(OutStream* outs, const Foo& value) {
        // Write some output to outs here...
    }

This makes it possible to pass `Foo` to any of the `String::format()`, `OutStream::format()`, `String::from()` or `OutStream::operator<<()` functions.
