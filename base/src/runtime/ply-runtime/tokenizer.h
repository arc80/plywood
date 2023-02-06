/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-runtime.h>

namespace ply {

enum TokenKind {
    TK_EOF = 0,
    TK_Invalid,
    TK_Whitespace,
    TK_NewLine,
    TK_LineComment,
    TK_CStyleComment,
    TK_Identifier,
    TK_NumericLiteral,
    TK_BeginString,
    TK_BeginMultilineString,
    TK_StringLiteral,
    TK_BeginStringEmbed, // Closed by CloseCurly
    TK_EndString,
    TK_OpenCurly,
    TK_CloseCurly,
    TK_OpenParen,
    TK_CloseParen,
    TK_OpenSquare,
    TK_CloseSquare,
    TK_Colon,
    TK_Semicolon,
    TK_Dot,
    TK_Comma,
    TK_Equal,
    TK_SlashEqual,
    TK_LessThan,
    TK_LessThanOrEqual,
    TK_GreaterThan,
    TK_GreaterThanOrEqual,
    TK_DoubleEqual,
    TK_Plus,
    TK_Minus,
    TK_Asterisk,
    TK_Slash,
    TK_Percent,
    TK_Bang,
    TK_Tilde,
    TK_VerticalBar,
    TK_DoubleVerticalBar,
    TK_Ampersand,
    TK_DoubleAmpersand,
    TK_Count,
};

enum StringStatus {
    SS_NotInsideString,
    SS_InsideString,
    SS_InsideStringEmbed,
};

struct Tokenizer {
    // Configuration:
    InStream in;
    Func<void(StringView)> error;
    bool tokenize_line_directives = false;
    bool tokenize_line_comments = false;
    bool tokenize_c_style_comments = false;
    bool tokenize_new_line = false;
    bool tokenize_negative_numbers = false;
    bool tokenize_multiline_strings = false;
    bool tokenize_string_embeds = false;
    bool tokenize_right_shift = false;

    // read_token() modifies these members:
    StringStatus string_status = SS_NotInsideString;
    bool is_multiline_string = false;
    String string_value;

    Tokenizer(InStream&& in) : in{std::move(in)} {
    }
    TokenKind read_token();
};

} // namespace ply
