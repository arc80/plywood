/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-biscuit/Core.h>
#include <ply-runtime/io/text/FileLocationMap.h>

namespace ply {
namespace biscuit {

enum class TokenType {
    Invalid,
    EndOfFile,
    NewLine,
    LineComment,
    CStyleComment,

    Identifier,
    NumericLiteral,
    BeginString,
    BeginMultilineString,
    StringLiteral,
    BeginStringEmbed, // Closed by CloseCurly
    EndString,

    OpenCurly,
    CloseCurly,
    OpenParen,
    CloseParen,
    OpenSquare,
    CloseSquare,

    Colon,
    Semicolon,
    Dot,
    Comma,

    Equal,
    SlashEqual,
    LessThan,
    LessThanOrEqual,
    GreaterThan,
    GreaterThanOrEqual,
    DoubleEqual,

    Plus,
    Minus,
    Asterisk,
    Slash,
    Percent,
    Bang,
    Tilde,

    VerticalBar,
    DoubleVerticalBar,
    Ampersand,
    DoubleAmpersand,

    Count,
};

// Full information about a single token.
// ExpandedTokens are generally temporary objects. The complete set of tokens is stored
// in the Tokenizer in a more compact way.
struct ExpandedToken {
    u32 token_idx = 0;
    u32 file_offset = 0;
    TokenType type = TokenType::Invalid;
    Label label;
    StringView text;

    String desc() const;
};

struct Tokenizer {
    // The input file being read.
    struct {
        const char* start = nullptr;
        const char* end = nullptr;
        const char* cur = nullptr;

        bool at_eof() const {
            return this->cur >= this->end;
        }
        char peek() const {
            PLY_ASSERT(this->cur < this->end);
            return *this->cur;
        }
        char next() {
            PLY_ASSERT(this->cur < this->end);
            return *this->cur++;
        }
    } vin;

    // FileLocationMap is initialized when set_source_input is called.
    FileLocationMap file_location_map;

    // Token data stored in a compact form.
    // Token indices are offsets into this buffer.
    BigPool<> token_data;
    u32 next_token_idx = 0; // Tokens can be pushed back into the token_data queue

    // There is entry in the file_offset_table for every 256 bytes of token_data.
    BigPool<u32> file_offset_table;

    // Behavior that can be changed on the fly by the parser.
    struct Behavior {
        bool tokenize_new_line = true;
        bool inside_string = false;
        bool is_multiline_string = false;
    };
    Behavior behavior;

    Tokenizer();
    void set_source_input(StringView path, StringView src);
    ExpandedToken read_token();
    ExpandedToken expand_token(u32 token_idx);
    void rewind_to(u32 token_idx) {
        PLY_ASSERT(token_idx <= this->next_token_idx);
        this->next_token_idx = token_idx;
    }
};

} // namespace biscuit
} // namespace ply