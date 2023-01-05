/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-biscuit/Core.h>
#include <ply-biscuit/Tokenizer.h>
#include <ply-biscuit/ParseTree.h>

namespace ply {
namespace biscuit {

struct KeywordParams {
    biscuit::ExpandedToken kwToken;
    biscuit::StatementBlock* stmtBlock = nullptr;
    AnyOwnedObject* attributes;
};
enum class KeywordResult {
    Illegal,
    Attribute,
    Block,
    Error,
};
typedef KeywordResult KeywordHandler(const KeywordParams& kp);
typedef void ValidateAttribute(Statement* stmt);
typedef void FunctionHandler(Owned<biscuit::Statement>&& stmt, const ExpandedToken& nameToken);

struct Parser {
    // Tokenizer.
    Tokenizer* tkr = nullptr;

    // Error reporting.
    Func<void(StringView message)> error;
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

    // Properties and hooks for extensibility.
    LabelMap<bool> keywords;
    struct Filter {
        Func<KeywordHandler> keywordHandler;
        Func<ValidateAttribute> validateAttributes;
        bool allowFunctions = false;
        bool allowInstructions = false;
    };
    Filter filter;
    Func<FunctionHandler> functionHandler;
    Statement* outerScope = nullptr;

    Parser();
    Owned<Expression> parseExpression(u32 outerPrecendenceLevel = Limits<u32>::Max,
                                      bool asStatement = false);
    void parseStatement(StatementBlock* stmtBlock);
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
bool parseParameterList(Parser* parser, Statement::FunctionDefinition* functionDef);
Owned<StatementBlock> parseStatementBlock(Parser* parser, const StatementBlockProperties& props);
Owned<StatementBlock> parseStatementBlockInner(Parser* parser,
                                               const StatementBlockProperties& props,
                                               bool fileScope = false);

} // namespace biscuit
} // namespace ply
