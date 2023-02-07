/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-cpp/Core.h>
#include <ply-cpp/ParseDeclarations.h>
#include <ply-cpp/Preprocessor.h>

namespace ply {
namespace cpp {

void ParseSupervisor::do_exit(AnyObject node) {
    if (auto* tmpl = node.safe_cast<grammar::Declaration::Template_>()) {
        PLY_ASSERT(tmpl->visor_decl); // Should have gotten declaration
        PLY_UNUSED(tmpl);
    }
    const AnyObject& scope = scope_stack.back();
    PLY_ASSERT(scope == node);
    PLY_UNUSED(scope);
    this->exit(node);
    this->scope_stack.pop();
}

void ParseSupervisor::got_declaration(grammar::Declaration&& decl) {
    this->on_got_declaration(decl);
    const AnyObject& scope = scope_stack.back();
    if (auto* ns = scope.safe_cast<grammar::Declaration::Namespace_>()) {
        ns->visor_decls.append(std::move(decl));
    } else if (auto* tmpl = scope.safe_cast<grammar::Declaration::Template_>()) {
        PLY_ASSERT(!tmpl->visor_decl);
        tmpl->visor_decl = new grammar::Declaration{std::move(decl)};
    } else if (auto* record = scope.safe_cast<grammar::DeclSpecifier::Record>()) {
        record->visor_decls.append(std::move(decl));
    } else if (auto* tu = scope.safe_cast<grammar::TranslationUnit>()) {
        tu->visor_decls.append(std::move(decl));
    } else if (auto* linkage = scope.safe_cast<grammar::Declaration::Linkage>()) {
        linkage->visor_decls.append(std::move(decl));
    } else {
        PLY_ASSERT(0);
    }
}

String ParseSupervisor::get_class_name(StringView with_sep,
                                       bool with_name_space) const {
    MemOutStream mout;
    PLY_ASSERT(this->scope_stack[0].is<grammar::TranslationUnit>());
    StringView sep = "";
    for (u32 i = 1; i < scope_stack.num_items(); i++) {
        const AnyObject& scope = scope_stack[i];
        if (auto ns = scope.safe_cast<grammar::Declaration::Namespace_>()) {
            if (with_name_space) {
                mout << sep << ns->qid.get_class_name();
                sep = with_sep;
            }
        } else if (auto record = scope.safe_cast<grammar::DeclSpecifier::Record>()) {
            mout << sep << record->qid.get_class_name();
            sep = with_sep;
        } else if (auto enum_ = scope.safe_cast<grammar::DeclSpecifier::Enum_>()) {
            mout << sep << enum_->qid.get_class_name();
            sep = with_sep;
        }
    }
    return mout.move_to_string();
}

String ParseSupervisor::get_namespace_prefix() const {
    MemOutStream mout;
    PLY_ASSERT(this->scope_stack[0].is<grammar::TranslationUnit>());
    for (u32 i = 1; i < this->scope_stack.num_items(); i++) {
        const AnyObject& scope = scope_stack[i];
        auto ns = scope.safe_cast<grammar::Declaration::Namespace_>();
        if (!ns)
            break;
        mout << ns->qid.get_class_name() << "::";
    }
    return mout.move_to_string();
}

Token read_token_internal(Parser* parser) {
    if (parser->token_queue_pos < parser->token_queue.num_items()) {
        Token r = parser->token_queue[parser->token_queue_pos];
        if (!parser->restore_point_enabled) {
            PLY_ASSERT(parser->token_queue_pos == 0);
            parser->token_queue.erase(0);
        } else {
            parser->token_queue_pos++;
        }
        return r;
    } else {
        if (parser->restore_point_enabled) {
            parser->token_queue.append(read_token(parser->pp));
            parser->token_queue_pos++;
            return parser->token_queue.back();
        } else {
            return read_token(parser->pp);
        }
    }
}

Token read_token(Parser* parser) {
    Token token;
    for (;;) {
        token = read_token_internal(parser);
        switch (token.type) {
            case Token::CStyleComment:
            case Token::LineComment:
            case Token::Directive:
            case Token::Macro: {
                parser->visor->got_macro_or_comment(token);
                break;
            }

            default: {
                return token;
            }
        }
    }
}

void push_back_token(Parser* parser, const Token& token) {
    if (parser->token_queue_pos > 0) {
        PLY_ASSERT(parser->token_queue[parser->token_queue_pos - 1] == token);
        parser->token_queue_pos--;
    } else {
        parser->token_queue.insert(parser->token_queue_pos) = token;
    }
}

bool ok_to_stay_in_scope(Parser* parser, const Token& token) {
    switch (token.type) {
        case Token::OpenCurly: {
            if (parser->outer_accept_flags & Parser::AcceptOpenCurly) {
                push_back_token(parser, token);
                return false;
            }
            break;
        }
        case Token::CloseCurly: {
            if (parser->outer_accept_flags & Parser::AcceptCloseCurly) {
                push_back_token(parser, token);
                return false;
            }
            break;
        }
        case Token::CloseParen: {
            if (parser->outer_accept_flags & Parser::AcceptCloseParen) {
                push_back_token(parser, token);
                return false;
            }
            break;
        }
        case Token::CloseAngle: {
            if (parser->outer_accept_flags & Parser::AcceptCloseAngle) {
                push_back_token(parser, token);
                return false;
            }
            break;
        }
        case Token::CloseSquare: {
            if (parser->outer_accept_flags & Parser::AcceptCloseSquare) {
                push_back_token(parser, token);
                return false;
            }
            break;
        }
        case Token::EndOfFile:
            return false;
        default:
            break;
    }
    return true;
}

//-------------------------------------------------------------------------------------
// skip_any_scope
//
// Returns false if an unexpected token is encountered and an outer scope is expected
// to handle it, as determined by parser->outer_accept_flags.
//-------------------------------------------------------------------------------------
bool skip_any_scope(Parser* parser, Token* out_close_token, const Token& open_token) {
    SetAcceptFlagsInScope accept_scope{parser, open_token.type};
    Token::Type close_punc = (Token::Type)((u32) open_token.type + 1);
    for (;;) {
        Token token = read_token(parser);
        if (token.type == close_punc) {
            if (out_close_token) {
                *out_close_token = token;
            }
            return true;
        }

        if (!ok_to_stay_in_scope(parser, token)) {
            parser->error(true, {ParseError::UnclosedToken, token, {}, open_token});
            return false;
        }

        switch (token.type) {
            case Token::OpenAngle: {
                if (parser->pp->tokenize_close_angles_only) {
                    // If we were immediately inside a template-parameter/argument scope
                    // < >, treat < as a nested scope, because we now need to encounter
                    // two CloseAngle tokens:
                    skip_any_scope(parser, nullptr, token);
                }
                // If we are not immediately inside a template-parameter/argument scope
                // < >, don't treat < as the beginning of a scope, since it might just
                // be a less-than operator.
                break;
            }
            case Token::OpenCurly:
            case Token::OpenParen:
            case Token::OpenSquare: {
                skip_any_scope(parser, nullptr, token);
                break;
            }
            default: {
            }
        }
    }
}

// Returns false if the given token was pushed back and ends an outer scope. Otherwise,
// it consumes the given token. If the given token begins a new scope, it consumes
// tokens until either the inner scope is closed, or until the inner scope is "canceled"
// by a closing token that closes an outer scope, as determined by
// parser->outer_accept_flags. In that case, the closing token is pushed back so that
// the caller can read it next. In each of those cases, it returns true to indicate to
// the caller that the given token was consumed and a new token is available to read.
bool handle_unexpected_token(Parser* parser, Token* out_close_token,
                             const Token& token) {
    // FIXME: Merge this with the second half of skip_any_scope:
    if (!ok_to_stay_in_scope(parser, token))
        return false;

    switch (token.type) {
        case Token::OpenAngle: {
            if (parser->pp->tokenize_close_angles_only) {
                // If we were immediately inside a template-parameter/argument scope <
                // >, treat < as a nested scope, because we now need to encounter two
                // CloseAngle tokens:
                skip_any_scope(parser, out_close_token, token);
                // Ignore the return value of skip_any_scope. If it's false, that means
                // some token canceled the inner scope and was pushed back. We want the
                // caller to read that token next.
            }
            // If we are not immediately inside a template-parameter/argument scope < >,
            // don't treat < as the beginning of a scope, since it might just be a
            // less-than operator.
            return true;
        }
        case Token::OpenCurly:
        case Token::OpenParen:
        case Token::OpenSquare: {
            skip_any_scope(parser, out_close_token, token);
            // Ignore the return value of skip_any_scope. If it's false, that means some
            // token canceled the inner scope and was pushed back. We want the caller to
            // read that token next.
            return true;
        }
        // FIXME: Log errors for unmatched closing brackets
        default: {
            return true;
        }
    }
}

SetAcceptFlagsInScope::SetAcceptFlagsInScope(Parser* parser,
                                             Token::Type open_token_type)
    : parser{parser} {
    this->prev_accept_flags = parser->outer_accept_flags;
    this->prev_tokenize_close_angles = parser->pp->tokenize_close_angles_only;

    switch (open_token_type) {
        case Token::OpenCurly: {
            parser->outer_accept_flags = Parser::AcceptCloseCurly;
            parser->pp->tokenize_close_angles_only = false;
            break;
        }
        case Token::OpenParen: {
            parser->outer_accept_flags =
                (parser->outer_accept_flags | Parser::AcceptCloseParen) &
                ~Parser::AcceptCloseAngle;
            parser->pp->tokenize_close_angles_only = false;
            break;
        }
        case Token::OpenAngle: {
            parser->outer_accept_flags =
                (parser->outer_accept_flags | Parser::AcceptCloseAngle);
            parser->pp->tokenize_close_angles_only = true;
            break;
        }
        case Token::OpenSquare: {
            parser->outer_accept_flags =
                (parser->outer_accept_flags | Parser::AcceptCloseSquare) &
                ~Parser::AcceptCloseAngle;
            parser->pp->tokenize_close_angles_only = false;
            break;
        }
        default: {
            PLY_ASSERT(0); // Illegal
            break;
        }
    }
}

SetAcceptFlagsInScope::~SetAcceptFlagsInScope() {
    parser->outer_accept_flags = this->prev_accept_flags;
    parser->pp->tokenize_close_angles_only = this->prev_tokenize_close_angles;
}

} // namespace cpp
} // namespace ply
