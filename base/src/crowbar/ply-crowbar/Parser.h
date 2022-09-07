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
    struct CustomBlockHooks {
        template <typename T>
        using Callback = bool(T* arg, const ExpandedToken& kwToken, StatementBlock* stmtBlock);
        struct Handler {
            Callback<void>* callback = nullptr;
            void* arg = nullptr;
        };
        LabelMap<Handler> map;

        template <typename T>
        PLY_INLINE void add(Label keyword, Callback<T>* callback, T* arg) {
            *this->map.insert(keyword) = {(Callback<void>*) callback, (void*) arg};
        }
    };

    CustomBlockHooks* customBlockHooks = nullptr;

    struct ExpressionTraitHooks {
        template <typename T>
        using Callback = bool(T* hooks, const crowbar::ExpandedToken& kwToken,
                              AnyOwnedObject* expressionTraits);
        struct Handler {
            Callback<void>* callback = nullptr;
            void* arg = nullptr;
        };
        LabelMap<Handler> map;

        template <typename T>
        PLY_INLINE void add(Label keyword, Callback<T>* callback, T* arg) {
            *this->map.insert(keyword) = {(Callback<void>*) callback, (void*) arg};
        }
    };

    ExpressionTraitHooks* exprTraitHooks = nullptr;

    // Tokenizer.
    Tokenizer* tkr = nullptr;

    // Error reporting.
    OutStream* errorOut = nullptr;
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

    // Information about the current parsing context.
    struct Context {
        Statement::FunctionDefinition* func = nullptr;
        Statement::CustomBlock* customBlock = nullptr;
    };
    Context context;

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

bool error(Parser* parser, const ExpandedToken& errorToken, ErrorTokenAction tokenAction,
           StringView message);
bool skipAnyScope(Parser* parser, ExpandedToken* outCloseToken, TokenType openTokenType);
bool handleUnexpectedToken(Parser* parser, ExpandedToken* outCloseToken,
                           const ExpandedToken& unexpected);
Owned<StatementBlock> parseStatementBlock(Parser* parser, const StatementBlockProperties& props);

} // namespace crowbar
} // namespace ply
