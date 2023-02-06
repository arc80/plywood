/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include "tokenizer.h"

namespace ply {

StringView tokenRepr[] = {
    "",   // TK_EOF
    "",   // TK_Invalid
    "",   // TK_Whitespace
    "",   // TK_NewLine
    "",   // TK_LineComment
    "",   // TK_CStyleComment
    "",   // TK_Identifier
    "",   // TK_NumericLiteral
    "",   // TK_BeginString
    "",   // TK_BeginMultilineString
    "",   // TK_StringLiteral
    "",   // TK_BeginStringEmbed
    "",   // TK_EndString
    "{",  // TK_OpenCurly
    "}",  // TK_CloseCurly
    "(",  // TK_OpenParen
    ")",  // TK_CloseParen
    "[",  // TK_OpenSquare
    "]",  // TK_CloseSquare
    ":",  // TK_Colon
    ";",  // TK_Semicolon
    ".",  // TK_Dot
    ",",  // TK_Comma
    "=",  // TK_Equal
    "/=", // TK_SlashEqual
    "<",  // TK_LessThan
    "<=", // TK_LessThanOrEqual
    ">",  // TK_GreaterThan
    ">=", // TK_GreaterThanOrEqual
    "==", // TK_DoubleEqual
    "+",  // TK_Plus
    "-",  // TK_Minus
    "*",  // TK_Asterisk
    "/",  // TK_Slash
    "%",  // TK_Percent
    "!",  // TK_Bang
    "~",  // TK_Tilde
    "|",  // TK_VerticalBar
    "||", // TK_DoubleVerticalBar
    "&",  // TK_Ampersand
    "&&", // TK_DoubleAmpersand
};
PLY_STATIC_ASSERT(PLY_STATIC_ARRAY_SIZE(tokenRepr) == (u32) TK_Count);

TokenKind read_string(Tokenizer* tkr) {
    PLY_ASSERT(tkr->string_status == SS_InsideString);
    MemOutStream mout;
    while (true) {
        if (!tkr->in.ensure_readable()) {
            // End of file
            if (mout.get_seek_pos() > 0)
                break;
            tkr->error("end-of-file inside string literal");
            tkr->string_status = SS_NotInsideString;
            return TK_EndString;
        }

        char c = *tkr->in.cur_byte;
        if (c == '\\') {
            // Escaped character
            tkr->in.cur_byte++;
            if (tkr->in.ensure_readable()) {
                mout << *tkr->in.cur_byte++;
            }
        } else if (c == '"') {
            auto cursor = tkr->in.get_save_point();
            if (tkr->is_multiline_string) {
                tkr->in.cur_byte++;
                char next2[2];
                tkr->in.read({next2, 2});
                if (StringView{next2} == "\"\"") {
                    // Multiline string closed by """
                    if (mout.get_seek_pos() == 0) {
                        tkr->string_status = SS_NotInsideString;
                        return TK_EndString;
                    }
                    tkr->in.rewind(cursor);
                    break;
                } else {
                    // Single " within multiline string
                    tkr->in.rewind(cursor);
                    tkr->in.cur_byte++;
                    mout << c;
                }
            } else {
                // String closed by "
                if (mout.get_seek_pos() > 0)
                    break;
                tkr->in.cur_byte++;
                tkr->string_status = SS_NotInsideString;
                return TK_EndString;
            }
        } else if ((c == '$') && tkr->tokenize_string_embeds) {
            auto cursor = tkr->in.get_save_point();
            tkr->in.cur_byte++;
            if (tkr->in.ensure_readable()) {
                c = *tkr->in.cur_byte;
                if (c == '{') {
                    if (mout.get_seek_pos() == 0) {
                        tkr->in.cur_byte++;
                        tkr->string_status = SS_InsideStringEmbed;
                        return TK_BeginStringEmbed;
                    }
                    tkr->in.rewind(cursor);
                    break;
                }
            }
            mout << '$';
        } else {
            if ((c == '\n') && !tkr->is_multiline_string) {
                if (mout.get_seek_pos() > 0)
                    break;
                tkr->error("newline inside string literal");
                tkr->string_status = SS_NotInsideString;
                return TK_EndString;
            }
            tkr->in.cur_byte++;
            mout << c;
        }
    }

    // Return a segment of this string.
    tkr->string_value = mout.moveToString();
    PLY_ASSERT(tkr->string_value);
    return TK_StringLiteral;
}

TokenKind Tokenizer::read_token() {
    // Detect EOF.
    if (!this->in.ensure_readable())
        return TK_EOF;

    // If tokenizer is inside a string literal, read the next part.
    if (this->string_status == SS_InsideString)
        return read_string(this);

    char c = *this->in.cur_byte++;
    if (c < 0 || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
        // Identifier
        while (this->in.ensure_readable()) {
            c = *this->in.cur_byte;
            if (c >= 0 && !(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') &&
                c != '_' && !(c >= '0' && c <= '9'))
                break;
            this->in.cur_byte++;
        }
        return TK_Identifier;
    }

    if (c >= '0' && c <= '9') {
        // Numeric literal
        while (this->in.ensure_readable()) {
            c = *this->in.cur_byte;
            if (!(c >= '0' && c <= '9'))
                break;
            this->in.cur_byte++;
        }
        return TK_NumericLiteral;
    }

    if (c <= ' ') {
        // Whitespace
        if (c == '\n' && this->tokenize_new_line)
            return TK_NewLine;
        while (this->in.ensure_readable()) {
            char c = *this->in.cur_byte;
            if (c == '\n' && this->tokenize_new_line)
                break;
            if (c > ' ')
                break;
            this->in.cur_byte++;
        }
        return TK_Whitespace;
    }

    if (c == '"') {
        // String literal
        this->string_status = SS_InsideString;
        if (this->tokenize_multiline_strings) {
            // Check for start of multiline string.
            auto cursor = this->in.get_save_point();
            char next2[2];
            this->in.read({next2, 2});
            this->is_multiline_string = (StringView{next2} == "\"\"");
            if (this->is_multiline_string) {
                if (this->in.ensure_readable() && *this->in.cur_byte == '\n') {
                    // Consume the newline after """.
                    this->in.cur_byte++;
                }
                return TK_BeginMultilineString;
            }
            this->in.rewind(cursor);
        }
        return TK_BeginString;
    }

    if (c == '/' && in.ensure_readable()) {
        c = *this->in.cur_byte;
        if (c == '/' && this->tokenize_line_comments) {
            // Line comment
            this->in.cur_byte++;
            while (this->in.ensure_readable()) {
                c = *this->in.cur_byte;
                if (c == '\n')
                    break;
                this->in.cur_byte++;
            }
            return TK_LineComment;
        }

        if (c == '*' && this->tokenize_c_style_comments) {
            // C-style comment
            this->in.cur_byte++;
            while (true) {
                if (this->in.ensure_readable()) {
                    this->error("end-of-file inside block comment");
                    break;
                }
                c = *this->in.cur_byte++;
                if (c == '*') {
                    if (this->in.ensure_readable()) {
                        c = *this->in.cur_byte;
                        if (c == '/') {
                            this->in.cur_byte++;
                            break;
                        }
                    }
                }
            }
            return TK_CStyleComment;
        }
    }

    // Punctuation
    switch (c) {
        case ';':
            return TK_Semicolon;
        case '%':
            return TK_Percent;
        case '!':
            return TK_Bang;
        case '~':
            return TK_Tilde;
        case '{':
            return TK_OpenCurly;
        case '}': {
            if (this->string_status == SS_InsideStringEmbed) {
                this->string_status = SS_InsideString;
            }
            return TK_CloseCurly;
        }
        case '(':
            return TK_OpenParen;
        case ')':
            return TK_CloseParen;
        case '.':
            return TK_Dot;
        case ',':
            return TK_Comma;
        case '+':
            return TK_Plus;
        case '-':
            return TK_Minus;
        case '*':
            return TK_Asterisk;
        case '/':
            if (this->in.ensure_readable() && *this->in.cur_byte == '=') {
                this->in.cur_byte++;
                return TK_SlashEqual;
            }
            return TK_Slash;
        case '=':
            if (this->in.ensure_readable() && *this->in.cur_byte == '=') {
                this->in.cur_byte++;
                return TK_DoubleEqual;
            }
            return TK_Equal;
        case '<':
            if (this->in.ensure_readable() && *this->in.cur_byte == '=') {
                this->in.cur_byte++;
                return TK_LessThanOrEqual;
            }
            return TK_LessThan;
        case '>':
            if (this->in.ensure_readable() && *this->in.cur_byte == '=') {
                this->in.cur_byte++;
                return TK_GreaterThanOrEqual;
            }
            return TK_GreaterThan;
        case '|':
            if (this->in.ensure_readable() && *this->in.cur_byte == '|') {
                this->in.cur_byte++;
                return TK_DoubleVerticalBar;
            }
            return TK_VerticalBar;
        case '&':
            if (this->in.ensure_readable() && *this->in.cur_byte == '&') {
                this->in.cur_byte++;
                return TK_DoubleAmpersand;
            }
            return TK_Ampersand;
        default:
            break;
    }

    return TK_Invalid;
}

} // namespace ply
