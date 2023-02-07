/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-biscuit/Core.h>
#include <ply-biscuit/Parser.h>

namespace ply {
namespace biscuit {

bool ok_to_stay_in_scope(Parser* parser, const ExpandedToken& token) {
    switch (token.type) {
        case TokenType::CloseCurly: {
            if (parser->recovery.outer_accept_flags &
                Parser::RecoveryState::AcceptCloseCurly) {
                parser->tkr->rewind_to(token.token_idx);
                return false;
            }
            break;
        }
        case TokenType::CloseParen: {
            if (parser->recovery.outer_accept_flags &
                Parser::RecoveryState::AcceptCloseParen) {
                parser->tkr->rewind_to(token.token_idx);
                return false;
            }
            break;
        }
        case TokenType::CloseSquare: {
            if (parser->recovery.outer_accept_flags &
                Parser::RecoveryState::AcceptCloseSquare) {
                parser->tkr->rewind_to(token.token_idx);
                return false;
            }
            break;
        }
        case TokenType::EndOfFile:
            return false;
        default:
            break;
    }
    return true;
}

u32 token_type_to_accept_close_flag(TokenType token_type) {
    switch (token_type) {
        case TokenType::OpenCurly:
            return Parser::RecoveryState::AcceptCloseCurly;
        case TokenType::OpenParen:
            return Parser::RecoveryState::AcceptCloseParen;
        case TokenType::OpenSquare:
            return Parser::RecoveryState::AcceptCloseSquare;
        default:
            PLY_ASSERT(0);
            break;
    }
    return 0;
}

bool skip_any_scope(Parser* parser, ExpandedToken* out_close_token,
                    TokenType open_token_type) {
    PLY_SET_IN_SCOPE(parser->recovery.outer_accept_flags,
                     parser->recovery.outer_accept_flags |
                         token_type_to_accept_close_flag(open_token_type));
    TokenType close_punc = (TokenType) ((u32) open_token_type + 1);
    for (;;) {
        ExpandedToken token = parser->tkr->read_token();
        if (token.type == close_punc) {
            if (out_close_token) {
                *out_close_token = token;
            }
            return true;
        }

        if (!ok_to_stay_in_scope(parser, token)) {
            parser->recovery.mute_errors = false;
            return false;
        }

        switch (token.type) {
            case TokenType::OpenCurly:
            case TokenType::OpenParen:
            case TokenType::OpenSquare: {
                skip_any_scope(parser, nullptr, token.type);
                break;
            }
            default: {
            }
        }
    }
}

// This function handles unexpected tokens. Most of the time, it just consumes the token
// and returns true. However, if the unexpected token opens a new scope, such as {, ( or
// [, it tries to skip the entire nested scope and returns true if successful. If the
// unexpected token ends an outer scope, or if a token that ends an outer scope is
// encountered while trying to skip a nested scope, it pushes the token back and returns
// false. That way, the outer scope can be terminated by the caller.
bool handle_unexpected_token(Parser* parser, ExpandedToken* out_close_token,
                             const ExpandedToken& unexpected) {
    if (!ok_to_stay_in_scope(parser, unexpected))
        return false;

    switch (unexpected.type) {
        case TokenType::OpenCurly:
        case TokenType::OpenParen:
        case TokenType::OpenSquare: {
            skip_any_scope(parser, out_close_token, unexpected.type);
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

} // namespace biscuit
} // namespace ply
