/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
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
    // scopeStack items can be:
    //  - grammar::TranslationUnit
    //  - grammar::DeclSpecifier::Record
    //  - grammar::DeclSpecifier::Enum_
    //  - grammar::Declaration::Linkage
    //  - grammar::Declaration::Namespace_
    //  - grammar::Declaration::Template_
    //  - grammar::Declaration::Simple (must be a function with a body!)
    Array<TypedPtr> scopeStack;

    virtual void enter(TypedPtr node) {
    }
    virtual void exit(TypedPtr node) {
    }
    virtual void onGotDeclaration(const grammar::Declaration& decl) {
    }
    virtual void onGotEnumerator(const grammar::InitEnumeratorWithComma* initEnor) {
    }
    virtual void gotMacroOrComment(Token token) {
    }
    virtual void onGotInclude(StringView directive) {
    }
    virtual bool handleError(Owned<BaseError>&& err) {
        return false;
    }

    PLY_INLINE void doEnter(TypedPtr node) {
        this->scopeStack.append(node);
        this->enter(node);
    }
    void doExit(TypedPtr node);
    void gotDeclaration(grammar::Declaration&& decl);

    String getClassName(StringView withSep = "::", bool withNameSpace = true) const;
    String getNamespacePrefix() const;
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
PLY_DECLARE_TYPE_DESCRIPTOR(ExpectedToken)

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
    Token errorToken;
    ExpectedToken expected = ExpectedToken::None; // Only used by some error types
    Token precedingToken;                         // Only used by some error types
    // ply reflect off

    PLY_INLINE ParseError() = default;
    PLY_INLINE ParseError(Type type, Token errorToken, ExpectedToken expected = ExpectedToken::None,
                          Token precedingToken = {})
        : type{type}, errorToken{errorToken}, expected{expected}, precedingToken{precedingToken} {
    }
    virtual void writeMessage(OutStream* outs, const PPVisitedFiles* visitedFiles) const override;
};
PLY_DECLARE_TYPE_DESCRIPTOR(ParseError::Type)

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
    bool restorePointEnabled = false;
    Array<Token> tokenQueue;
    u32 tokenQueuePos = 0;

    // Status
    u32 passNumber = 1;
    bool atDeclarationScope = true;

    // Error recovery
    u32 rawErrorCount = 0;
    bool muteErrors = false;
    u32 outerAcceptFlags = 0;

    PLY_INLINE void stopMutingErrors() {
        this->muteErrors = false;
    }

    void error(bool beginMuting, ParseError&& err);
};

enum class ParseQualifiedMode {
    AllowIncomplete,
    RequireComplete,
    RequireCompleteOrEmpty,
};

// Consumes as much as it can; unrecognized tokens are returned to caller without logging an error
grammar::QualifiedID parseQualifiedID(Parser* parser, ParseQualifiedMode mode);

// Declarators and specifiers
void parseSpecifiersAndDeclarators(Parser* parser, grammar::Declaration::Simple& simple,
                                   const SpecDcorMode& mode);

void parseParameterDeclarationList(Parser* parser, grammar::ParamDeclarationList& params,
                                   bool forTemplate);
grammar::FunctionQualifierSeq parseFunctionQualifierSeq(Parser* parser);

grammar::DeclaratorProduction*
parseParameterList(Parser* parser, Owned<grammar::DeclaratorProduction>** prodToModify);
void parseOptionalFunctionBody(Parser* parser, grammar::Initializer& result,
                               const grammar::Declaration::Simple& simple);
void parseOptionalTypeIDInitializer(Parser* parser, grammar::Initializer& result);
void parseOptionalVariableInitializer(Parser* parser, grammar::Initializer& result,
                                      bool allowBracedInit);
void parseInitDeclarators(Parser* parser, grammar::Declaration::Simple& simple,
                          const SpecDcorMode& mode);
// Returns true if any tokens were consumed:
bool parseDeclaration(Parser* parser, StringView enclosingClassName);
void parseDeclarationList(Parser* parser, Token* outCloseCurly, StringView enclosingClassName);
grammar::TranslationUnit parseTranslationUnit(Parser* parser);

// Expressions
Tuple<Token, Token> parseExpression(Parser* parser, bool optional = false);

// Misc
void parseEnumBody(Parser* parser, grammar::DeclSpecifier::Enum_* en);

void dumpParseTree(OutStream* outs, TypedPtr any, u32 indent = 0,
                   const PPVisitedFiles* visitedFiles = nullptr);
bool closeScope(Parser* parser, Token* outCloseToken, const Token& openToken);

} // namespace cpp
} // namespace ply
