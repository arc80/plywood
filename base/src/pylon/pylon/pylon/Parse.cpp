/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <pylon/Core.h>
#include <pylon/Parse.h>

namespace pylon {

bool is_alnum_unit(u32 c) {
    return (c == '_') || (c == '$') || (c == '-') || (c == '.') ||
           (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
           (c >= 128);
}

void Parser::dump_error(const ParseError& error, OutStream& out) const {
    FileLocation error_loc = this->file_loc_map.get_file_location(error.file_ofs);
    out.format("({}, {}): error: {}\n", error_loc.line_number, error_loc.column_number,
               error.message);
    for (u32 i = 0; i < error.context.num_items(); i++) {
        const ParseError::Scope& scope = error.context.back(-(s32) i - 1);
        FileLocation context_loc = this->file_loc_map.get_file_location(scope.file_ofs);
        out.format("({}, {}) ", context_loc.line_number, context_loc.column_number);
        switch (scope.type) {
            case ParseError::Scope::Object:
                out << "while reading object started here";
                break;

            case ParseError::Scope::Property:
                out << "while reading property " << scope.name << " started here";
                break;

            case ParseError::Scope::Duplicate:
                out << "existing property was defined here";
                break;

            case ParseError::Scope::Array:
                out << "while reading item " << scope.index
                    << " of the array started here (index is zero-based)";
                break;
        }
        out << '\n';
    }
}

void Parser::error(u32 file_ofs, HybridString&& message) {
    if (this->error_callback) {
        ParseError err{file_ofs, std::move(message), context};
        this->error_callback(err);
    }
    this->any_error_ = true;
}

void Parser::advance_char() {
    if (read_ofs + 1 < src_view.num_bytes) {
        read_ofs++;
        next_unit = src_view.bytes[read_ofs];
    } else {
        next_unit = -1;
    }
}

Parser::Token Parser::read_plain_token(Token::Type type) {
    Token result = {type, this->read_ofs, {}};
    advance_char();
    return result;
}

bool Parser::read_escaped_hex(OutStream& out, u32 escape_file_ofs) {
    PLY_ASSERT(0); // FIXME
    return false;
}

Parser::Token Parser::read_quoted_string() {
    PLY_ASSERT(next_unit == '"' || next_unit == '\'');
    Token token = {Token::Type::Text, this->read_ofs, {}};
    MemOutStream out;
    NativeEndianWriter wr{out};
    s32 end_byte = next_unit;
    u32 quote_run = 1;
    bool multiline = false;
    advance_char();

    for (;;) {
        if (next_unit == end_byte) {
            advance_char();
            if (quote_run == 0) {
                if (multiline) {
                    quote_run++;
                } else {
                    break; // end of string
                }
            } else {
                quote_run++;
                if (quote_run == 3) {
                    if (multiline) {
                        break; // end of string
                    } else {
                        multiline = true;
                        quote_run = 0;
                    }
                }
            }
        } else {
            if (quote_run > 0) {
                if (multiline) {
                    for (u32 i = 0; i < quote_run; i++) {
                        wr.write((u8) end_byte);
                    }
                } else if (quote_run == 2) {
                    break; // empty string
                }
                quote_run = 0;
            }

            switch (next_unit) {
                case -1: {
                    error(this->read_ofs, "Unexpected end of file in string literal");
                    return {};
                }

                case '\r':
                case '\n': {
                    if (multiline) {
                        if (next_unit == '\n') {
                            wr.write((u8) next_unit);
                        }
                        advance_char();
                    } else {
                        error(this->read_ofs,
                              "Unexpected end of line in string literal");
                        return {};
                    }
                    break;
                }

                case '\\': {
                    // Escape sequence
                    u32 escape_file_ofs = this->read_ofs;
                    advance_char();
                    s32 code = next_unit;
                    advance_char();
                    switch (code) {
                        case -1: {
                            error(this->read_ofs,
                                  "Unexpected end of file in string literal");
                            return {};
                        }

                        case '\r':
                        case '\n': {
                            error(this->read_ofs,
                                  "Unexpected end of line in string literal");
                            return {};
                        }

                        case '\\':
                        case '\'':
                        case '"': {
                            wr.write((u8) code);
                            break;
                        }

                        case 'r': {
                            wr.write((u8) '\r');
                            break;
                        }

                        case 'n': {
                            wr.write((u8) '\n');
                            break;
                        }

                        case 't': {
                            wr.write((u8) '\t');
                            break;
                        }

                        case 'x': {
                            if (!read_escaped_hex(out, escape_file_ofs))
                                return {}; // FIXME: Would be better to continue reading
                                           // the rest of the string
                            break;
                        }

                        default: {
                            error(
                                escape_file_ofs,
                                String::format("Unrecognized escape sequence \"\\{}\"",
                                               (char) code));
                            return {}; // FIXME: Would be better to continue reading the
                                       // rest of the string
                        }
                    }
                    break;
                }

                default: {
                    wr.write((u8) next_unit);
                    advance_char();
                    break;
                }
            }
        }
    }

    token.text = out.move_to_string();
    return token;
}

Parser::Token Parser::read_literal() {
    PLY_ASSERT(is_alnum_unit(next_unit));
    Token token = {Token::Text, this->read_ofs, {}};
    u32 start_ofs = read_ofs;

    while (is_alnum_unit(next_unit)) {
        advance_char();
    }

    token.text = StringView{(char*) src_view.bytes + start_ofs, read_ofs - start_ofs};
    return token;
}

Parser::Token Parser::read_token(bool tokenize_new_line) {
    if (push_back_token.is_valid()) {
        Token token = std::move(push_back_token);
        push_back_token = {};
        return token;
    }

    for (;;) {
        switch (next_unit) {
            case ' ':
            case '\t':
            case '\r':
                advance_char();
                break;

            case '\n': {
                u32 new_line_ofs = this->read_ofs;
                advance_char();
                if (tokenize_new_line)
                    return {Token::NewLine, new_line_ofs, {}};
                break;
            }

            case -1:
                return {Token::EndOfFile, this->read_ofs, {}};
            case '{':
                return read_plain_token(Token::OpenCurly);
            case '}':
                return read_plain_token(Token::CloseCurly);
            case '[':
                return read_plain_token(Token::OpenSquare);
            case ']':
                return read_plain_token(Token::CloseSquare);
            case ':':
                return read_plain_token(Token::Colon);
            case '=':
                return read_plain_token(Token::Equals);
            case ',':
                return read_plain_token(Token::Comma);
            case ';':
                return read_plain_token(Token::Semicolon);

            case '"':
            case '\'':
                return read_quoted_string();

            default:
                if (is_alnum_unit(next_unit))
                    return read_literal();
                else
                    return {Token::Junk, this->read_ofs, {}};
        }
    }
}

HybridString Parser::to_string(const Token& token) {
    switch (token.type) {
        case Token::OpenCurly:
            return "\"{\"";
        case Token::CloseCurly:
            return "\"}\"";
        case Token::OpenSquare:
            return "\"[\"";
        case Token::CloseSquare:
            return "\"]\"";
        case Token::Colon:
            return "\":\"";
        case Token::Equals:
            return "\"=\"";
        case Token::Comma:
            return "\",\"";
        case Token::Semicolon:
            return "\";\"";
        case Token::Text:
            return String::format("text \"{}\"", escape(token.text));
        case Token::Junk:
            return String::format("junk \"{}\"", escape{token.text});
        case Token::NewLine:
            return "newline";
        case Token::EndOfFile:
            return "end of file";
        default:
            PLY_ASSERT(0);
            return "???";
    }
}

HybridString Parser::to_string(const Node* node) {
    switch ((Node::Type) node->type) {
        case Node::Type::Object:
            return "object";
        case Node::Type::Array:
            return "array";
        case Node::Type::Text:
            return String::format("text \"{}\"", escape(node->text()));
        default:
            PLY_ASSERT(0);
            return "???";
    }
}

Owned<Node> Parser::read_object(const Token& start_token) {
    PLY_ASSERT(start_token.type == Token::OpenCurly);
    ScopeHandler object_scope{*this, ParseError::Scope::object(start_token.file_ofs)};
    Owned<Node> node = Node::create_object(start_token.file_ofs);
    Map<String, u32> prop_locations;
    Token prev_property = {};
    for (;;) {
        bool got_separator = false;
        Token first_token = {};
        for (;;) {
            first_token = read_token(true);
            switch (first_token.type) {
                case Token::CloseCurly:
                    return node;

                case Token::Comma:
                case Token::Semicolon:
                case Token::NewLine:
                    got_separator = true;
                    break;

                default:
                    goto break_outer;
            }
        }
    break_outer:

        if (first_token.type == Token::Text) {
            if (prev_property.is_valid() && !got_separator) {
                error(first_token.file_ofs,
                      String::format("Expected a comma, semicolon or newline "
                                     "separator between properties \"{}\" and \"{}\"",
                                     escape(prev_property.text),
                                     escape(first_token.text)));
                return {};
            }
        } else if (prev_property.is_valid()) {
            error(first_token.file_ofs,
                  String::format("Unexpected {} after property \"{}\"",
                                 to_string(first_token), escape(prev_property.text)));
            return {};
        } else {
            error(first_token.file_ofs,
                  String::format("Expected property, got {}", to_string(first_token)));
            return {};
        }

        bool was_found = false;
        u32* file_ofs = prop_locations.insert_or_find(first_token.text, &was_found);
        if (was_found) {
            ScopeHandler duplicate_scope{*this,
                                         ParseError::Scope::duplicate(*file_ofs)};
            error(first_token.file_ofs, String::format("Duplicate property \"{}\"",
                                                       escape(first_token.text)));
            return {};
        }

        Token colon = read_token();
        if (colon.type != Token::Colon && colon.type != Token::Equals) {
            error(colon.file_ofs,
                  String::format("Expected \":\" or \"=\" after \"{}\", got {}",
                                 escape(first_token.text), to_string(colon)));
            return {};
        }

        {
            // Read value of property
            ScopeHandler property_scope{
                *this,
                ParseError::Scope::property(first_token.file_ofs, first_token.text)};
            Owned<Node> value = read_expression(read_token(), &colon);
            if (!value->is_valid())
                return value;
            *file_ofs = first_token.file_ofs;
            node->set(std::move(first_token.text), std::move(value));
        }

        prev_property = std::move(first_token);
    }
    return {};
}

Owned<Node> Parser::read_array(const Token& start_token) {
    PLY_ASSERT(start_token.type == Token::OpenSquare);
    ScopeHandler array_scope{*this, ParseError::Scope::array(start_token.file_ofs, 0)};
    Owned<Node> array_node = Node::create_array(start_token.file_ofs);
    Token sep_token_holder;
    Token* sep_token = nullptr;
    for (;;) {
        Token token = read_token(true);
        switch (token.type) {
            case Token::CloseSquare:
                return array_node;

            case Token::Comma:
            case Token::Semicolon:
            case Token::NewLine:
                sep_token_holder = std::move(token);
                sep_token = &sep_token_holder;
                break;

            default: {
                Owned<Node> value = read_expression(std::move(token), sep_token);
                if (!value->is_valid())
                    return value;
                array_node->array().append(std::move(value));
                array_scope.get().index++;
                sep_token = nullptr;
                break;
            }
        }
    }
}

Owned<Node> Parser::read_expression(Token&& first_token, const Token* after_token) {
    switch (first_token.type) {
        case Token::OpenCurly:
            return read_object(first_token);

        case Token::OpenSquare:
            return read_array(first_token);

        case Token::Text:
            return Node::create_text(std::move(first_token.text), first_token.file_ofs);

        case Token::Invalid:
            return {};

        default: {
            MemOutStream mout;
            mout << "Unexpected " << to_string(first_token);
            if (after_token) {
                mout << " after " << to_string(*after_token);
            }
            error(first_token.file_ofs, mout.move_to_string());
            return {};
        }
    }
}

Parser::Result Parser::parse(StringView path, StringView src_view_) {
    src_view = src_view_;
    next_unit = src_view.num_bytes > 0 ? src_view[0] : -1;

    this->file_loc_map = FileLocationMap::from_view(path, src_view_);

    Token root_token = read_token();
    Owned<Node> root = read_expression(std::move(root_token));
    if (!root->is_valid())
        return {};

    Token next_token = read_token();
    if (next_token.type != Token::EndOfFile) {
        error(next_token.file_ofs,
              String::format("Unexpected {} after {}", to_string(next_token),
                             to_string(root)));
        return {};
    }

    return {std::move(root), std::move(this->file_loc_map)};
}

} // namespace pylon
