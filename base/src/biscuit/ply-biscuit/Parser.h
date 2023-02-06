/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-biscuit/Core.h>
#include <ply-biscuit/Tokenizer.h>
#include <ply-biscuit/ParseTree.h>

namespace ply {
namespace biscuit {

struct KeywordParams {
    biscuit::ExpandedToken kw_token;
    biscuit::StatementBlock* stmt_block = nullptr;
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
typedef void FunctionHandler(Owned<biscuit::Statement>&& stmt,
                             const ExpandedToken& name_token);

struct Parser {
    // Tokenizer.
    Tokenizer* tkr = nullptr;

    // Error reporting.
    Func<void(StringView message)> error;
    u32 error_count = 0;

    // Error recovery.
    struct RecoveryState {
        // When an illegal token is encountered, errors are muted for all subsequent
        // tokens until the parser recognizes a token that gets it back on track,
        // usually in an outer scope.
        bool mute_errors = false;
        static constexpr u32 AcceptCloseCurly = 1;
        static constexpr u32 AcceptCloseParen = 2;
        static constexpr u32 AcceptCloseSquare = 4;
        u32 outer_accept_flags = 0;
    };
    RecoveryState recovery;

    // Properties and hooks for extensibility.
    Map<Label, bool> keywords;
    struct Filter {
        Func<KeywordHandler> keyword_handler;
        Func<ValidateAttribute> validate_attributes;
        bool allow_functions = false;
        bool allow_instructions = false;
    };
    Filter filter;
    Func<FunctionHandler> function_handler;
    Statement* outer_scope = nullptr;

    Parser();
    Owned<Expression> parse_expression(u32 outer_precendence_level = Limits<u32>::Max,
                                       bool as_statement = false);
    void parse_statement(StatementBlock* stmt_block);
};

enum class ErrorTokenAction {
    DoNothing,
    PushBack,
    HandleUnexpected,
};

struct StatementBlockProperties {
    StringView block_type;
    StringView after_item_text;
    bool curly_braces_optional_if_control_flow = false;

    PLY_INLINE
    StatementBlockProperties(StringView block_type, StringView after_item_text = {},
                             bool curly_braces_optional_if_control_flow = false)
        : block_type{block_type}, after_item_text{after_item_text},
          curly_braces_optional_if_control_flow{curly_braces_optional_if_control_flow} {
    }
};

bool error_at_token(Parser* parser, const ExpandedToken& error_token,
                    ErrorTokenAction token_action, StringView message);
bool skip_any_scope(Parser* parser, ExpandedToken* out_close_token,
                    TokenType open_token_type);
bool handle_unexpected_token(Parser* parser, ExpandedToken* out_close_token,
                             const ExpandedToken& unexpected);
bool parse_parameter_list(Parser* parser, Statement::FunctionDefinition* function_def);
Owned<StatementBlock> parse_statement_block(Parser* parser,
                                            const StatementBlockProperties& props);
Owned<StatementBlock> parse_statement_block_inner(Parser* parser,
                                                  const StatementBlockProperties& props,
                                                  bool file_scope = false);

} // namespace biscuit
} // namespace ply
