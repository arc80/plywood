/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-cpp/Core.h>
#include <ply-cpp/Grammar.h>

namespace ply {
namespace cpp {
namespace grammar {

StringView UnqualifiedID::getCtorDtorName() const {
    if (auto id = this->identifier()) {
        return id->name.identifier;
    } else if (auto dtor = this->destructor()) {
        return dtor->name.identifier;
    } else if (auto tmpl = this->templateID()) {
        return tmpl->name.identifier; // FIXME: is this valid?
    }
    return {};
}

String QualifiedID::toString() const {
    MemOutStream mout;

    for (const grammar::NestedNameComponent& comp : this->nestedName) {
        if (auto ident = comp.type.identifierOrTemplated()) {
            mout << ident->name.identifier;
            if (ident->openAngled.isValid()) {
                // FIXME: dump nested IDs/expressions
                mout << "<>";
            }
        } else if (auto declType = comp.type.declType()) {
            mout << "decltype()";
        } else {
            PLY_ASSERT(0);
        }
        PLY_ASSERT(comp.sep.isValid());
        mout << "::";
    }

    using ID = grammar::UnqualifiedID::ID;
    switch (this->unqual.id) {
        case ID::Identifier: {
            mout << this->unqual.identifier()->name.identifier;
            break;
        }
        case ID::TemplateID: {
            auto tid = this->unqual.templateID();
            mout << tid->name.identifier << tid->openAngled.identifier;
            // FIXME: dump nested IDs/expressions
            mout << tid->closeAngled.identifier;
            break;
        }
        case ID::DeclType: {
            // auto declType = this->unqual.declType();
            mout << "decltype()";
            break;
        }
        case ID::Destructor: {
            auto dtor = this->unqual.destructor();
            mout << dtor->tilde.identifier << dtor->name.identifier;
            break;
        }
        case ID::OperatorFunc: {
            auto opFunc = this->unqual.operatorFunc();
            mout << opFunc->keyword.identifier << opFunc->punc.identifier << opFunc->punc2.identifier;
            break;
        }
        case ID::ConversionFunc: {
            // FIXME: improve this
            mout << "(conversion)";
            break;
        }
        case ID::Empty: {
            mout << "(empty)";
            break;
        }
        default: {
            PLY_ASSERT(0);
            break;
        }
    }

    return mout.moveToString();
}

Token NestedNameComponent::getFirstToken() const {
    switch (this->type.id) {
        using ID = NestedNameComponent::Type::ID;
        case ID::IdentifierOrTemplated: {
            return this->type.identifierOrTemplated()->name;
        }
        case ID::DeclType: {
            return this->type.declType()->keyword;
        }
        default: {
            PLY_ASSERT(0);
            return {};
        }
    }
}

Token UnqualifiedID::getFirstToken() const {
    switch (this->id) {
        using ID = UnqualifiedID::ID;
        case ID::Empty: {
            return {};
        }
        case ID::Identifier: {
            return this->identifier()->name;
        }
        case ID::TemplateID: {
            return this->templateID()->name;
        }
        case ID::DeclType: {
            return this->declType()->keyword;
        }
        case ID::Destructor: {
            return this->destructor()->tilde;
        }
        case ID::OperatorFunc: {
            return this->operatorFunc()->keyword;
        }
        case ID::ConversionFunc: {
            return this->conversionFunc()->keyword;
        }
        default: {
            PLY_ASSERT(0);
            return {};
        }
    }
}

// Used when logging errors
Token QualifiedID::getFirstToken() const {
    if (this->nestedName.numItems() > 0) {
        return this->nestedName[0].getFirstToken();
    } else {
        return this->unqual.getFirstToken();
    }
}

} // namespace grammar
} // namespace cpp
} // namespace ply

#include "codegen/Grammar.inl" //%%
