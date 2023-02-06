/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-cpp/Core.h>
#include <ply-cpp/Token.h>
#include <ply-cpp/Grammar.h>
#include <ply-cpp/Preprocessor.h>
#include <ply-cpp/Error.h>

namespace ply {
namespace cpp {

struct Parser;
struct RecoveryPoint;
struct RecoveryCheckRequired;
struct SpecDcorMode;
struct PPVisitedFiles;

struct ParseSupervisor {
    Parser* parser = nullptr;
    // scope_stack items can be:
    //  - grammar::TranslationUnit
    //  - grammar::DeclSpecifier::Record
    //  - grammar::DeclSpecifier::Enum_
    //  - grammar::Declaration::Linkage
    //  - grammar::Declaration::Namespace_
    //  - grammar::Declaration::Template_
    //  - grammar::Declaration::Simple (must be a function with a body!)
    Array<AnyObject> scope_stack;

    virtual void enter(AnyObject node) {
    }
    virtual void exit(AnyObject node) {
    }
    virtual void on_got_declaration(const grammar::Declaration& decl) {
    }
    virtual void on_got_enumerator(const grammar::InitEnumeratorWithComma* init_enor) {
    }
    virtual void got_macro_or_comment(Token token) {
    }
    virtual void on_got_include(StringView directive) {
    }
    virtual bool handle_error(Owned<BaseError>&& err) {
        return false;
    }

    PLY_INLINE void do_enter(AnyObject node) {
        this->scope_stack.append(node);
        this->enter(node);
    }
    void do_exit(AnyObject node);
    void got_declaration(grammar::Declaration&& decl);

    String get_class_name(StringView with_sep = "::",
                          bool with_name_space = true) const;
    String get_namespace_prefix() const;
};

enum class ExpectedToken {
    // ply reflect enum
    None,
    Identifier,
    NestedNamePrefix,
    OpenParen,
    OpenCurly,
    OpenAngle,
    OpenCurlyOrParen,
    CloseParen,
    CloseSquare,
    DestructorClassName,
    OperatorToken,
    Colon,
    Equal,
    QualifiedID,
    UnqualifiedID,
    Semicolon,
    Comma,
    CommaOrCloseParen,
    CommaOrCloseCurly,
    CommaOrOpenCurly,
    Declaration,
    EnumeratorOrCloseCurly,
    CommaOrCloseAngle,
    TrailingReturnType,
    BaseOrMember,
    ParameterType,
    TemplateParameterDecl,
    TypeSpecifier,
    ClassKeyword,
};

struct ParseError : BaseError {
    enum Type {
        // ply reflect enum
        Invalid,
        Expected,
        UnexpectedEOF,
        UnclosedToken,
        MissingCommaAfterEnumerator,
        UnmatchedCloseToken,
        QualifierNotAllowedHere,
        TypeIDCannotHaveName,
        NestedNameNotAllowedHere,
        TooManyTypeSpecifiers,
        ExpectedFunctionBodyAfterMemberInitList,
        CantMixFunctionDefAndDecl,
        ScopedEnumRequiresName,
        MissingDeclaration,
        DuplicateVirtSpecifier,
    };

    PLY_REFLECT()
    Type type = Invalid;
    Token error_token;
    ExpectedToken expected = ExpectedToken::None; // Only used by some error types
    Token preceding_token;                        // Only used by some error types
    // ply reflect off

    PLY_INLINE ParseError() = default;
    PLY_INLINE ParseError(Type type, Token error_token,
                          ExpectedToken expected = ExpectedToken::None,
                          Token preceding_token = {})
        : type{type}, error_token{error_token}, expected{expected},
          preceding_token{preceding_token} {
    }
    virtual void write_message(OutStream& out,
                               const PPVisitedFiles* visited_files) const override;
};

struct Parser {
    static constexpr u32 AcceptOpenCurly = 0x1;
    static constexpr u32 AcceptCloseCurly = 0x2;
    static constexpr u32 AcceptCloseParen = 0x4;
    static constexpr u32 AcceptCloseSquare = 0x8;
    static constexpr u32 AcceptCloseAngle = 0x10;
    static constexpr u32 AcceptComma = 0x20;
    static constexpr u32 AcceptSemicolon = 0x40;

    ParseSupervisor* visor = nullptr;
    Preprocessor* pp = nullptr;

    // This is temporary until we have a better way to do backtracking and pushback
    bool restore_point_enabled = false;
    Array<Token> token_queue;
    u32 token_queue_pos = 0;

    // Status
    u32 pass_number = 1;
    bool at_declaration_scope = true;

    // Error recovery
    u32 raw_error_count = 0;
    bool mute_errors = false;
    u32 outer_accept_flags = 0;

    PLY_INLINE void stop_muting_errors() {
        this->mute_errors = false;
    }

    void error(bool begin_muting, ParseError&& err);
};

enum class ParseQualifiedMode {
    AllowIncomplete,
    RequireComplete,
    RequireCompleteOrEmpty,
};

// Consumes as much as it can; unrecognized tokens are returned to caller without
// logging an error
grammar::QualifiedID parse_qualified_id(Parser* parser, ParseQualifiedMode mode);

// Declarators and specifiers
void parse_specifiers_and_declarators(Parser* parser,
                                      grammar::Declaration::Simple& simple,
                                      const SpecDcorMode& mode);

void parse_parameter_declaration_list(Parser* parser,
                                      grammar::ParamDeclarationList& params,
                                      bool for_template);
grammar::FunctionQualifierSeq parse_function_qualifier_seq(Parser* parser);

grammar::DeclaratorProduction*
parse_parameter_list(Parser* parser,
                     Owned<grammar::DeclaratorProduction>** prod_to_modify);
void parse_optional_function_body(Parser* parser, grammar::Initializer& result,
                                  const grammar::Declaration::Simple& simple);
void parse_optional_type_idinitializer(Parser* parser, grammar::Initializer& result);
void parse_optional_variable_initializer(Parser* parser, grammar::Initializer& result,
                                         bool allow_braced_init);
void parse_init_declarators(Parser* parser, grammar::Declaration::Simple& simple,
                            const SpecDcorMode& mode);
// Returns true if any tokens were consumed:
bool parse_declaration(Parser* parser, StringView enclosing_class_name);
void parse_declaration_list(Parser* parser, Token* out_close_curly,
                            StringView enclosing_class_name);
grammar::TranslationUnit parse_translation_unit(Parser* parser);

// Expressions
Tuple<Token, Token> parse_expression(Parser* parser, bool optional = false);

// Misc
void parse_enum_body(Parser* parser, grammar::DeclSpecifier::Enum_* en);

void dump_parse_tree(OutStream& out, AnyObject any, u32 indent = 0,
                     const PPVisitedFiles* visited_files = nullptr);
bool close_scope(Parser* parser, Token* out_close_token, const Token& open_token);

} // namespace cpp

PLY_DECLARE_TYPE_DESCRIPTOR(cpp::ExpectedToken)
PLY_DECLARE_TYPE_DESCRIPTOR(cpp::ParseError::Type)

} // namespace ply
