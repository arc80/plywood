/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-crowbar/Core.h>
#include <ply-crowbar/Tokenizer.h>
#include <ply-crowbar/ParseTree.h>

namespace ply {
namespace crowbar {

struct Parser {
    // Hooks to customize parser behavior.
    struct Hooks {
        virtual bool recognizeKeyword(const ExpandedToken& token) {
            return false;
        }
        virtual void onError(StringView errorMsg) = 0;
    };
    Hooks* hooks = nullptr;

    // Tokenizer.
    Tokenizer* tkr = nullptr;

    // Error recovery state.
    struct RecoveryState {
        // When an illegal token is encountered, errors are muted for all subsequent tokens until
        // the parser recognizes a token that gets it back on track, usually in an outer scope.
        bool muteErrors = false;
        static constexpr u32 AcceptCloseCurly = 1;
        static constexpr u32 AcceptCloseParen = 2;
        static constexpr u32 AcceptCloseSquare = 4;
        u32 outerAcceptFlags = 0;
    };
    RecoveryState recovery;

    Owned<Expression> parseExpression(u32 outerPrecendenceLevel = Limits<u32>::Max, bool asStatement = false);
    Owned<Statement> parseStatement();
    Owned<File> parseFile();
};

enum class ErrorTokenAction {
    DoNothing,
    PushBack,
    HandleUnexpected,
};

struct StatementBlockProperties {
    StringView blockType;
    StringView afterItemText;
    bool curlyBracesOptionalIfControlFlow = false;

    PLY_INLINE StatementBlockProperties(StringView blockType, StringView afterItemText = {},
                                        bool curlyBracesOptionalIfControlFlow = false)
        : blockType{blockType}, afterItemText{afterItemText},
          curlyBracesOptionalIfControlFlow{curlyBracesOptionalIfControlFlow} {
    }
};

bool error(Parser* parser, const ExpandedToken& errorToken, ErrorTokenAction tokenAction,
           StringView message);
bool skipAnyScope(Parser* parser, ExpandedToken* outCloseToken, TokenType openTokenType);
bool handleUnexpectedToken(Parser* parser, ExpandedToken* outCloseToken,
                           const ExpandedToken& unexpected);
Owned<StatementBlock> parseStatementBlock(Parser* parser, const StatementBlockProperties& props);

} // namespace crowbar
} // namespace ply
