/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                   ┃
┃    ╱   ╱╲    Plywood Multimedia Toolkit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/          ┃
┃    └──┴┴┴┘                                 ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

namespace ply {

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

enum TokenType {
    TT_Invalid,
    TT_EOF,
    TT_NewLine,
    TT_LineComment,
    TT_CStyleComment,
    TT_Identifier,
    TT_NumericLiteral,
    TT_BeginString,
    TT_BeginMultilineString,
    TT_StringLiteral,
    TT_BeginStringEmbed, // Closed by CloseCurly
    TT_EndString,
    TT_OpenCurly,
    TT_CloseCurly,
    TT_OpenParen,
    TT_CloseParen,
    TT_OpenSquare,
    TT_CloseSquare,
    TT_Colon,
    TT_Semicolon,
    TT_Dot,
    TT_Comma,
    TT_Equal,
    TT_SlashEqual,
    TT_LessThan,
    TT_LessThanOrEqual,
    TT_GreaterThan,
    TT_GreaterThanOrEqual,
    TT_DoubleEqual,
    TT_Plus,
    TT_Minus,
    TT_Asterisk,
    TT_Slash,
    TT_Percent,
    TT_Bang,
    TT_Tilde,
    TT_VerticalBar,
    TT_DoubleVerticalBar,
    TT_Ampersand,
    TT_DoubleAmpersand,
    TT_Count,
};

struct Tokenizer {
    // Configuration:
    InStream in;
    bool tokenize_line_directives = false;
    bool tokenize_line_comments = false;
    bool tokenize_c_style_comments = false;
    bool tokenize_negative_numbers = false;
    bool tokenize_multiline_strings = false;
    bool tokenize_string_embeds = false;
    bool tokenize_right_shift = false;

    // read_token() modifies these members:
    bool is_inside_string = false;
    ChunkCursor token_start;

    Tokenizer();
    TokenType read_token();
    void attach_stream(InStream&& in) {
        this->in = std::move(in);
    }
    InStream detach_stream() {
        return std::move(this->in);
    }
};

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
                if ((sptr(tkr->vin.end - tkr->vin.cur) >= 3) &&
                    (tkr->vin.cur[0] == '"') && (tkr->vin.cur[1] == '"') &&
                    (tkr->vin.cur[2] == '"')) {
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

TokenType Tokenizer::read_token() {
    // Save start of token.
    this->token_start = this->in.read_cursor;

    // Make shorthand variable: cur_byte
    char*& cur_byte = this->in.cur_byte;

    // Detect EOF.
    if (!this->in.ensure_bytes_available())
        return TT_EOF;

    // If tokenizer is inside a string literal, read the next part.
    if (tkr->is_inside_string)
        goto inside_string;

    char c = *cur_byte++;
    if (c < 0 || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
        // ┏━━━━━━━━━━━━━━┓
        // ┃  Identifier  ┃
        // ┗━━━━━━━━━━━━━━┛
        while (in.ensure_bytes_available()) {
            c = *cur_byte;
            if (c >= 0 && !(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') &&
                c != '_' && !(c >= '0' && c <= '9'))
                break;
            cur_byte++;
        }
        return TT_Identifier;
    }

    if (c >= '0' && c <= '9') {
        // ┏━━━━━━━━━━━━━━━━━━━┓
        // ┃  Numeric Literal  ┃
        // ┗━━━━━━━━━━━━━━━━━━━┛
        while (in.ensure_bytes_available()) {
            c = *in.cur_byte;
            if (!(c >= '0' && c <= '9'))
                break;
            in.cur_byte++;
        }
        return TT_NumericLiteral;
    }

    if (c <= ' ') {
        // ┏━━━━━━━━━━━━━━┓
        // ┃  Whitespace  ┃
        // ┗━━━━━━━━━━━━━━┛
        if (c == '\n' && tkr->tokenize_new_line)
            return TT_NewLine;
        while (in.ensure_bytes_available()) {
            char c = *in.cur_byte;
            if (c == '\n' && tkr->tokenize_new_line)
                break;
            if (c > ' ')
                break;
            in.cur_byte++;
        }
        return TT_Whitespace;
    }

    if (c == '"') {
        // ┏━━━━━━━━━━━━━━━━━━┓
        // ┃  String Literal  ┃
        // ┗━━━━━━━━━━━━━━━━━━┛
        if (in.ensure_bytes_available(2)) {
            in.cur_byte += 2;
            return TT_BeginMultilineString;
        }
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

    if (c == '/' && in.ensure_bytes_available()) {
        c = *cur_byte;
        if (c == '/' && this->tokenize_line_comments) {
            // ┏━━━━━━━━━━━━━━━━┓
            // ┃  Line Comment  ┃
            // ┗━━━━━━━━━━━━━━━━┛
            cur_byte++;
            while (in.ensure_bytes_available()) {
                c = *cur_byte;
                if (c == '\n')
                    break;
                cur_byte++;
            }
            return TT_LineComment;
        }

        if (c == '*' && this->tokenize_c_style_comments) {
            // ┏━━━━━━━━━━━━━━━━━━━┓
            // ┃  C-Style Comment  ┃
            // ┗━━━━━━━━━━━━━━━━━━━┛
            cur_byte++;
            while (in.ensure_bytes_available()) {
                c = *cur_byte++;
                if (c == '*') {
                    if (in.ensure_bytes_available()) {
                        c = *cur_byte;
                        if (c == '/') {
                            cur_byte++;
                            break;
                        }
                    }
                }
            }
            // The caller is responsible for unclosed comment diagnostics.
            return TT_CStyleComment;
        }
    }

    // ┏━━━━━━━━━━━━━━━┓
    // ┃  Punctuation  ┃
    // ┗━━━━━━━━━━━━━━━┛
    switch (c) {
        case ';':
            return TT_Semicolon;
        case '%':
            return TT_Percent;
        case '!':
            return TT_Bang;
        case '~':
            return TT_Tilde;
        case '{':
            return TT_OpenCurly;
        case '}':
            return TT_CloseCurly;
        case '(':
            return TT_OpenParen;
        case ')':
            return TT_CloseParen;
        case '.':
            return TT_Dot;
        case ',':
            return TT_Comma;
        case '+':
            return TT_Plus;
        case '-':
            return TT_Minus;
        case '*':
            return TT_Asterisk;
        case '/':
            if (this->in.ensure_bytes_available() && *cur_byte == '=') {
                cur_byte++;
                return TT_SlashEqual;
            }
            return TT_Slash;
        case '=':
            if (this->in.ensure_bytes_available() && *cur_byte == '=') {
                cur_byte++;
                return TT_DoubleEqual;
            }
            return TT_Equal;
        case '<':
            if (this->in.ensure_bytes_available() && *cur_byte == '=') {
                cur_byte++;
                return TT_LessThanOrEqual;
            }
            return TT_LessThan;
        case '>':
            if (this->in.ensure_bytes_available() && *cur_byte == '=') {
                cur_byte++;
                return TT_GreaterThanOrEqual;
            }
            return TT_GreaterThan;
        case '|':
            if (this->in.ensure_bytes_available() && *cur_byte == '|') {
                cur_byte++;
                return TT_DoubleVerticalBar;
            }
            return TT_VerticalBar;
        case '&':
            if (this->in.ensure_bytes_available() && *cur_byte == '&') {
                cur_byte++;
                return TT_DoubleAmpersand;
            }
            return TT_Ampersand;
        default:
            break;
    }

    return TT_Invalid;
}

} // namespace ply
