/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-biscuit/Tokenizer.h>

namespace ply {
namespace biscuit {

PLY_NO_INLINE Tokenizer::Tokenizer()
    : token_data{1024 * 1024 * 1024}, file_offset_table{4 * 1024 * 1024} {
}

PLY_NO_INLINE void Tokenizer::set_source_input(StringView path, StringView src) {
    this->file_location_map = FileLocationMap::from_view(path, src);
    this->vin.start = src.bytes;
    this->vin.end = src.end();
    this->vin.cur = src.bytes;
}

StringView token_repr[] = {
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
PLY_STATIC_ASSERT(PLY_STATIC_ARRAY_SIZE(token_repr) == (u32) TokenType::Count);

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

PLY_NO_INLINE void write_compact(Tokenizer* tkr, u32 value) {
    char* start = tkr->token_data.begin_write(5);
    char* end = start;
    impl::LabelEncoder::encode_value(end, value);
    tkr->token_data.end_write(end - start);
}

PLY_NO_INLINE void encode_token(Tokenizer* tkr, const ExpandedToken& exp_token) {
    PLY_ASSERT(tkr->next_token_idx == safe_demote<u32>(tkr->token_data.num_items()));

    // Write the token's file offset as a compact offset from the current
    // file_offset_table entry.
    u32 fot_idx = exp_token.token_idx >> 8;
    while (safe_demote<u32>(tkr->file_offset_table.num_items()) <= fot_idx) {
        // Write a new entry to the file_offset_table.
        tkr->file_offset_table.append(exp_token.file_offset);
    }

    // Encode the file offset
    u32 fot_value = tkr->file_offset_table.back();
    PLY_ASSERT(fot_value <= exp_token.file_offset);
    write_compact(tkr, exp_token.file_offset - fot_value);

    // Write the token type as a single byte.
    PLY_STATIC_ASSERT((u32) TokenType::Count < 256);
    tkr->token_data.append((char) exp_token.type);

    if (exp_token.type == TokenType::Identifier ||
        exp_token.type == TokenType::StringLiteral) {
        // Write the string key
        write_compact(tkr, exp_token.label.idx);
    }
    tkr->next_token_idx = safe_demote<u32>(tkr->token_data.num_items());
}

PLY_NO_INLINE void make_end_of_file_token(ExpandedToken& exp_token, Tokenizer* tkr) {
    exp_token.token_idx = tkr->next_token_idx;
    exp_token.file_offset = safe_demote<u32>(tkr->vin.end - tkr->vin.start);
    exp_token.type = TokenType::EndOfFile;
}

PLY_NO_INLINE u32 expand_token_internal(ExpandedToken& exp_token, Tokenizer* tkr,
                                        u32 token_idx) {
    // Expand the compact token located at the specified token_idx and return its
    // compact length.
    PLY_ASSERT(token_idx <= tkr->token_data.num_items());
    if (token_idx == tkr->token_data.num_items()) {
        make_end_of_file_token(exp_token, tkr);
        return 0;
    } else {
        exp_token.token_idx = token_idx;
        const char* start = tkr->token_data.get(token_idx);
        const char* data = start;
        exp_token.file_offset = tkr->file_offset_table[token_idx >> 8] +
                                impl::LabelEncoder::decode_value(data);
        exp_token.type = (TokenType) (u8) *data++;
        if (exp_token.type == TokenType::Identifier ||
            exp_token.type == TokenType::StringLiteral) {
            // Read the label index
            exp_token.label.idx = impl::LabelEncoder::decode_value(data);
        }
        PLY_ASSERT(data <= tkr->token_data.end());
        u32 compact_length = safe_demote<u32>(data - start);
        u32 end_offset = safe_demote<u32>(tkr->vin.cur - tkr->vin.start);
        if (data < tkr->token_data.end()) {
            end_offset = tkr->file_offset_table[(token_idx + 1) >> 8] +
                         impl::LabelEncoder::decode_value(data);
        }
        // FIXME: this text includes any whitespace between the two tokens
        exp_token.text = StringView::from_range(tkr->vin.start + exp_token.file_offset,
                                                tkr->vin.start + end_offset);
        return compact_length;
    }
}

ExpandedToken Tokenizer::expand_token(u32 token_idx) {
    ExpandedToken result;
    expand_token_internal(result, this, token_idx);
    return result;
}

PLY_NO_INLINE void skip_line_comment(Tokenizer* tkr) {
    tkr->vin.next();
    while (!tkr->vin.at_eof()) {
        char c = tkr->vin.peek();
        if (c == '\n')
            return; // End of line comment
        tkr->vin.next();
    }
}

PLY_NO_INLINE void skip_cstyle_comment(Tokenizer* tkr) {
    tkr->vin.next();
    while (!tkr->vin.at_eof()) {
        char c = tkr->vin.next();
        while (c == '*') {
            if (tkr->vin.at_eof())
                break;
            c = tkr->vin.next();
            if (c == '/')
                return; // End of c-style comment
        }
    }
    // FIXME: raise an error if comment is unclosed
}

PLY_NO_INLINE void read_identifier(ExpandedToken& exp_token, Tokenizer* tkr) {
    // Find end of identifier
    const char* start = tkr->vin.cur;
    tkr->vin.next();
    while (!tkr->vin.at_eof()) {
        char c = tkr->vin.peek();
        if (c < 0 || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' ||
            (c >= '0' && c <= '9')) {
            tkr->vin.next();
        } else {
            break;
        }
    }

    StringView text = StringView::from_range(start, tkr->vin.cur);
    exp_token.type = TokenType::Identifier;
    exp_token.label = g_labelStorage.insert(text);
    exp_token.text = text;
}

PLY_NO_INLINE void read_numeric(ExpandedToken& exp_token, Tokenizer* tkr) {
    // Find end of identifier
    const char* start = tkr->vin.cur;
    tkr->vin.next();
    while (!tkr->vin.at_eof()) {
        char c = tkr->vin.peek();
        if (c >= '0' && c <= '9') {
            tkr->vin.next();
        } else {
            break;
        }
    }

    StringView text = StringView::from_range(start, tkr->vin.cur);
    exp_token.type = TokenType::NumericLiteral;
    exp_token.text = text;
}

PLY_NO_INLINE void read_string(ExpandedToken& exp_token, Tokenizer* tkr) {
    MemOutStream mout;
    for (;;) {
        if (tkr->vin.at_eof())
            goto eof_error;
        char c = tkr->vin.peek();
        if (c == '\\') {
            tkr->vin.next();
            if (tkr->vin.at_eof())
                goto eof_error;
            mout << tkr->vin.next();
        } else if (c == '"') {
            if (tkr->behavior.is_multiline_string) {
                if ((sptr(tkr->vin.end - tkr->vin.cur) >= 3) &&
                    (tkr->vin.cur[0] == '"') && (tkr->vin.cur[1] == '"') &&
                    (tkr->vin.cur[2] == '"')) {
                    if (mout.get_seek_pos() > 0)
                        goto got_string_literal;
                    tkr->vin.cur += 3;
                    exp_token.type = TokenType::EndString;
                    return;
                }
                mout << c;
                tkr->vin.next();
            } else {
                if (mout.get_seek_pos() > 0)
                    goto got_string_literal;
                tkr->vin.next();
                exp_token.type = TokenType::EndString;
                return;
            }
        } else if (c == '$') {
            tkr->vin.next();
            if (!tkr->vin.at_eof()) {
                c = tkr->vin.peek();
                if (c == '{') {
                    if (mout.get_seek_pos() == 0) {
                        exp_token.type = TokenType::BeginStringEmbed;
                        tkr->vin.next();
                        return;
                    }
                    tkr->vin.cur--; // rewind to start of '${'
                    goto got_string_literal;
                }
            }
            mout << '$';
        } else {
            if ((c == '\n') && !tkr->behavior.is_multiline_string) {
                PLY_ASSERT(0); // FIXME: handle gracefully
            }
            mout << c;
            tkr->vin.next();
        }
    }
eof_error:
    PLY_ASSERT(0);
got_string_literal:
    String text = mout.move_to_string();
    exp_token.type = TokenType::StringLiteral;
    exp_token.label = g_labelStorage.insert(text);
    exp_token.text = g_labelStorage.view(exp_token.label);
}

PLY_NO_INLINE ExpandedToken Tokenizer::read_token() {
    ExpandedToken exp_token;
    while (this->next_token_idx < this->token_data.num_items()) {
        u32 offset = expand_token_internal(exp_token, this, this->next_token_idx);
        this->next_token_idx += offset;
        if (!(exp_token.type == TokenType::NewLine &&
              !this->behavior.tokenize_new_line))
            return exp_token;
    }

    for (;;) {
        if (this->vin.at_eof()) {
            exp_token.token_idx = this->next_token_idx;
            exp_token.file_offset = safe_demote<u32>(this->vin.end - this->vin.start);
            exp_token.type = TokenType::EndOfFile;
            return exp_token;
        }

        exp_token.file_offset = safe_demote<u32>(this->vin.cur - this->vin.start);
        exp_token.token_idx = safe_demote<u32>(this->token_data.num_items());
        if (this->behavior.inside_string) {
            read_string(exp_token, this);
            return exp_token;
        }

        char c = this->vin.peek();
        if (c < 0 || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
            read_identifier(exp_token, this);
            goto result;
        }
        if (c >= '0' && c <= '9') {
            read_numeric(exp_token, this);
            goto result;
        }
        this->vin.next();
        switch (c) {
            // newline
            case '\n': {
                if (this->behavior.tokenize_new_line) {
                    exp_token.type = TokenType::NewLine;
                    goto result;
                }
                break;
            }

            // whitespace
            case '\r':
            case '\t':
            case ' ': {
                while (!this->vin.at_eof()) {
                    char n = this->vin.peek();
                    if (!(n == '\r' || n == '\t' || n == ' '))
                        break;
                    this->vin.next();
                }
                break;
            }

            case ';': {
                exp_token.type = TokenType::Semicolon;
                goto result;
            }

            case '"': {
                if (sptr(this->vin.end - this->vin.cur) >= 2) {
                    if ((this->vin.cur[0] == '"') && (this->vin.cur[1] == '"')) {
                        this->vin.cur += 2;
                        exp_token.type = TokenType::BeginMultilineString;
                        if (!this->vin.at_eof() && (this->vin.peek() == '\n')) {
                            // Consume the initial newline
                            this->vin.next();
                        }
                        goto result;
                    }
                }
                exp_token.type = TokenType::BeginString;
                goto result;
            }

            case '/': {
                if (!this->vin.at_eof()) {
                    if (this->vin.peek() == '/') {
                        // Line comment
                        skip_line_comment(this);
                        break;
                    } else if (this->vin.peek() == '*') {
                        // C-Style comment
                        skip_cstyle_comment(this);
                        break;
                    } else if (this->vin.peek() == '=') {
                        // Slash equal
                        this->vin.next();
                        exp_token.type = TokenType::SlashEqual;
                        goto result;
                    }
                }
                // Slash
                exp_token.type = TokenType::Slash;
                goto result;
                break;
            }

            case '%': {
                exp_token.type = TokenType::Percent;
                goto result;
            }

            case '!': {
                exp_token.type = TokenType::Bang;
                goto result;
            }

            case '~': {
                exp_token.type = TokenType::Tilde;
                goto result;
            }

            case '{': {
                exp_token.type = TokenType::OpenCurly;
                goto result;
            }

            case '}': {
                exp_token.type = TokenType::CloseCurly;
                goto result;
            }

            case '(': {
                exp_token.type = TokenType::OpenParen;
                goto result;
            }

            case ')': {
                exp_token.type = TokenType::CloseParen;
                goto result;
            }

            case '.': {
                exp_token.type = TokenType::Dot;
                goto result;
            }

            case ',': {
                exp_token.type = TokenType::Comma;
                goto result;
            }

            case '+': {
                exp_token.type = TokenType::Plus;
                goto result;
            }

            case '-': {
                exp_token.type = TokenType::Minus;
                goto result;
            }

            case '*': {
                exp_token.type = TokenType::Asterisk;
                goto result;
            }

            case '=': {
                if (!this->vin.at_eof()) {
                    if (this->vin.peek() == '=') {
                        this->vin.next();
                        exp_token.type = TokenType::DoubleEqual;
                        goto result;
                    }
                }
                exp_token.type = TokenType::Equal;
                goto result;
            }

            case '<': {
                if (!this->vin.at_eof()) {
                    if (this->vin.peek() == '=') {
                        this->vin.next();
                        exp_token.type = TokenType::LessThanOrEqual;
                        goto result;
                    }
                }
                exp_token.type = TokenType::LessThan;
                goto result;
            }

            case '>': {
                if (!this->vin.at_eof()) {
                    if (this->vin.peek() == '=') {
                        this->vin.next();
                        exp_token.type = TokenType::GreaterThanOrEqual;
                        goto result;
                    }
                }
                exp_token.type = TokenType::GreaterThan;
                goto result;
            }

            case '|': {
                if (!this->vin.at_eof()) {
                    if (this->vin.peek() == '|') {
                        this->vin.next();
                        exp_token.type = TokenType::DoubleVerticalBar;
                        goto result;
                    }
                }
                exp_token.type = TokenType::VerticalBar;
                goto result;
            }

            case '&': {
                if (!this->vin.at_eof()) {
                    if (this->vin.peek() == '&') {
                        this->vin.next();
                        exp_token.type = TokenType::DoubleAmpersand;
                        goto result;
                    }
                }
                exp_token.type = TokenType::Ampersand;
                goto result;
            }

            default: {
                exp_token.type = TokenType::Invalid;
                goto result;
            }
        }
    }

result:
    exp_token.text =
        StringView::from_range(this->vin.start + exp_token.file_offset, this->vin.cur);
    encode_token(this, exp_token);
    return exp_token;
}

} // namespace biscuit
} // namespace ply
