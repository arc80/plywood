/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <pylon/Core.h>
#include <pylon/Node.h>
#include <ply-runtime/io/text/FileLocationMap.h>

namespace pylon {

struct ParseError {
    struct Scope {
        enum Type { Object, Property, Duplicate, Array };
        u32 file_ofs;
        Type type;
        StringView name;
        u32 index;

        static Scope object(u32 file_ofs) {
            return {file_ofs, Object, {}, 0};
        }
        static Scope property(u32 file_ofs, StringView name) {
            return {file_ofs, Property, name, 0};
        }
        static Scope duplicate(u32 file_ofs) {
            return {file_ofs, Duplicate, {}, 0};
        }
        static Scope array(u32 file_ofs, u32 index) {
            return {file_ofs, Array, {}, index};
        }
    };

    u32 file_ofs;
    HybridString message;
    const Array<Scope>& context;
};

class Parser {
private:
    struct Token {
        enum Type {
            Invalid,
            OpenCurly,
            CloseCurly,
            OpenSquare,
            CloseSquare,
            Colon,
            Equals,
            Comma,
            Semicolon,
            Text,
            Junk,
            NewLine,
            EndOfFile
        };
        Type type = Invalid;
        u32 file_ofs = 0;
        HybridString text;

        bool is_valid() const {
            return type != Type::Invalid;
        }
    };

    Func<void(const ParseError& err)> error_callback;
    FileLocationMap file_loc_map;
    bool any_error_ = false;
    StringView src_view;
    u32 read_ofs = 0;
    s32 next_unit = 0;
    u32 tab_size = 4;
    Token push_back_token;
    Array<ParseError::Scope> context;

    void push_back(Token&& token) {
        push_back_token = std::move(token);
    }

    struct ScopeHandler {
        Parser& parser;
        u32 index;

        ScopeHandler(Parser& parser, ParseError::Scope&& scope)
            : parser{parser}, index{parser.context.num_items()} {
            parser.context.append(std::move(scope));
        }
        ~ScopeHandler() {
            // parser.context can be empty when ParseError is thrown
            if (!parser.context.is_empty()) {
                PLY_ASSERT(parser.context.num_items() == index + 1);
                parser.context.pop();
            }
        }
        ParseError::Scope& get() {
            return parser.context[index];
        }
    };

    void error(u32 file_ofs, HybridString&& message);
    void advance_char();
    Token read_plain_token(Token::Type type);
    bool read_escaped_hex(OutStream& out, u32 escape_file_ofs);
    Token read_quoted_string();
    Token read_literal();
    Token read_token(bool tokenize_new_line = false);
    static HybridString to_string(const Token& token);
    static HybridString to_string(const Node* node);
    Owned<Node> read_object(const Token& start_token);
    Owned<Node> read_array(const Token& start_token);
    Owned<Node> read_expression(Token&& first_token,
                                const Token* after_token = nullptr);

public:
    void set_tab_size(int tab_size_) {
        tab_size = tab_size_;
    }
    void set_error_callback(Func<void(const ParseError& err)>&& cb) {
        this->error_callback = std::move(cb);
    }
    bool any_error() const {
        return this->any_error_;
    }

    struct Result {
        Owned<Node> root;
        FileLocationMap file_loc_map;
    };

    void dump_error(const ParseError& error, OutStream& out) const;

    Result parse(StringView path, StringView src_view_);
};

} // namespace pylon
