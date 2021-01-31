/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-web-cook-docs/Core.h>
#include <ply-cpp/Grammar.h>
#include <ply-web-cook-docs/SemaToString.h>

namespace ply {
namespace cpp {
namespace sema {

Array<Stringifier::Component> Stringifier::toStringComps(const TemplateArg::Type& templateArgType) {
    if (auto unknown = templateArgType.unknown()) {
        return {{Component::Other, unknown->expression, nullptr}};
    } else if (auto typeID = templateArgType.typeID()) {
        return this->toStringComps(typeID->declSpecifierSeq.view(), typeID->abstractDcor, {}, {},
                                   false);
    } else {
        PLY_ASSERT(0);
        return {};
    }
}

Array<Stringifier::Component> Stringifier::applyProductions(const DeclaratorProduction* prod,
                                                            Array<Stringifier::Component>&& comps) {
    auto startsWithIdentifier = [](ArrayView<Stringifier::Component> comps) {
        return comps.numItems >= 1 && (comps[0].type == Component::KeywordOrIdentifier ||
                                       comps[0].type == Component::BeginRootDeclarator);
    };
    bool hasLeftPart = false;
    auto parenthesizeIfNeeded = [&] {
        if (!hasLeftPart)
            return;
        comps.insert(0) = {Component::Other, "(", nullptr};
        comps.append({Component::Other, ")", nullptr});
        hasLeftPart = false;
    };
    while (prod) {
        if (auto pointerTo = prod->pointerTo()) {
            if (startsWithIdentifier(comps.view())) {
                comps.insert(0) = {Component::Other, " ", nullptr};
            }
            comps.insert(0) = {Component::PointerOrReference,
                               getPunctuationString(pointerTo->puncType), nullptr};
            hasLeftPart = true;
            prod = pointerTo->target;
        } else if (auto function = prod->function()) {
            parenthesizeIfNeeded();
            comps.append({Component::Other, "(", nullptr});
            for (u32 i = 0; i < function->params.numItems(); i++) {
                if (i > 0) {
                    comps.append({Component::Other, ", ", nullptr});
                }
                const SingleDeclaration& param = function->params[i];
                comps.moveExtend(this->toStringComps(param, false).view());
            }
            comps.append({Component::Other, ")", nullptr});
            for (cpp::Token::Type qual : function->qualifiers) {
                if (qual == cpp::Token::Identifier) {
                    // FIXME: We're assuming the only identifier possible here is const.
                    // Change this once const becomes a dedicated token type.
                    comps.append({Component::Other, " const", nullptr});
                } else {
                    comps.append({Component::Other,
                                  String::format(" {}", getPunctuationString(qual)), nullptr});
                }
            }
            prod = function->target;
        } else if (auto qualifier = prod->qualifier()) {
            if (startsWithIdentifier(comps.view())) {
                comps.insert(0) = {Component::Other, " ", nullptr};
            }
            comps.insert(0) = {Component::KeywordOrIdentifier, qualifier->keyword, nullptr};
            hasLeftPart = true;
            prod = qualifier->target;
        } else if (auto arrayOf = prod->arrayOf()) {
            // FIXME: sema does not support the array subscript yet
            parenthesizeIfNeeded();
            comps.append({Component::Other, "[]", nullptr});
            prod = arrayOf->target;
        } else {
            PLY_ASSERT(0);
        }
    }
    return std::move(comps);
}

Array<Stringifier::Component> Stringifier::toStringComps(const QualifiedID& qid, QIDRole role) {
    Array<Component> result;
    docs::SemaEntity* scope = this->fromScope;
    bool checkParents = true;
    if (role == QIDRole::RootDeclarator) {
        PLY_ASSERT(qid.nestedName.isEmpty());
        if (this->prependClassName && scope && scope->parent->type == docs::SemaEntity::Class) {
            result.append({Component::KeywordOrIdentifier, scope->parent->name});
            result.append({Component::Other, "::", nullptr});
        }
        result.append({Component::BeginRootDeclarator, {}, nullptr});
    }
    for (const NestedNameComponent& nestedComp : qid.nestedName) {
        if (auto identifier = nestedComp.identifier()) {
            if (scope) {
                scope = scope->lookup(identifier->name, checkParents);
            }
            result.append({Component::KeywordOrIdentifier, identifier->name, scope});
            result.append({Component::Other, "::", nullptr});
        } else if (auto templated = nestedComp.templated()) {
            if (scope) {
                scope = scope->lookup(identifier->name, checkParents);
            }
            result.append({Component::KeywordOrIdentifier, templated->name, scope});
            result.append({Component::Other, "<", nullptr});
            for (u32 i = 0; i < templated->args.numItems(); i++) {
                if (i > 0) {
                    result.append({Component::Other, ", ", nullptr});
                }
                const TemplateArg& arg = templated->args[i];
                result.moveExtend(this->toStringComps(arg.type).view());
            }
            result.append({Component::Other, ">::", nullptr});
        } else if (auto declType = nestedComp.declType()) {
            result.append({Component::KeywordOrIdentifier, "decltype", nullptr});
            result.append(
                {Component::Other, StringView{"("} + declType->expression + ")::", nullptr});
        } else {
            PLY_ASSERT(0);
        }
        checkParents = false;
    }
    if (auto identifier = qid.unqual.identifier()) {
        if (role == QIDRole::TypeSpecifier && scope) {
            scope = scope->lookup(identifier->name, checkParents);
        } else {
            scope = nullptr;
        }
        result.append({Component::KeywordOrIdentifier, identifier->name, scope});
    } else if (auto templateID = qid.unqual.templateID()) {
        if (role == QIDRole::TypeSpecifier && scope) {
            scope = scope->lookup(templateID->name, checkParents);
        } else {
            scope = nullptr;
        }
        result.append({Component::KeywordOrIdentifier, templateID->name, scope});
        if (role == QIDRole::RootDeclarator) {
            result.append({Component::EndRootDeclarator, {}, nullptr});
            role = QIDRole::Declarator;
        }
        result.append({Component::Other, "<", nullptr});
        for (u32 i = 0; i < templateID->args.numItems(); i++) {
            if (i > 0) {
                result.append({Component::Other, ", ", nullptr});
            }
            const TemplateArg& arg = templateID->args[i];
            result.moveExtend(this->toStringComps(arg.type).view());
        }
        result.append({Component::Other, ">", nullptr});
    } else if (auto declType = qid.unqual.declType()) {
        result.append({Component::KeywordOrIdentifier, "decltype", nullptr});
        result.append({Component::Other, StringView{"("} + declType->expression + ")", nullptr});
    } else if (auto destructor = qid.unqual.destructor()) {
        result.append({Component::Other, "~", nullptr});
        result.append({Component::KeywordOrIdentifier, destructor->name, nullptr});
    } else if (auto operatorFunc = qid.unqual.operatorFunc()) {
        result.append({Component::KeywordOrIdentifier, "operator", nullptr});
        result.append({Component::Other, getPunctuationString(operatorFunc->punc), nullptr});
        if (operatorFunc->punc2 != cpp::Token::Invalid) {
            result.append({Component::Other, getPunctuationString(operatorFunc->punc2), nullptr});
        }
    } else if (auto conversionFunc = qid.unqual.conversionFunc()) {
        result.append({Component::KeywordOrIdentifier, "operator", nullptr});
        result.append({Component::Other, " ", nullptr});
        result.moveExtend(this->toStringComps(conversionFunc->declSpecifierSeq.view(),
                                              conversionFunc->abstractDcor, {}, {}, false)
                              .view());
    }
    if (role == QIDRole::RootDeclarator) {
        result.append({Component::EndRootDeclarator, {}, nullptr});
    }
    return result;
}

Array<Stringifier::Component>
Stringifier::toStringComps(ArrayView<const DeclSpecifier> declSpecifierSeq,
                           const DeclaratorProduction* prod, const QualifiedID& qid,
                           const Initializer& init, bool forRootDeclaration) {
    Array<Component> comps;
    bool first = true;
    for (const DeclSpecifier& declSpec : declSpecifierSeq) {
        if (!first) {
            comps.append({Component::Other, " ", nullptr});
        }
        first = false;
        if (auto keyword = declSpec.keyword()) {
            comps.append({Component::KeywordOrIdentifier, keyword->token, nullptr});
        } else if (auto typeID = declSpec.typeID()) {
            if (typeID->hasTypename) {
                comps.append({Component::KeywordOrIdentifier, "typename", nullptr});
                comps.append({Component::Other, " ", nullptr});
            }
            comps.moveExtend(this->toStringComps(typeID->qid, QIDRole::TypeSpecifier).view());
        } else if (auto typeParam = declSpec.typeParam()) {
            comps.append({Component::KeywordOrIdentifier, "typename", nullptr});
        } else {
            PLY_ASSERT(0);
        }
    }

    Array<Component> qidComps;
    {
        // Don't try to lookup the declarator
        //        PLY_SET_IN_SCOPE(this->fromScope, nullptr);
        qidComps = this->toStringComps(qid, forRootDeclaration ? QIDRole::RootDeclarator
                                                               : QIDRole::Declarator);
    }
    Array<Component> dcorComps = this->applyProductions(prod, std::move(qidComps));
    if (dcorComps && dcorComps[0].type != Component::PointerOrReference) {
        comps.append({Component::Other, " ", nullptr});
    }
    comps.moveExtend(dcorComps.view());

    if (auto initAssign = init.assignment()) {
        comps.append({Component::Other, StringView{" = "}, nullptr});
        comps.moveExtend(this->toStringComps(initAssign->type).view());
    } else if (auto bitField = init.bitField()) {
        comps.append({Component::Other, StringView{" : "} + bitField->expression, nullptr});
    }

    return comps;
}

Array<Stringifier::Component> toStringComps(const SingleDeclaration& single,
                                            docs::SemaEntity* fromScope, bool prependClassName) {
    Stringifier stringifier;
    stringifier.fromScope = fromScope;
    stringifier.prependClassName = prependClassName;
    return stringifier.toStringComps(single, true);
}

String toString(const SingleDeclaration& single) {
    MemOutStream mout;
    for (const Stringifier::Component& comp : toStringComps(single, nullptr, false)) {
        mout << comp.text;
    }
    return mout.moveToString();
}

} // namespace sema
} // namespace cpp
} // namespace ply
