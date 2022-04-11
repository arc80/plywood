/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-crowbar/Core.h>
#include <ply-crowbar/Parser.h>

namespace ply {
namespace crowbar {

bool okToStayInScope(Parser* parser, const ExpandedToken& token) {
    switch (token.type) {
        case TokenType::CloseCurly: {
            if (parser->recovery.outerAcceptFlags & Parser::RecoveryState::AcceptCloseCurly) {
                parser->tkr->rewindTo(token.tokenIdx);
                return false;
            }
            break;
        }
        case TokenType::CloseParen: {
            if (parser->recovery.outerAcceptFlags & Parser::RecoveryState::AcceptCloseParen) {
                parser->tkr->rewindTo(token.tokenIdx);
                return false;
            }
            break;
        }
        case TokenType::CloseSquare: {
            if (parser->recovery.outerAcceptFlags & Parser::RecoveryState::AcceptCloseSquare) {
                parser->tkr->rewindTo(token.tokenIdx);
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

PLY_NO_INLINE u32 tokenTypeToAcceptCloseFlag(TokenType tokenType) {
    switch (tokenType) {
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

bool skipAnyScope(Parser* parser, ExpandedToken* outCloseToken, const ExpandedToken& openToken) {
    PLY_SET_IN_SCOPE(parser->recovery.outerAcceptFlags,
                     parser->recovery.outerAcceptFlags |
                         tokenTypeToAcceptCloseFlag(openToken.type));
    TokenType closePunc = (TokenType)((u32) openToken.type + 1);
    for (;;) {
        ExpandedToken token = parser->tkr->readToken();
        if (token.type == closePunc) {
            if (outCloseToken) {
                *outCloseToken = token;
            }
            return true;
        }

        if (!okToStayInScope(parser, token)) {
            parser->recovery.muteErrors = false;
            return false;
        }

        switch (token.type) {
            case TokenType::OpenCurly:
            case TokenType::OpenParen:
            case TokenType::OpenSquare: {
                skipAnyScope(parser, nullptr, token);
                break;
            }
            default: {
            }
        }
    }
}

// This function handles unexpected tokens. Most of the time, it just consumes the token and returns
// true. However, if the unexpected token opens a new scope, such as {, ( or [, it tries to skip the
// entire nested scope and returns true if successful. If the unexpected token ends an outer scope,
// or if a token that ends an outer scope is encountered while trying to skip a nested scope, it
// pushes the token back and returns false. That way, the outer scope can be terminated by the
// caller.
PLY_NO_INLINE bool handleUnexpectedToken(Parser* parser, ExpandedToken* outCloseToken,
                                         const ExpandedToken& unexpected) {
    if (!okToStayInScope(parser, unexpected))
        return false;

    switch (unexpected.type) {
        case TokenType::OpenCurly:
        case TokenType::OpenParen:
        case TokenType::OpenSquare: {
            skipAnyScope(parser, outCloseToken, unexpected);
            // Ignore the return value of skipAnyScope. If it's false, that means some token
            // canceled the inner scope and was pushed back. We want the caller to read that token
            // next.
            return true;
        }
        // FIXME: Log errors for unmatched closing brackets
        default: {
            return true;
        }
    }
}

} // namespace crowbar
} // namespace ply
