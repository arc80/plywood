/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-cpp/Core.h>
#include <ply-cpp/Parser.h>

namespace ply {
namespace cpp {

struct SpecDcorMode {
    enum Mode {
        GlobalOrMember, // If it's a member, enclosing_class_name will be non-empty
        Param,
        TemplateParam,
        TypeID,
        ConversionTypeID,
    };
    Mode mode = GlobalOrMember;
    StringView enclosing_class_name;

    SpecDcorMode(Mode mode, StringView enclosing_class_name = {})
        : mode{mode}, enclosing_class_name{enclosing_class_name} {
    }
    bool is_any_param() const {
        return mode == Param || mode == TemplateParam;
    }
    bool is_any_type_id() const {
        return mode == TypeID || mode == ConversionTypeID;
    }
};

Token read_token(Parser* parser);
// FIXME: Change skip_any_scope to return out_close_token (or an invalid token) instead
bool skip_any_scope(Parser* parser, Token* out_close_token, const Token& open_token);
void push_back_token(Parser* parser, const Token& token);
// Returns false if token was pushed back and caller must return to outer scope:
bool handle_unexpected_token(Parser* parser, Token* out_close_token,
                             const Token& token);
bool ok_to_stay_in_scope(Parser* parser, const Token& token);

struct ParseActivity {
    Parser* parser = nullptr;
    u32 saved_error_count = 0;
    LinearLocation saved_token_loc = 0;

    ParseActivity(Parser* parser) : parser{parser} {
        this->saved_error_count = parser->raw_error_count;
        Token token = read_token(parser);
        this->saved_token_loc = token.linear_loc;
        push_back_token(parser, token);
    }

    bool error_occurred() const {
        return this->parser->raw_error_count != this->saved_error_count;
    }

    bool any_tokens_consumed() const {
        if (this->parser->token_queue.is_empty())
            return false;
        return this->parser->token_queue[0].linear_loc == this->saved_token_loc;
    }
};

// FIXME: Move somewhere else. It's an internal struct used by various parse functions.
struct SetAcceptFlagsInScope {
    Parser* parser;
    u32 prev_accept_flags = 0;
    bool prev_tokenize_close_angles = false;

    SetAcceptFlagsInScope(Parser* parser, Token::Type open_token_type);
    ~SetAcceptFlagsInScope();
};

} // namespace cpp
} // namespace ply
