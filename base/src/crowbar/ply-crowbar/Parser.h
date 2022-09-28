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

typedef bool CustomBlockHandler(const ExpandedToken& kwToken, StatementBlock* stmtBlock);
typedef bool ExpressionTraitHandler(const ExpandedToken& kwToken, AnyOwnedObject* expressionTraits);

struct Parser {
    LabelMap<Functor<CustomBlockHandler>>* customBlockHandlers = nullptr;
    LabelMap<Functor<ExpressionTraitHandler>>* exprTraitHandlers = nullptr;
    Functor<bool(const ExpandedToken& nameToken, const Statement* stmt, bool isEntering)>
        onDefineFunction; // return value of true rejects the function

    // Tokenizer.
    Tokenizer* tkr = nullptr;

    // Error reporting.
    Functor<void(StringView message)> error;
    u32 errorCount = 0;

    // Error recovery.
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

    // Information about the current parsing context:
    Statement* functionLikeScope = nullptr; // nullptr means file scope

    Owned<Expression> parseExpression(u32 outerPrecendenceLevel = Limits<u32>::Max,
                                      bool asStatement = false);
    void parseStatement(StatementBlock* stmtBlock);
    Owned<StatementBlock> parseFile();
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

bool errorAtToken(Parser* parser, const ExpandedToken& errorToken, ErrorTokenAction tokenAction,
                  StringView message);
bool skipAnyScope(Parser* parser, ExpandedToken* outCloseToken, TokenType openTokenType);
bool handleUnexpectedToken(Parser* parser, ExpandedToken* outCloseToken,
                           const ExpandedToken& unexpected);
Owned<StatementBlock> parseStatementBlock(Parser* parser, const StatementBlockProperties& props);

} // namespace crowbar
} // namespace ply
