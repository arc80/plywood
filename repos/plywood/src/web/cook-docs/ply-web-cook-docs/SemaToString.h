/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-web-cook-docs/Core.h>
#include <ply-web-cook-docs/Sema.h>
#include <ply-web-cook-docs/SemaEntity.h>

namespace ply {
namespace cpp {
namespace sema {

struct Stringifier;

struct Stringifier {
    struct Component {
        enum Type {
            PointerOrReference,
            KeywordOrIdentifier,
            Other,
            BeginRootDeclarator,
            EndRootDeclarator,
        };
        Type type = KeywordOrIdentifier;
        String text;
        docs::SemaEntity* sema = nullptr;
    };

    enum class QIDRole {
        TypeSpecifier,
        Declarator,
        RootDeclarator,
    };

    docs::SemaEntity* fromScope = nullptr;
    bool prependClassName = false;

    Array<Stringifier::Component> toStringComps(const TemplateArg::Type& templateArgType);
    Array<Stringifier::Component> applyProductions(const DeclaratorProduction* prod,
                                                   Array<Stringifier::Component>&& inner);
    Array<Stringifier::Component> toStringComps(const QualifiedID& qid, QIDRole role);
    Array<Stringifier::Component> toStringComps(ArrayView<const DeclSpecifier> declSpecifierSeq,
                                                const DeclaratorProduction* prod,
                                                const QualifiedID& qid, const Initializer& init,
                                                bool forRootDeclarator);
    PLY_INLINE Array<Stringifier::Component> toStringComps(const SingleDeclaration& single,
                                                           bool forRootDeclarator) {
        return this->toStringComps(single.declSpecifierSeq, single.dcor.prod,
                                   single.dcor.qid, single.init, forRootDeclarator);
    }
};

Array<Stringifier::Component> toStringComps(const SingleDeclaration& single,
                                            docs::SemaEntity* fromScope, bool prependClassName);
String toString(const SingleDeclaration& single);

} // namespace sema
} // namespace cpp
} // namespace ply
