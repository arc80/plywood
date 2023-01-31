/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-cpp/Core.h>
#include <ply-cpp/ParseDeclarations.h>
#include <ply-cpp/Preprocessor.h>

namespace ply {
namespace cpp {

void ParseSupervisor::doExit(AnyObject node) {
    if (auto* tmpl = node.safeCast<grammar::Declaration::Template_>()) {
        PLY_ASSERT(tmpl->visor_decl); // Should have gotten declaration
        PLY_UNUSED(tmpl);
    }
    const AnyObject& scope = scopeStack.back();
    PLY_ASSERT(scope == node);
    PLY_UNUSED(scope);
    this->exit(node);
    this->scopeStack.pop();
}

void ParseSupervisor::gotDeclaration(grammar::Declaration&& decl) {
    this->onGotDeclaration(decl);
    const AnyObject& scope = scopeStack.back();
    if (auto* ns = scope.safeCast<grammar::Declaration::Namespace_>()) {
        ns->visor_decls.append(std::move(decl));
    } else if (auto* tmpl = scope.safeCast<grammar::Declaration::Template_>()) {
        PLY_ASSERT(!tmpl->visor_decl);
        tmpl->visor_decl = new grammar::Declaration{std::move(decl)};
    } else if (auto* record = scope.safeCast<grammar::DeclSpecifier::Record>()) {
        record->visor_decls.append(std::move(decl));
    } else if (auto* tu = scope.safeCast<grammar::TranslationUnit>()) {
        tu->visor_decls.append(std::move(decl));
    } else if (auto* linkage = scope.safeCast<grammar::Declaration::Linkage>()) {
        linkage->visor_decls.append(std::move(decl));
    } else {
        PLY_ASSERT(0);
    }
}

String ParseSupervisor::getClassName(StringView withSep, bool withNameSpace) const {
    MemOutStream mout;
    PLY_ASSERT(this->scopeStack[0].is<grammar::TranslationUnit>());
    StringView sep = "";
    for (u32 i = 1; i < scopeStack.numItems(); i++) {
        const AnyObject& scope = scopeStack[i];
        if (auto ns = scope.safeCast<grammar::Declaration::Namespace_>()) {
            if (withNameSpace) {
                mout << sep << ns->qid.getClassName();
                sep = withSep;
            }
        } else if (auto record = scope.safeCast<grammar::DeclSpecifier::Record>()) {
            mout << sep << record->qid.getClassName();
            sep = withSep;
        } else if (auto enum_ = scope.safeCast<grammar::DeclSpecifier::Enum_>()) {
            mout << sep << enum_->qid.getClassName();
            sep = withSep;
        }
    }
    return mout.moveToString();
}

String ParseSupervisor::getNamespacePrefix() const {
    MemOutStream mout;
    PLY_ASSERT(this->scopeStack[0].is<grammar::TranslationUnit>());
    for (u32 i = 1; i < this->scopeStack.numItems(); i++) {
        const AnyObject& scope = scopeStack[i];
        auto ns = scope.safeCast<grammar::Declaration::Namespace_>();
        if (!ns)
            break;
        mout << ns->qid.getClassName() << "::";
    }
    return mout.moveToString();
}

PLY_NO_INLINE Token readTokenInternal(Parser* parser) {
    if (parser->tokenQueuePos < parser->tokenQueue.numItems()) {
        Token r = parser->tokenQueue[parser->tokenQueuePos];
        if (!parser->restorePointEnabled) {
            PLY_ASSERT(parser->tokenQueuePos == 0);
            parser->tokenQueue.erase(0);
        } else {
            parser->tokenQueuePos++;
        }
        return r;
    } else {
        if (parser->restorePointEnabled) {
            parser->tokenQueue.append(readToken(parser->pp));
            parser->tokenQueuePos++;
            return parser->tokenQueue.back();
        } else {
            return readToken(parser->pp);
        }
    }
}

Token readToken(Parser* parser) {
    Token token;
    for (;;) {
        token = readTokenInternal(parser);
        switch (token.type) {
            case Token::CStyleComment:
            case Token::LineComment:
            case Token::Directive:
            case Token::Macro: {
                parser->visor->gotMacroOrComment(token);
                break;
            }

            default: {
                return token;
            }
        }
    }
}

PLY_NO_INLINE void pushBackToken(Parser* parser, const Token& token) {
    if (parser->tokenQueuePos > 0) {
        PLY_ASSERT(parser->tokenQueue[parser->tokenQueuePos - 1] == token);
        parser->tokenQueuePos--;
    } else {
        parser->tokenQueue.insert(parser->tokenQueuePos) = token;
    }
}

bool okToStayInScope(Parser* parser, const Token& token) {
    switch (token.type) {
        case Token::OpenCurly: {
            if (parser->outerAcceptFlags & Parser::AcceptOpenCurly) {
                pushBackToken(parser, token);
                return false;
            }
            break;
        }
        case Token::CloseCurly: {
            if (parser->outerAcceptFlags & Parser::AcceptCloseCurly) {
                pushBackToken(parser, token);
                return false;
            }
            break;
        }
        case Token::CloseParen: {
            if (parser->outerAcceptFlags & Parser::AcceptCloseParen) {
                pushBackToken(parser, token);
                return false;
            }
            break;
        }
        case Token::CloseAngle: {
            if (parser->outerAcceptFlags & Parser::AcceptCloseAngle) {
                pushBackToken(parser, token);
                return false;
            }
            break;
        }
        case Token::CloseSquare: {
            if (parser->outerAcceptFlags & Parser::AcceptCloseSquare) {
                pushBackToken(parser, token);
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
// skipAnyScope
//
// Returns false if an unexpected token is encountered and an outer scope is expected
// to handle it, as determined by parser->outerAcceptFlags.
//-------------------------------------------------------------------------------------
bool skipAnyScope(Parser* parser, Token* outCloseToken, const Token& openToken) {
    SetAcceptFlagsInScope acceptScope{parser, openToken.type};
    Token::Type closePunc = (Token::Type)((u32) openToken.type + 1);
    for (;;) {
        Token token = readToken(parser);
        if (token.type == closePunc) {
            if (outCloseToken) {
                *outCloseToken = token;
            }
            return true;
        }

        if (!okToStayInScope(parser, token)) {
            parser->error(true, {ParseError::UnclosedToken, token, {}, openToken});
            return false;
        }

        switch (token.type) {
            case Token::OpenAngle: {
                if (parser->pp->tokenizeCloseAnglesOnly) {
                    // If we were immediately inside a template-parameter/argument scope < >, treat
                    // < as a nested scope, because we now need to encounter two CloseAngle tokens:
                    skipAnyScope(parser, nullptr, token);
                }
                // If we are not immediately inside a template-parameter/argument scope < >, don't
                // treat < as the beginning of a scope, since it might just be a less-than operator.
                break;
            }
            case Token::OpenCurly:
            case Token::OpenParen:
            case Token::OpenSquare: {
                skipAnyScope(parser, nullptr, token);
                break;
            }
            default: {
            }
        }
    }
}

// Returns false if the given token was pushed back and ends an outer scope. Otherwise, it consumes
// the given token. If the given token begins a new scope, it consumes tokens until either the inner
// scope is closed, or until the inner scope is "canceled" by a closing token that closes an outer
// scope, as determined by parser->outerAcceptFlags. In that case, the closing token is pushed back
// so that the caller can read it next. In each of those cases, it returns true to indicate to the
// caller that the given token was consumed and a new token is available to read.
PLY_NO_INLINE bool handleUnexpectedToken(Parser* parser, Token* outCloseToken, const Token& token) {
    // FIXME: Merge this with the second half of skipAnyScope:
    if (!okToStayInScope(parser, token))
        return false;

    switch (token.type) {
        case Token::OpenAngle: {
            if (parser->pp->tokenizeCloseAnglesOnly) {
                // If we were immediately inside a template-parameter/argument scope < >, treat
                // < as a nested scope, because we now need to encounter two CloseAngle tokens:
                skipAnyScope(parser, outCloseToken, token);
                // Ignore the return value of skipAnyScope. If it's false, that means some token
                // canceled the inner scope and was pushed back. We want the caller to read that
                // token next.
            }
            // If we are not immediately inside a template-parameter/argument scope < >, don't
            // treat < as the beginning of a scope, since it might just be a less-than operator.
            return true;
        }
        case Token::OpenCurly:
        case Token::OpenParen:
        case Token::OpenSquare: {
            skipAnyScope(parser, outCloseToken, token);
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

PLY_NO_INLINE SetAcceptFlagsInScope::SetAcceptFlagsInScope(Parser* parser,
                                                           Token::Type openTokenType)
    : parser{parser} {
    this->prevAcceptFlags = parser->outerAcceptFlags;
    this->prevTokenizeCloseAngles = parser->pp->tokenizeCloseAnglesOnly;

    switch (openTokenType) {
        case Token::OpenCurly: {
            parser->outerAcceptFlags = Parser::AcceptCloseCurly;
            parser->pp->tokenizeCloseAnglesOnly = false;
            break;
        }
        case Token::OpenParen: {
            parser->outerAcceptFlags =
                (parser->outerAcceptFlags | Parser::AcceptCloseParen) & ~Parser::AcceptCloseAngle;
            parser->pp->tokenizeCloseAnglesOnly = false;
            break;
        }
        case Token::OpenAngle: {
            parser->outerAcceptFlags = (parser->outerAcceptFlags | Parser::AcceptCloseAngle);
            parser->pp->tokenizeCloseAnglesOnly = true;
            break;
        }
        case Token::OpenSquare: {
            parser->outerAcceptFlags =
                (parser->outerAcceptFlags | Parser::AcceptCloseSquare) & ~Parser::AcceptCloseAngle;
            parser->pp->tokenizeCloseAnglesOnly = false;
            break;
        }
        default: {
            PLY_ASSERT(0); // Illegal
            break;
        }
    }
}

PLY_NO_INLINE SetAcceptFlagsInScope::~SetAcceptFlagsInScope() {
    parser->outerAcceptFlags = this->prevAcceptFlags;
    parser->pp->tokenizeCloseAnglesOnly = this->prevTokenizeCloseAngles;
}

} // namespace cpp
} // namespace ply
