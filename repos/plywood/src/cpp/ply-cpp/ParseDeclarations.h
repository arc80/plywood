/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-cpp/Core.h>
#include <ply-cpp/Parser.h>

namespace ply {
namespace cpp {

struct SpecDcorMode {
    enum Mode {
        GlobalOrMember, // If it's a member, enclosingClassName will be non-empty
        Param,
        TemplateParam,
        TypeID,
        ConversionTypeID,
    };
    Mode mode = GlobalOrMember;
    StringView enclosingClassName;

    PLY_INLINE SpecDcorMode(Mode mode, StringView enclosingClassName = {})
        : mode{mode}, enclosingClassName{enclosingClassName} {
    }
    PLY_INLINE bool isAnyParam() const {
        return mode == Param || mode == TemplateParam;
    }
    PLY_INLINE bool isAnyTypeID() const {
        return mode == TypeID || mode == ConversionTypeID;
    }
};

Token readToken(Parser* parser, bool atDeclarationScope = false);
// FIXME: Change skipAnyScope to return outCloseToken (or an invalid token) instead
bool skipAnyScope(Parser* parser, Token* outCloseToken, const Token& openToken);
void pushBackToken(Parser* parser, const Token& token);
// Returns false if token was pushed back and caller must return to outer scope:
bool handleUnexpectedToken(Parser* parser, Token* outCloseToken, const Token& token);
bool okToStayInScope(Parser* parser, const Token& token);

struct ParseActivity {
    Parser* parser = nullptr;
    u32 savedErrorCount = 0;
    LinearLocation savedTokenLoc = 0;

    ParseActivity(Parser* parser) : parser{parser} {
        this->savedErrorCount = parser->rawErrorCount;
        Token token = readToken(parser);
        this->savedTokenLoc = token.linearLoc;
        pushBackToken(parser, token);
    }

    bool errorOccurred() const {
        return this->parser->rawErrorCount != this->savedErrorCount;
    }

    bool anyTokensConsumed() const {
        if (this->parser->tokenQueue.isEmpty())
            return false;
        return this->parser->tokenQueue[0].linearLoc == this->savedTokenLoc;
    }
};

// FIXME: Move somewhere else. It's an internal struct used by various parse functions.
struct SetAcceptFlagsInScope {
    Parser* parser;
    u32 prevAcceptFlags = 0;
    bool prevTokenizeCloseAngles = false;

    SetAcceptFlagsInScope(Parser* parser, Token::Type openTokenType);
    ~SetAcceptFlagsInScope();
};

} // namespace cpp
} // namespace ply
