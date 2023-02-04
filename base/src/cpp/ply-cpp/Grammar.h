/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-cpp/Core.h>
#include <ply-cpp/Token.h>

namespace ply {
namespace cpp {
namespace grammar {

struct ExpressionWithComma;
struct DeclSpecifier;
struct DeclaratorProduction;
struct TemplateArgumentWithComma;

//-----------------------------------------------------------------------------
// Expression is not fully developed yet.
// Will probably split into a separate file.
//-----------------------------------------------------------------------------
struct Expression {
    // ply make reflected switch
    struct Call {
        PLY_REFLECT()
        Token openParen;
        Token closeParen;
        Array<ExpressionWithComma> arguments;
        // ply reflect off
    };
#include "codegen/switch-ply-cpp-grammar-Expression.inl" //@@ply
};

struct ExpressionWithComma {
    PLY_REFLECT()
    Expression expr;
    Token comma;
    // ply reflect off
};

//-----------------------------------------------------------------------------
// NestedNameComponents describe the optional "scope specifiers" of qualified-ids.
// Corresponds to nested-name-specifier in the grammar.
// The components can be namespaces, class names, class template names or decltype() expressions.
//      Foo::
//      Foo<int>::
//      decltype(x)::
//-----------------------------------------------------------------------------
struct NestedNameComponent {
    struct Type {
        // ply make reflected switch
        struct IdentifierOrTemplated {
            PLY_REFLECT()
            Token name;
            Token openAngled; // If Invalid, it's not a template
            Token closeAngled;
            Array<TemplateArgumentWithComma> args;
            // ply reflect off
        };
        struct DeclType {
            PLY_REFLECT()
            Token keyword;
            Token openParen;
            Token closeParen;
            // ply reflect off
        };
#include "codegen/switch-ply-cpp-grammar-NestedNameComponent-Type.inl" //@@ply
    };

    PLY_REFLECT()
    Type type;
    Token sep;
    // ply reflect off

    Token getFirstToken() const;
};

//-----------------------------------------------------------------------------
// UnqualifiedID is the final component of a QualifiedID.
// Corresponds to unqualified-id in the grammar.
//      Foo::x
//           ^
//-----------------------------------------------------------------------------
struct UnqualifiedID {
    // ply make reflected switch
    struct Empty {
        PLY_REFLECT()
        // ply reflect off
    };
    struct Identifier {
        PLY_REFLECT()
        Token name;
        // ply reflect off
    };
    struct TemplateID {
        PLY_REFLECT()
        Token name;
        Token openAngled;
        Token closeAngled;
        Array<TemplateArgumentWithComma> args;
        // ply reflect off
    };
    struct DeclType {
        PLY_REFLECT()
        Token keyword;
        Token openParen;
        Token closeParen;
        // ply reflect off
    };
    struct Destructor {
        PLY_REFLECT()
        Token tilde;
        Token name;
        // ply reflect off
    };
    struct OperatorFunc {
        PLY_REFLECT()
        Token keyword;
        Token punc;
        Token punc2;
        // ply reflect off
    };
    struct ConversionFunc {
        PLY_REFLECT()
        Token keyword;
        Array<Owned<DeclSpecifier>> declSpecifierSeq;
        Owned<DeclaratorProduction> abstractDcor;
        // ply reflect off
    };

    StringView getCtorDtorName() const;
    Token getFirstToken() const;
#include "codegen/switch-ply-cpp-grammar-UnqualifiedID.inl" //@@ply
};

//-----------------------------------------------------------------------------
// QualifiedIDs identify variables, functions and types, and maybe have an optional
// Array<NestedNameComponents>.
// Corresponds to qualified-id in the grammar.
//      x
//      Foo
//      Foo::x
//      Foo::Bar::x
//      Foo::operator int
//      Foo::~Foo
//-----------------------------------------------------------------------------
struct QualifiedID {
    PLY_REFLECT()
    Array<NestedNameComponent> nestedName;
    UnqualifiedID unqual;
    // ply reflect off

    PLY_INLINE bool isEmpty() const {
        return this->nestedName.isEmpty() && this->unqual.empty();
    }
    PLY_INLINE bool isNestedNameOnly() const {
        return !this->nestedName.isEmpty() && this->unqual.empty();
    }
    PLY_INLINE bool isComplete() const {
        return !this->unqual.empty();
    }
    PLY_INLINE StringView getClassName() const {
        if (auto ident = this->unqual.identifier()) {
            return ident->name.identifier;
        } else if (auto templateID = this->unqual.templateID()) {
            return templateID->name.identifier;
        } else {
            return {};
        }
    }
    String toString() const;
    Token getFirstToken() const;
};

struct TemplateArgumentWithComma {
    // FIXME: template arguments should support constant expressions, too...
    struct Type {
        // ply make reflected switch
        struct Unknown {
            PLY_REFLECT()
            Token startToken;
            Token endToken;
            // ply reflect off
        };
        struct TypeID {
            PLY_REFLECT()
            Array<Owned<DeclSpecifier>> declSpecifierSeq;
            Owned<DeclaratorProduction> abstractDcor;
            // ply reflect off
        };
#include "codegen/switch-ply-cpp-grammar-TemplateArgumentWithComma-Type.inl" //@@ply
    };

    PLY_REFLECT()
    Type type;
    Token comma;
    // ply reflect off
};

//-----------------------------------------------------------------------------
// An Array<BaseSpecifierWithCommas> describes the base class list at the top of a class definition.
// The last item in the array does not normally have a comma. Corresponds to base-specifier-list in
// the grammar.
//      class Foo : public Bar, protected Wiz
//                  ^^^^^^^^^^^^^^^^^^^^^^^^^
//-----------------------------------------------------------------------------
struct BaseSpecifierWithComma {
    PLY_REFLECT()
    Token accessSpec;
    QualifiedID baseQid;
    Token comma;
    // ply reflect off
};

struct InitEnumeratorWithComma;

struct Declaration;

//-----------------------------------------------------------------------------
// An Array<Owned<DeclSpecifier>>, combined with a Declarator (possibly wrapped in an
// InitDeclaratorWithComma), describes a declaration, function parameter, template parameter or type
// id (as in an alias). Corresponds to decl-specifier or type-specifier in the grammar.
//
// The Array<Owned<DeclSpecifier>> is the part before the name being declared (if any), and
// basically describes the "base type" of the declaration, which the Declarator can modify into a
// pointer, reference, array, etc.
//
// The reason why there's an array of DeclSpecifiers is because multiple keywords can describe the
// base type:
//      const unsigned int x;
//      ^^^^^^^^^^^^^^^^^^
//
// A DeclSpecifier can also be a class or enum definition. (If followed by a Declarator, a variable
// can be declared at the same time as the class or enum.)
//-----------------------------------------------------------------------------
struct DeclSpecifier {
    // ply make reflected switch
    struct Keyword {
        PLY_REFLECT()
        Token token;
        // ply reflect off
    };
    struct LangLinkage {
        PLY_REFLECT()
        Token extern_;
        Token literal;
        // ply reflect off
    };
    struct Record {
        PLY_REFLECT()
        Token classKey;
        QualifiedID qid;
        Array<Token> virtSpecifiers;
        Token colon;
        Array<BaseSpecifierWithComma> baseSpecifierList;
        Token openCurly;
        Token closeCurly;
        Array<Declaration> visor_decls; // Modified by supervisor
        // ply reflect off
    };
    struct Enum_ {
        PLY_REFLECT()
        Token enumKey;
        Token classKey;
        QualifiedID qid;
        Token basePunc;
        QualifiedID base;
        Token openCurly;
        Token closeCurly;
        Array<Owned<InitEnumeratorWithComma>> enumerators;
        // ply reflect off
    };
    struct TypeID { // FIXME: Should be called TypeSpec?
        PLY_REFLECT()
        Token typename_; // If preceded by typename keyword
        QualifiedID qid;
        // wasAssumed will be true whenever the parser makes a (possibly wrong) assumption due to
        // lack of type knowledge. For example:
        //      void func(int(A));
        //                    ^
        // If the parser does not definitively know whether A identifies a type, it will assume that
        // it is a type and set "wasAssumed" to true. The first parameter of func will be parsed as
        // an unnamed function that takes an unnamed parameter of type A and returns int, instead of
        // as an integer named A, which is how it would have been parsed if A did not identify a
        // type.
        bool wasAssumed = false;
        // ply reflect off
    };
    struct TypeParam {
        PLY_REFLECT()
        Token keyword; // typename or class
        Token ellipsis;
        Token identifier; // FIXME: Eliminate this, it goes in the QID
        // ply reflect off
    };
    struct Ellipsis {
        PLY_REFLECT()
        Token ellipsisToken;
        // ply reflect off
    };
#include "codegen/switch-ply-cpp-grammar-DeclSpecifier.inl" //@@ply
};

struct ParamDeclarationWithComma;

//-----------------------------------------------------------------------------
// ParamDeclarationList is used to describe both function and template parameters.
// Corresponds to parameter-declaration-list or template-parameter-list in the grammar.
//-----------------------------------------------------------------------------
struct ParamDeclarationList {
    PLY_REFLECT()
    Token openPunc;
    Token closePunc;
    Array<ParamDeclarationWithComma> params;
    // ply reflect off
};

//-----------------------------------------------------------------------------
// FunctionQualifierSeq is where member functions are made const, among other things.
// Corresponds to the "qualifiers" part of parameters-and-qualifiers in the grammar.
//      int getValue() const;
//                     ^^^^^
//-----------------------------------------------------------------------------
struct FunctionQualifierSeq {
    PLY_REFLECT()
    Array<Token> tokens;
    // ply reflect off
};

//-----------------------------------------------------------------------------
// Every Declarator has an optional chain of DeclaratorProductions. They're used to modify the base
// type of a declaration. They let you declare pointers, arrays and functions.
//-----------------------------------------------------------------------------
struct DeclaratorProduction {
    struct Type {
        // ply make reflected switch
        struct Parenthesized {
            PLY_REFLECT()
            Token openParen;
            Token closeParen;
            // ply reflect off
        };
        struct PointerTo {
            PLY_REFLECT()
            Array<NestedNameComponent> nestedName;
            Token punc;
            // ply reflect off
        };
        struct ArrayOf {
            PLY_REFLECT()
            Token openSquare;
            Token closeSquare;
            Expression size;
            // ply reflect off
        };
        struct Function {
            PLY_REFLECT()
            ParamDeclarationList params;
            FunctionQualifierSeq qualifiers;
            Token arrow;
            QualifiedID trailingRetType;
            // ply reflect off
        };
        struct Qualifier {
            PLY_REFLECT()
            Token keyword;
            // ply reflect off
        };
#include "codegen/switch-ply-cpp-grammar-DeclaratorProduction-Type.inl" //@@ply
    };

    PLY_REFLECT()
    Type type;
    Owned<DeclaratorProduction> target;
    // ply reflect off
};

//-----------------------------------------------------------------------------
// Declarators are combined with an Array<Owned<DeclSpecifier>> to form a declaration, function
// parameter, template parameter or type id (as in an alias). Corresponds to declarator or
// abstract-declarator in the grammar.
//
// In the case of a variable declaration, there can be multiple declarators:
//      int x, y;
//          ^^^^
//
// In the case of an function parameter or template parameter, the Declarator can be abstract, which
// means that the parameter is unnamed (QualifiedID is blank), and there is only the optional
// DeclaratorProduction chain which modifies the base type into a pointer, function, etc.
//      void func(int, char*);
//                   ^     ^
//
// In the case of a type alias, the Declarator is always abstract.
//      using Func = int();
//                      ^^
//-----------------------------------------------------------------------------
struct Declarator {
    PLY_REFLECT()
    Owned<DeclaratorProduction> prod;
    QualifiedID qid;
    // ply reflect off

    PLY_INLINE bool isFunction() const {
        return (this->prod && this->prod->type.function());
    };
};

//-----------------------------------------------------------------------------
// An Array<MemberInitializerWithComma> describes the optional member initializer list of a
// constructor. Corresponds to mem-initializer-list in the grammar.
//      Foo() : x{5}, y{7} {}
//              ^^^^^^^^^^
//-----------------------------------------------------------------------------
struct MemberInitializerWithComma {
    PLY_REFLECT()
    QualifiedID qid;
    Token openPunc;
    Token closePunc;
    Token comma;
    // ply reflect off
};

struct AssignmentType {
    // ply make reflected switch
    struct Expression {
        // Temporarily store the expression as a location span until we start parsing
        // expressions correctly
        PLY_REFLECT()
        Token start;
        Token end;
        // ply reflect off
    };
    struct TypeID {
        PLY_REFLECT()
        Array<Owned<DeclSpecifier>> declSpecifierSeq;
        Owned<DeclaratorProduction> abstractDcor;
        // ply reflect off
    };
#include "codegen/switch-ply-cpp-grammar-AssignmentType.inl" //@@ply
};

//-----------------------------------------------------------------------------
// Initializer is used by InitDeclaratorWithComma, and describes the optional part immediately
// following a Declarator. In the grammar, it corresponds to initializer, function-body and part of
// the bitfield production rule in member-declarator.
//      int x = 5;
//            ^^^
//      int func() {}
//              ^^^^^
//      int flag : 1;
//               ^^^
//
// It's also currently used by InitEnumerationWithComma and ParamDeclarationWithComma, but in those
// cases, it can only take the Assignment form. (Might stop using it in those cases, and just store
// the optional assignment directly.)
//-----------------------------------------------------------------------------
struct Initializer {
    // ply make reflected switch
    struct None {
        PLY_REFLECT()
        // ply reflect off
    };
    struct Assignment {
        PLY_REFLECT()
        AssignmentType type;
        Token equalSign;
        // ply reflect off
    };
    struct FunctionBody {
        PLY_REFLECT()
        Token colon;
        Array<MemberInitializerWithComma> memberInits;
        Token openCurly;
        Token closeCurly;
        // ply reflect off
    };
    struct BitField {
        PLY_REFLECT()
        Token colon;
        // Temporarily store the constant expression as a location span until we start parsing
        // expressions correctly
        Token expressionStart;
        Token expressionEnd;
        // ply reflect off
    };
#include "codegen/switch-ply-cpp-grammar-Initializer.inl" //@@ply
};

//-----------------------------------------------------------------------------
// An Array<InitDeclaratorWithComma> is used as part of Declaration::Simple, wrapping a Declarator
// and allowing it to have an optional Initializer. It corresponds to init-declarator-list in the
// grammar, except when the Initializer is a FunctionBody or a BitField. In those cases, it
// corresponds function-body and the bitfield production rule in member-declarator respectively.
// (Consider making separate Declaration states for the latter cases -- especially
// FunctionDefinition -- intsead of cramming everything into Declaration::Simple, as we do now.)
//-----------------------------------------------------------------------------
struct InitDeclaratorWithComma {
    PLY_REFLECT()
    Declarator dcor;
    Initializer init;
    Token comma;
    // ply reflect off
};

//-----------------------------------------------------------------------------
// An Array<Owned<InitEnumeratorWithComma>> is used by DeclSpecifier::Enum_ and describes a list of
// enum entries with their optional initialzers. It corresponds to enumerator-list in the grammar.
//      enum Color { red = 0, green, blue };
//                   ^^^^^^^^^^^^^^^^^^^^
//-----------------------------------------------------------------------------
struct InitEnumeratorWithComma {
    PLY_REFLECT()
    Token identifier;
    Initializer init;
    Token comma;
    // ply reflect off
};

//-----------------------------------------------------------------------------
// ParamDeclarationWithComma is used by ParamDeclarationList to describe a single function parameter
// or template parameter. It corresponds to parameter-declaration or template-parameter in the
// grammar.
//-----------------------------------------------------------------------------
struct ParamDeclarationWithComma {
    PLY_REFLECT()
    Array<Owned<DeclSpecifier>> declSpecifierSeq;
    Declarator dcor; // possibly abstract
    Initializer init;
    Token comma;
    // ply reflect off
};

//-----------------------------------------------------------------------------
// A Declaration describes an entry in the top-level namespace, a namespace, a class or a class
// template. It corresponds to declaration or member-declaration in the grammar. Some of its states
// are only allowed to appear in certain contexts; eg. AccessSpecifier can only appear inside a
// class.
//
// At parse time, the Array<Declaration> is populated by the parse supervisor, and not by the parser
// itself. (Need to document the reason for this, but don't remember right now.)
//-----------------------------------------------------------------------------
struct Declaration {
    // ply make reflected switch
    struct Namespace_ {
        PLY_REFLECT()
        Token keyword;
        QualifiedID qid;
        Token openCurly;
        Token closeCurly;
        Array<Declaration> visor_decls; // Modified by supervisor
        // ply reflect off
    };
    struct Template_ {
        PLY_REFLECT()
        Token keyword;
        ParamDeclarationList params;
        Owned<Declaration> visor_decl; // Modified by supervisor
        // ply reflect off
    };
    struct Simple {
        PLY_REFLECT()
        Array<Owned<DeclSpecifier>> declSpecifierSeq;
        Array<InitDeclaratorWithComma> initDeclarators;
        Token semicolon;
        // ply reflect off
    };
    struct AccessSpecifier {
        PLY_REFLECT()
        Token keyword;
        Token colon;
        // ply reflect off
    };
    struct StaticAssert {
        PLY_REFLECT()
        Token keyword;
        Expression::Call argList;
        Token semicolon;
        // ply reflect off
    };
    struct UsingDirective {
        PLY_REFLECT()
        Token using_;
        Token namespace_;
        QualifiedID qid;
        Token semicolon;
        // ply reflect off
    };
    struct Alias {
        PLY_REFLECT()
        Token using_;
        Token name;
        Token equals;
        Array<Owned<DeclSpecifier>> declSpecifierSeq;
        Declarator dcor; // should be abstract
        Token semicolon;
        // ply reflect off
    };
    struct Linkage {
        PLY_REFLECT()
        Token extern_;
        Token literal;
        Token openCurly;
        Token closeCurly;
        Array<Declaration> visor_decls; // Modified by supervisor
        // ply reflect off
    };
    struct Empty {
        PLY_REFLECT()
        Token semicolon;
        // ply reflect off
    };
#include "codegen/switch-ply-cpp-grammar-Declaration.inl" //@@ply
};

//-----------------------------------------------------------------------------
// TranslationUnit describes the top-level namespace of a source file. Correponds to
// translation-unit in the grammar.
//-----------------------------------------------------------------------------
struct TranslationUnit {
    PLY_REFLECT()
    Array<Declaration> visor_decls; // Modified by supervisor
    // ply reflect off
};

} // namespace grammar
} // namespace cpp
} // namespace ply
