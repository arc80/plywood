/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
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

    docs::SemaEntity* from_scope = nullptr;
    bool prepend_class_name = false;

    Array<Stringifier::Component>
    to_string_comps(const TemplateArg::Type& template_arg_type);
    Array<Stringifier::Component>
    apply_productions(const DeclaratorProduction* prod,
                      Array<Stringifier::Component>&& inner);
    Array<Stringifier::Component> to_string_comps(const QualifiedID& qid, QIDRole role);
    Array<Stringifier::Component>
    to_string_comps(ArrayView<const DeclSpecifier> decl_specifier_seq,
                    const DeclaratorProduction* prod, const QualifiedID& qid,
                    const Initializer& init, bool for_root_declarator);
    Array<Stringifier::Component> to_string_comps(const SingleDeclaration& single,
                                                  bool for_root_declarator) {
        return this->to_string_comps(single.decl_specifier_seq, single.dcor.prod,
                                     single.dcor.qid, single.init, for_root_declarator);
    }
};

Array<Stringifier::Component> to_string_comps(const SingleDeclaration& single,
                                              docs::SemaEntity* from_scope,
                                              bool prepend_class_name);
String to_string(const SingleDeclaration& single);

} // namespace sema
} // namespace cpp
} // namespace ply
