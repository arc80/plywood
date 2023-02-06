/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-web-cook-docs/Core.h>
#include <ply-cpp/Grammar.h>
#include <ply-web-cook-docs/SemaToString.h>

namespace ply {
namespace cpp {
namespace sema {

Array<Stringifier::Component>
Stringifier::to_string_comps(const TemplateArg::Type& template_arg_type) {
    if (auto unknown = template_arg_type.unknown()) {
        return {{Component::Other, unknown->expression, nullptr}};
    } else if (auto type_id = template_arg_type.type_id()) {
        return this->to_string_comps(type_id->decl_specifier_seq,
                                     type_id->abstract_dcor, {}, {}, false);
    } else {
        PLY_ASSERT(0);
        return {};
    }
}

Array<Stringifier::Component>
Stringifier::apply_productions(const DeclaratorProduction* prod,
                               Array<Stringifier::Component>&& comps) {
    auto starts_with_identifier = [](ArrayView<Stringifier::Component> comps) {
        return comps.num_items >= 1 &&
               (comps[0].type == Component::KeywordOrIdentifier ||
                comps[0].type == Component::BeginRootDeclarator);
    };
    bool has_left_part = false;
    auto parenthesize_if_needed = [&] {
        if (!has_left_part)
            return;
        comps.insert(0) = {Component::Other, "(", nullptr};
        comps.append({Component::Other, ")", nullptr});
        has_left_part = false;
    };
    while (prod) {
        if (auto pointer_to = prod->pointer_to()) {
            if (starts_with_identifier(comps)) {
                comps.insert(0) = {Component::Other, " ", nullptr};
            }
            comps.insert(0) = {Component::PointerOrReference,
                               get_punctuation_string(pointer_to->punc_type), nullptr};
            has_left_part = true;
            prod = pointer_to->target;
        } else if (auto function = prod->function()) {
            parenthesize_if_needed();
            comps.append({Component::Other, "(", nullptr});
            for (u32 i = 0; i < function->params.num_items(); i++) {
                if (i > 0) {
                    comps.append({Component::Other, ", ", nullptr});
                }
                const SingleDeclaration& param = function->params[i];
                comps.move_extend(this->to_string_comps(param, false));
            }
            comps.append({Component::Other, ")", nullptr});
            for (cpp::Token::Type qual : function->qualifiers) {
                if (qual == cpp::Token::Identifier) {
                    // FIXME: We're assuming the only identifier possible here is const.
                    // Change this once const becomes a dedicated token type.
                    comps.append({Component::Other, " const", nullptr});
                } else {
                    comps.append({Component::Other,
                                  String::format(" {}", get_punctuation_string(qual)),
                                  nullptr});
                }
            }
            prod = function->target;
        } else if (auto qualifier = prod->qualifier()) {
            if (starts_with_identifier(comps)) {
                comps.insert(0) = {Component::Other, " ", nullptr};
            }
            comps.insert(0) = {Component::KeywordOrIdentifier, qualifier->keyword,
                               nullptr};
            has_left_part = true;
            prod = qualifier->target;
        } else if (auto array_of = prod->array_of()) {
            // FIXME: sema does not support the array subscript yet
            parenthesize_if_needed();
            comps.append({Component::Other, "[]", nullptr});
            prod = array_of->target;
        } else {
            PLY_ASSERT(0);
        }
    }
    return std::move(comps);
}

Array<Stringifier::Component> Stringifier::to_string_comps(const QualifiedID& qid,
                                                           QIDRole role) {
    Array<Component> result;
    docs::SemaEntity* scope = this->from_scope;
    bool check_parents = true;
    if (role == QIDRole::RootDeclarator) {
        PLY_ASSERT(qid.nested_name.is_empty());
        if (this->prepend_class_name && scope &&
            scope->parent->type == docs::SemaEntity::Class) {
            result.append({Component::KeywordOrIdentifier, scope->parent->name});
            result.append({Component::Other, "::", nullptr});
        }
        result.append({Component::BeginRootDeclarator, {}, nullptr});
    }
    for (const NestedNameComponent& nested_comp : qid.nested_name) {
        if (auto identifier = nested_comp.identifier()) {
            if (scope) {
                scope = scope->lookup(identifier->name, check_parents);
            }
            result.append({Component::KeywordOrIdentifier, identifier->name, scope});
            result.append({Component::Other, "::", nullptr});
        } else if (auto templated = nested_comp.templated()) {
            if (scope) {
                scope = scope->lookup(identifier->name, check_parents);
            }
            result.append({Component::KeywordOrIdentifier, templated->name, scope});
            result.append({Component::Other, "<", nullptr});
            for (u32 i = 0; i < templated->args.num_items(); i++) {
                if (i > 0) {
                    result.append({Component::Other, ", ", nullptr});
                }
                const TemplateArg& arg = templated->args[i];
                result.move_extend(this->to_string_comps(arg.type));
            }
            result.append({Component::Other, ">::", nullptr});
        } else if (auto decl_type = nested_comp.decl_type()) {
            result.append({Component::KeywordOrIdentifier, "decltype", nullptr});
            result.append({Component::Other,
                           StringView{"("} + decl_type->expression + ")::", nullptr});
        } else {
            PLY_ASSERT(0);
        }
        check_parents = false;
    }
    if (auto identifier = qid.unqual.identifier()) {
        if (role == QIDRole::TypeSpecifier && scope) {
            scope = scope->lookup(identifier->name, check_parents);
        } else {
            scope = nullptr;
        }
        result.append({Component::KeywordOrIdentifier, identifier->name, scope});
    } else if (auto template_id = qid.unqual.template_id()) {
        if (role == QIDRole::TypeSpecifier && scope) {
            scope = scope->lookup(template_id->name, check_parents);
        } else {
            scope = nullptr;
        }
        result.append({Component::KeywordOrIdentifier, template_id->name, scope});
        if (role == QIDRole::RootDeclarator) {
            result.append({Component::EndRootDeclarator, {}, nullptr});
            role = QIDRole::Declarator;
        }
        result.append({Component::Other, "<", nullptr});
        for (u32 i = 0; i < template_id->args.num_items(); i++) {
            if (i > 0) {
                result.append({Component::Other, ", ", nullptr});
            }
            const TemplateArg& arg = template_id->args[i];
            result.move_extend(this->to_string_comps(arg.type));
        }
        result.append({Component::Other, ">", nullptr});
    } else if (auto decl_type = qid.unqual.decl_type()) {
        result.append({Component::KeywordOrIdentifier, "decltype", nullptr});
        result.append(
            {Component::Other, StringView{"("} + decl_type->expression + ")", nullptr});
    } else if (auto destructor = qid.unqual.destructor()) {
        result.append({Component::Other, "~", nullptr});
        result.append({Component::KeywordOrIdentifier, destructor->name, nullptr});
    } else if (auto operator_func = qid.unqual.operator_func()) {
        result.append({Component::KeywordOrIdentifier, "operator", nullptr});
        result.append(
            {Component::Other, get_punctuation_string(operator_func->punc), nullptr});
        if (operator_func->punc2 != cpp::Token::Invalid) {
            result.append({Component::Other,
                           get_punctuation_string(operator_func->punc2), nullptr});
        }
    } else if (auto conversion_func = qid.unqual.conversion_func()) {
        result.append({Component::KeywordOrIdentifier, "operator", nullptr});
        result.append({Component::Other, " ", nullptr});
        result.move_extend(this->to_string_comps(conversion_func->decl_specifier_seq,
                                                 conversion_func->abstract_dcor, {}, {},
                                                 false));
    }
    if (role == QIDRole::RootDeclarator) {
        result.append({Component::EndRootDeclarator, {}, nullptr});
    }
    return result;
}

Array<Stringifier::Component>
Stringifier::to_string_comps(ArrayView<const DeclSpecifier> decl_specifier_seq,
                             const DeclaratorProduction* prod, const QualifiedID& qid,
                             const Initializer& init, bool for_root_declaration) {
    Array<Component> comps;
    bool first = true;
    for (const DeclSpecifier& decl_spec : decl_specifier_seq) {
        if (!first) {
            comps.append({Component::Other, " ", nullptr});
        }
        first = false;
        if (auto keyword = decl_spec.keyword()) {
            comps.append({Component::KeywordOrIdentifier, keyword->token, nullptr});
        } else if (auto type_id = decl_spec.type_id()) {
            if (type_id->has_typename) {
                comps.append({Component::KeywordOrIdentifier, "typename", nullptr});
                comps.append({Component::Other, " ", nullptr});
            }
            comps.move_extend(
                this->to_string_comps(type_id->qid, QIDRole::TypeSpecifier));
        } else if (auto type_param = decl_spec.type_param()) {
            comps.append({Component::KeywordOrIdentifier, "typename", nullptr});
        } else {
            PLY_ASSERT(0);
        }
    }

    Array<Component> qid_comps;
    {
        // Don't try to lookup the declarator
        //        PLY_SET_IN_SCOPE(this->from_scope, nullptr);
        qid_comps = this->to_string_comps(
            qid, for_root_declaration ? QIDRole::RootDeclarator : QIDRole::Declarator);
    }
    Array<Component> dcor_comps = this->apply_productions(prod, std::move(qid_comps));
    if (dcor_comps && dcor_comps[0].type != Component::PointerOrReference) {
        comps.append({Component::Other, " ", nullptr});
    }
    comps.move_extend(dcor_comps);

    if (auto init_assign = init.assignment()) {
        comps.append({Component::Other, StringView{" = "}, nullptr});
        comps.move_extend(this->to_string_comps(init_assign->type));
    } else if (auto bit_field = init.bit_field()) {
        comps.append(
            {Component::Other, StringView{" : "} + bit_field->expression, nullptr});
    }

    return comps;
}

Array<Stringifier::Component> to_string_comps(const SingleDeclaration& single,
                                              docs::SemaEntity* from_scope,
                                              bool prepend_class_name) {
    Stringifier stringifier;
    stringifier.from_scope = from_scope;
    stringifier.prepend_class_name = prepend_class_name;
    return stringifier.to_string_comps(single, true);
}

String to_string(const SingleDeclaration& single) {
    MemOutStream mout;
    for (const Stringifier::Component& comp : to_string_comps(single, nullptr, false)) {
        mout << comp.text;
    }
    return mout.move_to_string();
}

} // namespace sema
} // namespace cpp
} // namespace ply
