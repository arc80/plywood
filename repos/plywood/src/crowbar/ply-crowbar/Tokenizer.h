/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-crowbar/Core.h>
#include <ply-runtime/container/InternedStrings.h>
#include <ply-runtime/io/text/FileLocationMap.h>

namespace ply {
namespace crowbar {

enum class TokenType {
    Invalid,
    EndOfFile,
    NewLine,
    LineComment,
    CStyleComment,

    Identifier,
    NumericLiteral,
    StringLiteral,

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

    Count,
};

// Full information about a single token.
// ExpandedTokens are generally temporary objects. The complete set of tokens is stored in the
// Tokenizer in a more compact way.
struct ExpandedToken {
    u32 tokenIdx = 0;
    u32 fileOffset = 0;
    TokenType type = TokenType::Invalid;
    u32 stringKey = 0; // Interned string.
    StringView text;

    String desc() const;
};

struct Tokenizer {
    // The input file being read.
    struct {
        const char* start = nullptr;
        const char* end = nullptr;
        const char* cur = nullptr;

        PLY_INLINE bool atEOF() const {
            return this->cur >= this->end;
        }
        PLY_INLINE char peek() const {
            PLY_ASSERT(this->cur < this->end);
            return *this->cur;
        }
        PLY_INLINE char next() {
            PLY_ASSERT(this->cur < this->end);
            return *this->cur++;
        }
    } vin;

    // FileLocationMap is initialized when setSourceInput is called.
    FileLocationMap fileLocationMap;

    // Token data stored in a compact form.
    // Token indices are offsets into this buffer.
    BigPool<> tokenData;
    u32 nextTokenIdx = 0; // Tokens can be pushed back into the tokenData queue

    // There is entry in the fileOffsetTable for every 256 bytes of tokenData.
    BigPool<u32> fileOffsetTable;

    // InternedStrings lets us compare identifiers just by comparing their 32-bit keys.
    InternedStrings* internedStrings = nullptr;

    // tokenizeNewLine can be changed on the fly.
    bool tokenizeNewLine = false;

    Tokenizer();
    void setSourceInput(StringView src);
    ExpandedToken readToken();
    ExpandedToken expandToken(u32 tokenIdx);
    PLY_INLINE void rewindTo(u32 tokenIdx) {
        PLY_ASSERT(tokenIdx <= this->nextTokenIdx);
        this->nextTokenIdx = tokenIdx;
    }
};

} // namespace crowbar
} // namespace ply