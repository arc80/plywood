/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-biscuit/Tokenizer.h>

namespace ply {
namespace biscuit {

PLY_NO_INLINE Tokenizer::Tokenizer()
    : tokenData{1024 * 1024 * 1024}, fileOffsetTable{4 * 1024 * 1024} {
}

PLY_NO_INLINE void Tokenizer::setSourceInput(StringView path, StringView src) {
    this->fileLocationMap = FileLocationMap::fromView(path, src);
    this->vin.start = src.bytes;
    this->vin.end = src.end();
    this->vin.cur = src.bytes;
}

StringView tokenRepr[] = {
    "",   // Invalid
    "",   // EndOfFile
    "",   // NewLine
    "",   // LineComment
    "",   // CStyleComment
    "",   // Identifier
    "",   // NumericLiteral
    "",   // BeginString
    "",   // BeginMultilineString
    "",   // StringLiteral
    "",   // BeginStringEmbed
    "",   // EndString
    "{",  // OpenCurly
    "}",  // CloseCurly
    "(",  // OpenParen
    ")",  // CloseParen
    "[",  // OpenSquare
    "]",  // CloseSquare
    ":",  // Colon
    ";",  // Semicolon
    ".",  // Dot
    ",",  // Comma
    "=",  // Equal
    "/=", // SlashEqual
    "<",  // LessThan
    "<=", // LessThanOrEqual
    ">",  // GreaterThan
    ">=", // GreaterThanOrEqual
    "==", // DoubleEqual
    "+",  // Plus
    "-",  // Minus
    "*",  // Asterisk
    "/",  // Slash
    "%",  // Percent
    "!",  // Bang
    "~",  // Tilde
    "|",  // VerticalBar
    "||", // DoubleVerticalBar
    "&",  // Ampersand
    "&&", // DoubleAmpersand
};
PLY_STATIC_ASSERT(PLY_STATIC_ARRAY_SIZE(tokenRepr) == (u32) TokenType::Count);

String ExpandedToken::desc() const {
    switch (this->type) {
        case TokenType::EndOfFile: {
            return "end-of-file";
        }
        case TokenType::NewLine: {
            return "end-of-line";
        }
        default: {
            return String::format("'{}'", this->text);
        }
    }
}

PLY_NO_INLINE void writeCompact(Tokenizer* tkr, u32 value) {
    char* start = tkr->tokenData.beginWrite(5);
    char* end = start;
    impl::LabelEncoder::encodeValue(end, value);
    tkr->tokenData.endWrite(end - start);
}

PLY_NO_INLINE void encodeToken(Tokenizer* tkr, const ExpandedToken& expToken) {
    PLY_ASSERT(tkr->nextTokenIdx == safeDemote<u32>(tkr->tokenData.numItems()));

    // Write the token's file offset as a compact offset from the current fileOffsetTable entry.
    u32 fot_idx = expToken.tokenIdx >> 8;
    while (safeDemote<u32>(tkr->fileOffsetTable.numItems()) <= fot_idx) {
        // Write a new entry to the fileOffsetTable.
        tkr->fileOffsetTable.append(expToken.fileOffset);
    }

    // Encode the file offset
    u32 fot_value = tkr->fileOffsetTable.back();
    PLY_ASSERT(fot_value <= expToken.fileOffset);
    writeCompact(tkr, expToken.fileOffset - fot_value);

    // Write the token type as a single byte.
    PLY_STATIC_ASSERT((u32) TokenType::Count < 256);
    tkr->tokenData.append((char) expToken.type);

    if (expToken.type == TokenType::Identifier || expToken.type == TokenType::StringLiteral) {
        // Write the string key
        writeCompact(tkr, expToken.label.idx);
    }
    tkr->nextTokenIdx = safeDemote<u32>(tkr->tokenData.numItems());
}

PLY_NO_INLINE void makeEndOfFileToken(ExpandedToken& expToken, Tokenizer* tkr) {
    expToken.tokenIdx = tkr->nextTokenIdx;
    expToken.fileOffset = safeDemote<u32>(tkr->vin.end - tkr->vin.start);
    expToken.type = TokenType::EndOfFile;
}

PLY_NO_INLINE u32 expandTokenInternal(ExpandedToken& expToken, Tokenizer* tkr, u32 tokenIdx) {
    // Expand the compact token located at the specified tokenIdx and return its compact length.
    PLY_ASSERT(tokenIdx <= tkr->tokenData.numItems());
    if (tokenIdx == tkr->tokenData.numItems()) {
        makeEndOfFileToken(expToken, tkr);
        return 0;
    } else {
        expToken.tokenIdx = tokenIdx;
        const char* start = tkr->tokenData.get(tokenIdx);
        const char* data = start;
        expToken.fileOffset =
            tkr->fileOffsetTable[tokenIdx >> 8] + impl::LabelEncoder::decodeValue(data);
        expToken.type = (TokenType) (u8) *data++;
        if (expToken.type == TokenType::Identifier || expToken.type == TokenType::StringLiteral) {
            // Read the label index
            expToken.label.idx = impl::LabelEncoder::decodeValue(data);
        }
        PLY_ASSERT(data <= tkr->tokenData.end());
        u32 compactLength = safeDemote<u32>(data - start);
        u32 endOffset = safeDemote<u32>(tkr->vin.cur - tkr->vin.start);
        if (data < tkr->tokenData.end()) {
            endOffset =
                tkr->fileOffsetTable[(tokenIdx + 1) >> 8] + impl::LabelEncoder::decodeValue(data);
        }
        // FIXME: this text includes any whitespace between the two tokens
        expToken.text =
            StringView::fromRange(tkr->vin.start + expToken.fileOffset, tkr->vin.start + endOffset);
        return compactLength;
    }
}

ExpandedToken Tokenizer::expandToken(u32 tokenIdx) {
    ExpandedToken result;
    expandTokenInternal(result, this, tokenIdx);
    return result;
}

PLY_NO_INLINE void skipLineComment(Tokenizer* tkr) {
    tkr->vin.next();
    while (!tkr->vin.atEOF()) {
        char c = tkr->vin.peek();
        if (c == '\n')
            return; // End of line comment
        tkr->vin.next();
    }
}

PLY_NO_INLINE void skipCStyleComment(Tokenizer* tkr) {
    tkr->vin.next();
    while (!tkr->vin.atEOF()) {
        char c = tkr->vin.next();
        while (c == '*') {
            if (tkr->vin.atEOF())
                break;
            c = tkr->vin.next();
            if (c == '/')
                return; // End of c-style comment
        }
    }
    // FIXME: raise an error if comment is unclosed
}

PLY_NO_INLINE void readIdentifier(ExpandedToken& expToken, Tokenizer* tkr) {
    // Find end of identifier
    const char* start = tkr->vin.cur;
    tkr->vin.next();
    while (!tkr->vin.atEOF()) {
        char c = tkr->vin.peek();
        if (c < 0 || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' ||
            (c >= '0' && c <= '9')) {
            tkr->vin.next();
        } else {
            break;
        }
    }

    StringView text = StringView::fromRange(start, tkr->vin.cur);
    expToken.type = TokenType::Identifier;
    expToken.label = g_labelStorage.insert(text);
    expToken.text = text;
}

PLY_NO_INLINE void readNumeric(ExpandedToken& expToken, Tokenizer* tkr) {
    // Find end of identifier
    const char* start = tkr->vin.cur;
    tkr->vin.next();
    while (!tkr->vin.atEOF()) {
        char c = tkr->vin.peek();
        if (c >= '0' && c <= '9') {
            tkr->vin.next();
        } else {
            break;
        }
    }

    StringView text = StringView::fromRange(start, tkr->vin.cur);
    expToken.type = TokenType::NumericLiteral;
    expToken.text = text;
}

PLY_NO_INLINE void readString(ExpandedToken& expToken, Tokenizer* tkr) {
    MemOutStream mout;
    for (;;) {
        if (tkr->vin.atEOF())
            goto eofError;
        char c = tkr->vin.peek();
        if (c == '\\') {
            tkr->vin.next();
            if (tkr->vin.atEOF())
                goto eofError;
            mout << tkr->vin.next();
        } else if (c == '"') {
            if (tkr->behavior.isMultilineString) {
                if ((sptr(tkr->vin.end - tkr->vin.cur) >= 3) && (tkr->vin.cur[0] == '"') &&
                    (tkr->vin.cur[1] == '"') && (tkr->vin.cur[2] == '"')) {
                    if (mout.getSeekPos() > 0)
                        goto gotStringLiteral;
                    tkr->vin.cur += 3;
                    expToken.type = TokenType::EndString;
                    return;
                }
                mout << c;
                tkr->vin.next();
            } else {
                if (mout.getSeekPos() > 0)
                    goto gotStringLiteral;
                tkr->vin.next();
                expToken.type = TokenType::EndString;
                return;
            }
        } else if (c == '$') {
            tkr->vin.next();
            if (!tkr->vin.atEOF()) {
                c = tkr->vin.peek();
                if (c == '{') {
                    if (mout.getSeekPos() == 0) {
                        expToken.type = TokenType::BeginStringEmbed;
                        tkr->vin.next();
                        return;
                    }
                    tkr->vin.cur--; // rewind to start of '${'
                    goto gotStringLiteral;
                }
            }
            mout << '$';
        } else {
            if ((c == '\n') && !tkr->behavior.isMultilineString) {
                PLY_ASSERT(0); // FIXME: handle gracefully
            }
            mout << c;
            tkr->vin.next();
        }
    }
eofError:
    PLY_ASSERT(0);
gotStringLiteral:
    String text = mout.moveToString();
    expToken.type = TokenType::StringLiteral;
    expToken.label = g_labelStorage.insert(text);
    expToken.text = g_labelStorage.view(expToken.label);
}

PLY_NO_INLINE ExpandedToken Tokenizer::readToken() {
    ExpandedToken expToken;
    while (this->nextTokenIdx < this->tokenData.numItems()) {
        u32 offset = expandTokenInternal(expToken, this, this->nextTokenIdx);
        this->nextTokenIdx += offset;
        if (!(expToken.type == TokenType::NewLine && !this->behavior.tokenizeNewLine))
            return expToken;
    }

    for (;;) {
        if (this->vin.atEOF()) {
            expToken.tokenIdx = this->nextTokenIdx;
            expToken.fileOffset = safeDemote<u32>(this->vin.end - this->vin.start);
            expToken.type = TokenType::EndOfFile;
            return expToken;
        }

        expToken.fileOffset = safeDemote<u32>(this->vin.cur - this->vin.start);
        expToken.tokenIdx = safeDemote<u32>(this->tokenData.numItems());
        if (this->behavior.insideString) {
            readString(expToken, this);
            return expToken;
        }

        char c = this->vin.peek();
        if (c < 0 || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
            readIdentifier(expToken, this);
            goto result;
        }
        if (c >= '0' && c <= '9') {
            readNumeric(expToken, this);
            goto result;
        }
        this->vin.next();
        switch (c) {
            // newline
            case '\n': {
                if (this->behavior.tokenizeNewLine) {
                    expToken.type = TokenType::NewLine;
                    goto result;
                }
                break;
            }

            // whitespace
            case '\r':
            case '\t':
            case ' ': {
                while (!this->vin.atEOF()) {
                    char n = this->vin.peek();
                    if (!(n == '\r' || n == '\t' || n == ' '))
                        break;
                    this->vin.next();
                }
                break;
            }

            case ';': {
                expToken.type = TokenType::Semicolon;
                goto result;
            }

            case '"': {
                if (sptr(this->vin.end - this->vin.cur) >= 2) {
                    if ((this->vin.cur[0] == '"') && (this->vin.cur[1] == '"')) {
                        this->vin.cur += 2;
                        expToken.type = TokenType::BeginMultilineString;
                        if (!this->vin.atEOF() && (this->vin.peek() == '\n')) {
                            // Consume the initial newline
                            this->vin.next();
                        }
                        goto result;
                    }
                }
                expToken.type = TokenType::BeginString;
                goto result;
            }

            case '/': {
                if (!this->vin.atEOF()) {
                    if (this->vin.peek() == '/') {
                        // Line comment
                        skipLineComment(this);
                        break;
                    } else if (this->vin.peek() == '*') {
                        // C-Style comment
                        skipCStyleComment(this);
                        break;
                    } else if (this->vin.peek() == '=') {
                        // Slash equal
                        this->vin.next();
                        expToken.type = TokenType::SlashEqual;
                        goto result;
                    }
                }
                // Slash
                expToken.type = TokenType::Slash;
                goto result;
                break;
            }

            case '%': {
                expToken.type = TokenType::Percent;
                goto result;
            }

            case '!': {
                expToken.type = TokenType::Bang;
                goto result;
            }

            case '~': {
                expToken.type = TokenType::Tilde;
                goto result;
            }

            case '{': {
                expToken.type = TokenType::OpenCurly;
                goto result;
            }

            case '}': {
                expToken.type = TokenType::CloseCurly;
                goto result;
            }

            case '(': {
                expToken.type = TokenType::OpenParen;
                goto result;
            }

            case ')': {
                expToken.type = TokenType::CloseParen;
                goto result;
            }

            case '.': {
                expToken.type = TokenType::Dot;
                goto result;
            }

            case ',': {
                expToken.type = TokenType::Comma;
                goto result;
            }

            case '+': {
                expToken.type = TokenType::Plus;
                goto result;
            }

            case '-': {
                expToken.type = TokenType::Minus;
                goto result;
            }

            case '*': {
                expToken.type = TokenType::Asterisk;
                goto result;
            }

            case '=': {
                if (!this->vin.atEOF()) {
                    if (this->vin.peek() == '=') {
                        this->vin.next();
                        expToken.type = TokenType::DoubleEqual;
                        goto result;
                    }
                }
                expToken.type = TokenType::Equal;
                goto result;
            }

            case '<': {
                if (!this->vin.atEOF()) {
                    if (this->vin.peek() == '=') {
                        this->vin.next();
                        expToken.type = TokenType::LessThanOrEqual;
                        goto result;
                    }
                }
                expToken.type = TokenType::LessThan;
                goto result;
            }

            case '>': {
                if (!this->vin.atEOF()) {
                    if (this->vin.peek() == '=') {
                        this->vin.next();
                        expToken.type = TokenType::GreaterThanOrEqual;
                        goto result;
                    }
                }
                expToken.type = TokenType::GreaterThan;
                goto result;
            }

            case '|': {
                if (!this->vin.atEOF()) {
                    if (this->vin.peek() == '|') {
                        this->vin.next();
                        expToken.type = TokenType::DoubleVerticalBar;
                        goto result;
                    }
                }
                expToken.type = TokenType::VerticalBar;
                goto result;
            }

            case '&': {
                if (!this->vin.atEOF()) {
                    if (this->vin.peek() == '&') {
                        this->vin.next();
                        expToken.type = TokenType::DoubleAmpersand;
                        goto result;
                    }
                }
                expToken.type = TokenType::Ampersand;
                goto result;
            }

            default: {
                expToken.type = TokenType::Invalid;
                goto result;
            }
        }
    }

result:
    expToken.text = StringView::fromRange(this->vin.start + expToken.fileOffset, this->vin.cur);
    encodeToken(this, expToken);
    return expToken;
}

} // namespace biscuit
} // namespace ply
