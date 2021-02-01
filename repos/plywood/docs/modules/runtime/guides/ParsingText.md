<% title "Parsing Text" %>
<% synopsis 
Parsing and converting text to other types.
%>

Plywood provides the following template functions to parse and convert text to other types:

* `InStream::parse<...>()` returns a value whose type depends on its template argument.
* `InStream::readString<...>()` always returns a `String`.
* `ViewInStream::readView<...>()` always returns a `StringView`.

Each of these functions requires a template argument that describes the expected string format. The template argument can either be a _direct type_ or a special _format class_.

## Parsing a Direct Type

When the template argument is a direct type, the `parse()` function returns that type directly. Plywood provides built-in support to directly parse the following types:

* The integer types `u8`, `u16`, `u32`, `u64`, `s8`, `s16`, `s32` and `s64`. By default, these are expected in decimal (base 10) notation.
* The floating-point types `float` and `double`. By default, these are expected in decimal (base 10) notation.

When such types are parsed, the template argument must always be specified explicitly, since the compiler has no way to deduce it:

    u32 a = ins.parse<u32>();

Furthermore, when such types are parsed, each function accepts an optional formatting argument whose type depends on the template argument. For example, when parsing a numeric type, an optional `fmt::Radix` argument is accepted:

    u32 b = ins.parse<u32>(fmt::Radix{16});

### Adding Support for Direct Types

You can add support for direct parsing of additional types by specializing the `fmt::TypeParser` class template. Every `fmt::TypeParser` specialization must define two static member functions:

* _Format_ `defaultFormat()` determines the type and default value of the optional formatting argument.
* _Type_ `parse(InStream* ins, const` _Format_`& format)` implements the parse function.

See the built-in `fmt::TypeParser` specializations for examples.

## Parsing a Format Class

When the template argument is a format class, the `parse()` function returns a value whose type depends on the format class. Plywood provides built-in support for the format classes listed below. When a format class is parsed, each function accepts an optional formatting argument whose type must match the format class. Therefore, if a formatting argument is passed to a parse function, the template argument doesn't need to be specified explicitly since the compiler can deduce it.

    String a = ins.readString<fmt::Identifier>(); // explicit template argument
    String b = ins.readString(fmt::Identifier{}); // template argument deduced

<% member [strong fmt::Identifier](u32 flags = 0) %>

Parses an alphanumeric identifier in greedy fashion. Consumes all bytes that are ASCII letters, underscores, or high bytes (having a value greater than or equal to 128). Note that if the string is encoded as UTF-8, all Unicode points greater than or equal to 128 (such as emoji) are considered alphanumeric and will be consumed. After the first byte, ASCII digits are consumed too. If the `fmt::WithDollarSign` flag is passed, `'$'` characters are also consumed. 

<% member [strong fmt::Line]() %>

Parses a line of text by consuming all bytes up to `'\n'`.

<% member [strong fmt::Whitespace]() %>

Parses whitespace. All `' '`, `'\t'`, `'\r'` and `'\n'` bytes are consumed.

<% member [strong fmt::NonWhitespace]() %>

Parses non-whitespace. All bytes that are not consumed by `fmt::Whitespace` are consumed.

<% endMembers %>

## Adding Support for Format Classes

You can add support for parsing additional format classes by specializing the `fmt::FormatParser` class template. Every `fmt::FormatParser` specialization must define a single static member function:

<% member [em Type] [strong parse](InStream* ins, const [em Format]& format) %>

See the built-in `fmt::FormatParser` specializations for examples.

<% endMembers %>
