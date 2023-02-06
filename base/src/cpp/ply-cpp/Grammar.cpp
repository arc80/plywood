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

StringView UnqualifiedID::get_ctor_dtor_name() const {
    if (auto id = this->identifier()) {
        return id->name.identifier;
    } else if (auto dtor = this->destructor()) {
        return dtor->name.identifier;
    } else if (auto tmpl = this->template_id()) {
        return tmpl->name.identifier; // FIXME: is this valid?
    }
    return {};
}

String QualifiedID::to_string() const {
    MemOutStream mout;

    for (const grammar::NestedNameComponent& comp : this->nested_name) {
        if (auto ident = comp.type.identifier_or_templated()) {
            mout << ident->name.identifier;
            if (ident->open_angled.is_valid()) {
                // FIXME: dump nested IDs/expressions
                mout << "<>";
            }
        } else if (auto decl_type = comp.type.decl_type()) {
            mout << "decltype()";
        } else {
            PLY_ASSERT(0);
        }
        PLY_ASSERT(comp.sep.is_valid());
        mout << "::";
    }

    using ID = grammar::UnqualifiedID::ID;
    switch (this->unqual.id) {
        case ID::Identifier: {
            mout << this->unqual.identifier()->name.identifier;
            break;
        }
        case ID::TemplateID: {
            auto tid = this->unqual.template_id();
            mout << tid->name.identifier << tid->open_angled.identifier;
            // FIXME: dump nested IDs/expressions
            mout << tid->close_angled.identifier;
            break;
        }
        case ID::DeclType: {
            // auto decl_type = this->unqual.decl_type();
            mout << "decltype()";
            break;
        }
        case ID::Destructor: {
            auto dtor = this->unqual.destructor();
            mout << dtor->tilde.identifier << dtor->name.identifier;
            break;
        }
        case ID::OperatorFunc: {
            auto op_func = this->unqual.operator_func();
            mout << op_func->keyword.identifier << op_func->punc.identifier
                 << op_func->punc2.identifier;
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

    return mout.move_to_string();
}

Token NestedNameComponent::get_first_token() const {
    switch (this->type.id) {
        using ID = NestedNameComponent::Type::ID;
        case ID::IdentifierOrTemplated: {
            return this->type.identifier_or_templated()->name;
        }
        case ID::DeclType: {
            return this->type.decl_type()->keyword;
        }
        default: {
            PLY_ASSERT(0);
            return {};
        }
    }
}

Token UnqualifiedID::get_first_token() const {
    switch (this->id) {
        using ID = UnqualifiedID::ID;
        case ID::Empty: {
            return {};
        }
        case ID::Identifier: {
            return this->identifier()->name;
        }
        case ID::TemplateID: {
            return this->template_id()->name;
        }
        case ID::DeclType: {
            return this->decl_type()->keyword;
        }
        case ID::Destructor: {
            return this->destructor()->tilde;
        }
        case ID::OperatorFunc: {
            return this->operator_func()->keyword;
        }
        case ID::ConversionFunc: {
            return this->conversion_func()->keyword;
        }
        default: {
            PLY_ASSERT(0);
            return {};
        }
    }
}

// Used when logging errors
Token QualifiedID::get_first_token() const {
    if (this->nested_name.num_items() > 0) {
        return this->nested_name[0].get_first_token();
    } else {
        return this->unqual.get_first_token();
    }
}

} // namespace grammar
} // namespace cpp
} // namespace ply

#include "codegen/Grammar.inl" //%%
