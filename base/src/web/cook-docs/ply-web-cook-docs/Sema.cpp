/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-web-cook-docs/Core.h>
#include <ply-cpp/Grammar.h>
#include <ply-cpp/PPVisitedFiles.h>
#include <ply-web-cook-docs/Sema.h>

namespace ply {
namespace cpp {

// This is temporary code until the C++ parser parses expressions correctly
String temp_extract_initializer(const PPVisitedFiles* visited_files,
                                LinearLocation start_loc, LinearLocation end_loc) {
    if (!visited_files || start_loc < 0 || end_loc < 0)
        return {};
    // Note: These iterators may not be usable if the initializer began or ended with a
    // macro. What we really want is the topmost item on the include chain that is not a
    // macro.
    auto start_iter = visited_files->location_map.find_last_less_than(start_loc + 1);
    auto end_iter = visited_files->location_map.find_last_less_than(end_loc + 1);
    if (start_iter.get_item().include_chain_idx !=
        end_iter.get_item().include_chain_idx)
        return {};
    const cpp::PPVisitedFiles::IncludeChain& chain =
        visited_files->include_chains[start_iter.get_item().include_chain_idx];
    if (chain.is_macro_expansion)
        return {};
    const cpp::PPVisitedFiles::SourceFile* src_file =
        &visited_files->source_files[chain.file_or_exp_idx];
    PLY_ASSERT(start_loc >= start_iter.get_item().linear_loc);
    PLY_ASSERT(end_loc >= end_iter.get_item().linear_loc);
    u32 start_pos = check_cast<u32>(start_iter.get_item().offset +
                                     (start_loc - start_iter.get_item().linear_loc));
    u32 end_pos = check_cast<u32>(end_iter.get_item().offset +
                                   (end_loc - end_iter.get_item().linear_loc));
    return src_file->contents.sub_str(start_pos, end_pos - start_pos);
}

struct SemaConverter {
    const PPVisitedFiles* visited_files = nullptr;
    bool any_error = false;

    PLY_NO_INLINE sema::TemplateArg
    to_sema(const grammar::TemplateArgumentWithComma& g_arg) {
        sema::TemplateArg s_arg;
        if (auto g_type_id = g_arg.type.type_id()) {
            auto s_type_id = s_arg.type.type_id().switch_to();
            s_type_id->decl_specifier_seq =
                this->to_sema(g_type_id->decl_specifier_seq);
            s_type_id->abstract_dcor = this->to_sema(g_type_id->abstract_dcor);
        } else if (auto g_unknown = g_arg.type.unknown()) {
            auto s_unknown = s_arg.type.unknown();
            s_unknown->expression = temp_extract_initializer(
                this->visited_files, g_unknown->start_token.linear_loc,
                g_unknown->start_token.linear_loc +
                    g_unknown->start_token.identifier.num_bytes);
        } else {
            this->any_error = true;
        }
        return s_arg;
    }

    PLY_NO_INLINE sema::QualifiedID to_sema(const grammar::QualifiedID& g_qid) {
        sema::QualifiedID s_qid;
        for (const grammar::NestedNameComponent& g_nested_comp : g_qid.nested_name) {
            if (auto g_ident_or_templ = g_nested_comp.type.identifier_or_templated()) {
                if (g_ident_or_templ->open_angled.is_valid()) {
                    auto s_templated =
                        s_qid.nested_name.append().templated().switch_to();
                    s_templated->name = g_ident_or_templ->name.identifier;
                    for (const grammar::TemplateArgumentWithComma& g_template_arg :
                         g_ident_or_templ->args) {
                        s_templated->args.append(this->to_sema(g_template_arg));
                    }
                } else {
                    auto s_identifier =
                        s_qid.nested_name.append().identifier().switch_to();
                    s_identifier->name = g_ident_or_templ->name.identifier;
                }
            } else if (auto g_decl_type = g_nested_comp.type.decl_type()) {
                auto s_decl_type = s_qid.nested_name.append().decl_type().switch_to();
                s_decl_type->expression = temp_extract_initializer(
                    this->visited_files,
                    g_decl_type->open_paren.linear_loc +
                        g_decl_type->open_paren.identifier.num_bytes,
                    g_decl_type->close_paren.linear_loc);
            } else {
                this->any_error = true;
            }
        }
        if (auto g_identifier = g_qid.unqual.identifier()) {
            auto s_identifier = s_qid.unqual.identifier().switch_to();
            s_identifier->name = g_identifier->name.identifier;
        } else if (auto g_template_id = g_qid.unqual.template_id()) {
            auto s_template_id = s_qid.unqual.template_id().switch_to();
            s_template_id->name = g_template_id->name.identifier;
            for (const grammar::TemplateArgumentWithComma& g_arg :
                 g_template_id->args) {
                s_template_id->args.append(this->to_sema(g_arg));
            }
        } else if (auto g_decl_type = g_qid.unqual.decl_type()) {
            auto s_decl_type = s_qid.unqual.decl_type().switch_to();
            s_decl_type->expression = temp_extract_initializer(
                this->visited_files,
                g_decl_type->open_paren.linear_loc +
                    g_decl_type->open_paren.identifier.num_bytes,
                g_decl_type->close_paren.linear_loc);
        } else if (auto g_destructor = g_qid.unqual.destructor()) {
            auto s_destructor = s_qid.unqual.destructor().switch_to();
            s_destructor->name = g_destructor->name.identifier;
        } else if (auto g_operator_func = g_qid.unqual.operator_func()) {
            auto s_operator_func = s_qid.unqual.operator_func().switch_to();
            s_operator_func->punc = g_operator_func->punc.type;
            s_operator_func->punc2 = g_operator_func->punc2.type;
        } else if (auto g_conversion_func = g_qid.unqual.conversion_func()) {
            auto s_conversion_func = s_qid.unqual.conversion_func().switch_to();
            s_conversion_func->decl_specifier_seq =
                this->to_sema(g_conversion_func->decl_specifier_seq);
            s_conversion_func->abstract_dcor =
                this->to_sema(g_conversion_func->abstract_dcor);
        }
        return s_qid;
    }

    PLY_NO_INLINE Array<sema::DeclSpecifier>
    to_sema(ArrayView<const Owned<grammar::DeclSpecifier>> g_decl_specifier_seq) {
        Array<sema::DeclSpecifier> s_decl_specs;
        for (const grammar::DeclSpecifier* g_decl_spec : g_decl_specifier_seq) {
            if (auto g_keyword = g_decl_spec->keyword()) {
                auto s_keyword = s_decl_specs.append().keyword().switch_to();
                s_keyword->token = g_keyword->token.identifier;
            } else if (auto g_type_id = g_decl_spec->type_id()) {
                auto s_type_id = s_decl_specs.append().type_id().switch_to();
                s_type_id->has_typename = g_type_id->typename_.is_valid();
                s_type_id->was_assumed = g_type_id->was_assumed;
                s_type_id->qid = this->to_sema(g_type_id->qid);
            } else if (auto g_type_param = g_decl_spec->type_param()) {
                auto s_type_param = s_decl_specs.append().type_param().switch_to();
                s_type_param->has_ellipsis = g_type_param->ellipsis.is_valid();
            } else {
                this->any_error = true;
            }
        }
        return s_decl_specs;
    }

    PLY_NO_INLINE sema::SingleDeclaration
    to_sema(const grammar::ParamDeclarationWithComma& g_param) {
        sema::SingleDeclaration s_single;
        s_single.decl_specifier_seq = this->to_sema(g_param.decl_specifier_seq);
        s_single.dcor = this->to_sema(g_param.dcor);
        if (auto g_assignment = g_param.init.assignment()) {
            auto s_assignment = s_single.init.assignment().switch_to();
            if (auto g_expression = g_assignment->type.expression()) {
                s_assignment->type.unknown().switch_to()->expression =
                    temp_extract_initializer(
                        this->visited_files, g_expression->start.linear_loc,
                        g_expression->end.linear_loc +
                            g_expression->end.identifier.num_bytes);
            } else if (auto g_type_id = g_assignment->type.type_id()) {
                auto s_type_id = s_assignment->type.type_id().switch_to();
                s_type_id->decl_specifier_seq =
                    this->to_sema(g_type_id->decl_specifier_seq);
                s_type_id->abstract_dcor = this->to_sema(g_type_id->abstract_dcor);
            } else {
                this->any_error = true;
            }
        }
        return s_single;
    }

    PLY_NO_INLINE Owned<sema::DeclaratorProduction>
    to_sema(const grammar::DeclaratorProduction* g_dcor) {
        if (!g_dcor)
            return nullptr;
        auto s_prod = Owned<sema::DeclaratorProduction>::create();
        if (auto g_pointer_to = g_dcor->type.pointer_to()) {
            auto s_pointer_to = s_prod->pointer_to().switch_to();
            s_pointer_to->punc_type = g_pointer_to->punc.type;
            s_pointer_to->target = this->to_sema(g_dcor->target);
        } else if (auto g_function = g_dcor->type.function()) {
            auto s_function = s_prod->function().switch_to();
            s_function->target = this->to_sema(g_dcor->target);
            for (const grammar::ParamDeclarationWithComma& g_param :
                 g_function->params.params) {
                s_function->params.append(this->to_sema(g_param));
            }
            for (const Token& qual_token : g_function->qualifiers.tokens) {
                s_function->qualifiers.append(qual_token.type);
            }
        } else if (auto g_qualifier = g_dcor->type.qualifier()) {
            auto s_qualifier = s_prod->qualifier().switch_to();
            s_qualifier->keyword = g_qualifier->keyword.identifier;
            s_qualifier->target = this->to_sema(g_dcor->target);
        } else if (auto g_array_of = g_dcor->type.array_of()) {
            auto s_array_of = s_prod->array_of().switch_to();
            s_array_of->target = this->to_sema(g_dcor->target);
        } else if (auto g_parenthesized = g_dcor->type.parenthesized()) {
            s_prod = this->to_sema(g_dcor->target);
        } else {
            this->any_error = true;
        }
        return s_prod;
    }

    PLY_NO_INLINE sema::Declarator to_sema(const grammar::Declarator& g_dcor) {
        return {this->to_sema(g_dcor.prod), this->to_sema(g_dcor.qid)};
    }

    PLY_NO_INLINE Array<sema::SingleDeclaration>
    to_sema(const grammar::Declaration::Simple& g_simple) {
        Array<sema::SingleDeclaration> s_singles;
        for (const grammar::InitDeclaratorWithComma& g_init_dcor :
             g_simple.init_declarators) {
            sema::SingleDeclaration& s_single = s_singles.append();
            s_single.decl_specifier_seq = this->to_sema(g_simple.decl_specifier_seq);
            s_single.dcor = this->to_sema(g_init_dcor.dcor);
            if (auto bit_field = g_init_dcor.init.bit_field()) {
                s_single.init.bit_field().switch_to()->expression =
                    temp_extract_initializer(
                        this->visited_files, bit_field->expression_start.linear_loc,
                        bit_field->expression_end.linear_loc +
                            bit_field->expression_end.identifier.num_bytes);
            }
        }
        return s_singles;
    }
};

Array<sema::SingleDeclaration>
sema_from_parse_tree(const grammar::Declaration::Simple& g_simple,
                     const PPVisitedFiles* visited_files) {
    SemaConverter conv;
    conv.visited_files = visited_files;
    Array<sema::SingleDeclaration> result = conv.to_sema(g_simple);
    if (conv.any_error)
        return {};
    return result;
}

sema::SingleDeclaration
sema_from_param(const grammar::ParamDeclarationWithComma& g_param,
                const PPVisitedFiles* visited_files) {
    SemaConverter conv;
    conv.visited_files = visited_files;
    sema::SingleDeclaration result = conv.to_sema(g_param);
    if (conv.any_error)
        return {};
    return result;
}

sema::QualifiedID sema_from_qid(const grammar::QualifiedID& g_qid,
                                const PPVisitedFiles* visited_files) {
    SemaConverter conv;
    conv.visited_files = visited_files;
    sema::QualifiedID result = conv.to_sema(g_qid);
    if (conv.any_error)
        return {};
    return result;
}

Array<StringView> sema::QualifiedID::get_simplified_components() const {
    Array<StringView> result;
    for (const NestedNameComponent& comp : this->nested_name) {
        if (auto ident = comp.identifier()) {
            result.append(ident->name);
        } else if (auto templated = comp.templated()) {
            result.append(templated->name);
        } else {
            return {};
        }
    }
    if (auto ident = this->unqual.identifier()) {
        result.append(ident->name);
    } else if (auto templated = this->unqual.template_id()) {
        result.append(templated->name);
    } else {
        return {};
    }
    return result;
}

} // namespace cpp
} // namespace ply

#include "codegen/Sema.inl" //%%
