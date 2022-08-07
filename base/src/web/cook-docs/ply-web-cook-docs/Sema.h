/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-web-cook-docs/Core.h>
#include <ply-cpp/Token.h>

namespace ply {
namespace cpp {
namespace sema {

// These are based on cpp::grammar, except that they can outlive the parsed file
// because there are no Tokens (which are only valid as long as PPVisitedFiles continues to exist).

// Idea: Deduplicate common decl-specifier-sequences by hashing them?

struct QualifiedID;
struct DeclSpecifier;
struct DeclaratorProduction;
struct SingleDeclaration;

struct TemplateArg {
    struct Type {
        // ply make reflected switch
        struct Unknown {
            PLY_REFLECT()
            String expression;
            // ply reflect off
        };
        struct TypeID {
            PLY_REFLECT()
            Array<DeclSpecifier> declSpecifierSeq;
            Owned<DeclaratorProduction> abstractDcor;
            // ply reflect off
        };
#include "codegen/switch-ply-cpp-sema-TemplateArg-Type.inl" //@@ply
    };

    PLY_REFLECT()
    Type type;
    // ply reflect off
};

struct NestedNameComponent {
    // ply make reflected switch
    struct Identifier {
        PLY_REFLECT()
        String name;
        // ply reflect off
    };
    struct Templated {
        PLY_REFLECT()
        String name;
        Array<TemplateArg> args;
        // ply reflect off
    };
    struct DeclType {
        PLY_REFLECT()
        String expression;
        // ply reflect off
    };
#include "codegen/switch-ply-cpp-sema-NestedNameComponent.inl" //@@ply
};

struct UnqualifiedID {
    // ply make reflected switch
    struct Empty {
        PLY_REFLECT();
        // ply reflect off
    };
    struct Identifier {
        PLY_REFLECT()
        String name;
        // ply reflect off
    };
    struct TemplateID {
        PLY_REFLECT()
        String name;
        Array<TemplateArg> args;
        // ply reflect off
    };
    struct DeclType {
        PLY_REFLECT()
        String expression;
        // ply reflect off
    };
    struct Destructor {
        PLY_REFLECT()
        String name;
        // ply reflect off
    };
    struct OperatorFunc {
        PLY_REFLECT()
        cpp::Token::Type punc;
        cpp::Token::Type punc2;
        // ply reflect off
    };
    struct ConversionFunc {
        PLY_REFLECT()
        Array<DeclSpecifier> declSpecifierSeq;
        Owned<DeclaratorProduction> abstractDcor;
        // ply reflect off
    };
#include "codegen/switch-ply-cpp-sema-UnqualifiedID.inl" //@@ply
};

struct QualifiedID {
    PLY_REFLECT()
    Array<NestedNameComponent> nestedName;
    UnqualifiedID unqual;
    // ply reflect off

    PLY_INLINE bool isEmpty() const {
        return this->nestedName.isEmpty() && this->unqual.empty();
    }
    Array<StringView> getSimplifiedComponents() const;
};

struct DeclSpecifier {
    // ply make reflected switch
    struct Keyword {
        PLY_REFLECT()
        String token;
        // ply reflect off
    };
    struct TypeID { // FIXME: Should be called TypeSpec?
        PLY_REFLECT()
        bool hasTypename = false;
        bool wasAssumed = false;
        QualifiedID qid;
        // ply reflect off
    };
    struct TypeParam {
        PLY_REFLECT()
        bool hasEllipsis = false;
        // ply reflect off
    };
#include "codegen/switch-ply-cpp-sema-DeclSpecifier.inl" //@@ply
};

struct DeclaratorProduction {
    // ply make reflected switch
    struct PointerTo {
        PLY_REFLECT()
        cpp::Token::Type puncType;
        Owned<DeclaratorProduction> target;
        // ply reflect off
    };
    struct Function {
        PLY_REFLECT()
        Owned<DeclaratorProduction> target;
        Array<SingleDeclaration> params;
        Array<cpp::Token::Type> qualifiers;
        // ply reflect off
    };
    struct Qualifier {
        PLY_REFLECT()
        String keyword; // FIXME: Change to token type once const/volatile become token types
        Owned<DeclaratorProduction> target;
        // ply reflect off
    };
    struct ArrayOf {
        PLY_REFLECT()
        // For now, we don't store the array size (expression) in the sema
        Owned<DeclaratorProduction> target;
        // ply reflect off
    };
#include "codegen/switch-ply-cpp-sema-DeclaratorProduction.inl" //@@ply
};

struct Declarator {
    PLY_REFLECT()
    Owned<DeclaratorProduction> prod;
    QualifiedID qid;
    // ply reflect off
};

struct Initializer {
    // ply make reflected switch
    struct None {
        PLY_REFLECT()
        // ply reflect off
    };
    struct Assignment {
        PLY_REFLECT()
        TemplateArg::Type type;
        // ply reflect off
    };
    struct BitField {
        PLY_REFLECT()
        String expression;
        // ply reflect off
    };
#include "codegen/switch-ply-cpp-sema-Initializer.inl" //@@ply
};

struct SingleDeclaration {
    PLY_REFLECT()
    Array<DeclSpecifier> declSpecifierSeq;
    Declarator dcor;
    Initializer init;
    // ply reflect off
};

} // namespace sema
} // namespace cpp
} // namespace ply
