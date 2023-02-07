/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-web-cook-docs/Core.h>
#include <ply-cook/CookJob.h>
#include <crowbar-client/CrowbarClient.h>
#include <ply-cpp/Parser.h>
#include <ply-cpp/ParseAPI.h>
#include <ply-cpp/ErrorFormatting.h>
#include <ply-web-cook-docs/SemaEntity.h>
#include <ply-web-cook-docs/WebCookerIndex.h>

namespace ply {
namespace cpp {
void parse_plywood_src_file(StringView abs_src_path, cpp::PPVisitedFiles* visited_files,
                            ParseSupervisor* visor);
Array<sema::SingleDeclaration>
sema_from_parse_tree(const grammar::Declaration::Simple& g_simple,
                     const cpp::PPVisitedFiles* visited_files);
sema::SingleDeclaration
sema_from_param(const grammar::ParamDeclarationWithComma& g_param,
                const cpp::PPVisitedFiles* visited_files);
sema::QualifiedID sema_from_qid(const grammar::QualifiedID& g_qid,
                                const cpp::PPVisitedFiles* visited_files);
} // namespace cpp

namespace docs {

extern cook::CookJobType CookJobType_ExtractAPI;

// Fundamentally, the CookResult_ExtractAPI is the owner of all class/member
// SemaEntities namespace SemaEntities are owned indirectly via reference counting
struct CookResult_ExtractAPI : cook::CookResult {
    PLY_REFLECT();
    // ply reflect off
    Array<Reference<SemaEntity>> extracted_classes;
    Array<Reference<SemaEntity>> extracted_at_namespace_scope;
};

struct APIExtractor : cpp::ParseSupervisor {
    struct Error : cpp::BaseError {
        enum Type {
            // ply reflect enum
            Unknown = 0,
            AlreadyDefined,
            StrayMarkdown,
            BadIndentInDocumentationComment,
            NestedNamesNotSupported, // yet
            DirectivesMustBeAtStartOfDocumentationComment,
            EmptyDirective,
            BadDirective,
            DirectiveDoesNotTakeArguments,
            AlreadyInsideGroup,
            UnterminatedGroup,
            EndGroupOutsideGroup,
            GroupHasNoMarkdown,
            StrayMarkdownInGroup,
            EmptyDocumentationComment,
            DirectiveOnlyValidWithinClass,
            MarkdownOnlyValidWithinClass,
            DirectiveNotValidWithinClass,
            ExpectedClassName,
            UnexpectedAfterClassName,
            ClassNotFound,
            BeginParseTitleError,
            EndParseTitleError =
                BeginParseTitleError + (u32) ParseTitleError::NumErrors,
        };

        PLY_REFLECT();
        Type type = Unknown;
        String arg;
        cpp::LinearLocation linear_loc = -1;
        // ply reflect off

        Error(Type type, String arg, cpp::LinearLocation linear_loc)
            : type{type}, arg{arg}, linear_loc{linear_loc} {
        }
        virtual void
        write_message(OutStream* outs,
                      const cpp::PPVisitedFiles* visited_files) const override;
    };

    struct DocState {
        String markdown;
        cpp::LinearLocation markdown_loc = -1;
        cpp::LinearLocation group_directive_loc = -1;
        DocInfo::Entry* group_entry = nullptr;
        s32 category_index = -1;
        SemaEntity* add_to_class = nullptr;
    };

    Array<Reference<SemaEntity>> sema_scope_stack;
    CookResult_ExtractAPI* extract_apiresult = nullptr;
    Array<Owned<cpp::BaseError>>* errors = nullptr;
    DocState doc_state;

    void error(Error::Type type, StringView arg, cpp::LinearLocation linear_loc) {
        this->parser->pp->error_handler(new Error{type, arg, linear_loc});
    }

    struct ScopeInfo {
        s32 parent_idx = -1;
        s32 template_idx = -1;
        SemaEntity* parent_scope = nullptr;
    };

    ScopeInfo get_scope_info(s32 parent_idx) {
        PLY_ASSERT(this->scope_stack.num_items() == this->sema_scope_stack.num_items());
        // Get template & parent indices
        ScopeInfo scope_info;
        if (parent_idx >= 0) {
            scope_info.parent_idx = parent_idx;
            if (this->sema_scope_stack[scope_info.parent_idx]) {
                while (this->scope_stack[scope_info.parent_idx]
                           .safe_cast<cpp::grammar::Declaration::Template_>()) {
                    if (scope_info.template_idx < 0) {
                        scope_info.template_idx = scope_info.parent_idx;
                    }
                    scope_info.parent_idx--;
                }
                scope_info.parent_scope = this->sema_scope_stack[scope_info.parent_idx];
            }
        }
        return scope_info;
    }

    virtual void enter(AnyObject node) override {
        if (node.safe_cast<cpp::grammar::TranslationUnit>()) {
            PLY_ASSERT(this->sema_scope_stack.num_items() == 1);
            PLY_ASSERT(this->sema_scope_stack[0]);
            return;
        }

        bool check_doc_state = true;
        PLY_ON_SCOPE_EXIT({
            if (check_doc_state) {
                if (this->doc_state.group_directive_loc >= 0) {
                    this->error(Error::UnterminatedGroup, {},
                                this->doc_state.group_directive_loc);
                    this->doc_state.group_directive_loc = -1;
                    this->doc_state.group_entry = nullptr;
                }
                if (this->doc_state.markdown_loc >= 0) {
                    this->error(Error::StrayMarkdown, {}, this->doc_state.markdown_loc);
                    this->doc_state.markdown = {};
                    this->doc_state.markdown_loc = -1;
                }
                this->doc_state.category_index = -1;
                this->doc_state.add_to_class = nullptr;
            }
        });

        Reference<SemaEntity>& cur_stack_entry = this->sema_scope_stack.append();
        ScopeInfo scope_info =
            this->get_scope_info(this->sema_scope_stack.num_items() - 2);
        if (!scope_info.parent_scope)
            return; // Early out if no parent

        if (auto* record = node.safe_cast<cpp::grammar::DeclSpecifier::Record>()) {
            if (record->qid.nested_name.num_items() > 0) {
                // It's a class/struct/union declaration with a nested name specifier
                return;
            }
            auto ident = record->qid.unqual.identifier();
            if (!ident) {
                // It's an unnamed class or union
                return;
            }
            for (auto iter =
                     scope_info.parent_scope->name_to_child
                         .find_first_greater_or_equal_to(ident->name.identifier);
                 iter.is_valid() && iter.get_item()->name == ident->name.identifier;
                 iter.next()) {
                this->error(Error::AlreadyDefined, ident->name.identifier,
                            record->qid.get_first_token().linear_loc);
                // Create the class_ent anyway
            }
            Reference<SemaEntity> class_ent = new SemaEntity;
            class_ent->set_parent(scope_info.parent_scope);
            class_ent->type = SemaEntity::Class;
            class_ent->name = ident->name.identifier;
            class_ent->doc_info = new DocInfo;
            class_ent->doc_info->class_ = class_ent;
            if (this->doc_state.markdown_loc >= 0) {
                class_ent->doc_info->class_markdown_desc =
                    std::move(this->doc_state.markdown);
                this->doc_state.markdown_loc = -1;
            }
            for (const cpp::grammar::BaseSpecifierWithComma& base :
                 record->base_specifier_list) {
                cpp::sema::QualifiedID qid =
                    cpp::sema_from_qid(base.base_qid, this->parser->pp->visited_files);
                if (!qid.is_empty()) {
                    class_ent->base_classes.append(std::move(qid));
                }
            }

            scope_info.parent_scope->name_to_child.insert(class_ent);
            if (scope_info.parent_scope->type == SemaEntity::Namespace) {
                // FIXME: Only save the class_ent if it has docstrings (?)
                this->extract_apiresult->extracted_classes.append(class_ent);
            } else {
                PLY_ASSERT(scope_info.parent_scope->type == SemaEntity::Class);
                scope_info.parent_scope->child_seq.append(class_ent);
            }
            cur_stack_entry = std::move(class_ent);
            return;
        } else if (auto* ns_ =
                       node.safe_cast<cpp::grammar::Declaration::Namespace_>()) {
            if (ns_->qid.nested_name.num_items() > 0) {
                this->error(Error::Unknown, {},
                            record->qid.get_first_token().linear_loc);
                return;
            }
            auto ident = ns_->qid.unqual.identifier();
            if (!ident) {
                this->error(Error::Unknown, {},
                            record->qid.get_first_token().linear_loc);
                return;
            }
            for (auto iter =
                     scope_info.parent_scope->name_to_child
                         .find_first_greater_or_equal_to(ident->name.identifier);
                 iter.is_valid() && iter.get_item()->name == ident->name.identifier;
                 iter.next()) {
                if (iter.get_item()->type == SemaEntity::Namespace) {
                    // re-entering the same namespace
                    cur_stack_entry = iter.get_item();
                    return;
                } else {
                    this->error(Error::AlreadyDefined, ident->name.identifier,
                                record->qid.get_first_token().linear_loc);
                    // FIXME: Should create the namespace anyway?
                    // This matters in order to get incremental cooking working
                    return;
                }
            }
            // Add new namespace to outer scope
            Reference<SemaEntity> namespace_ent = new SemaEntity;
            namespace_ent->set_parent(scope_info.parent_scope);
            namespace_ent->type = SemaEntity::Namespace;
            namespace_ent->name = ident->name.identifier;
            scope_info.parent_scope->name_to_child.insert(namespace_ent);
            cur_stack_entry = std::move(namespace_ent);
            return;
        } else if (auto* template_ =
                       node.safe_cast<cpp::grammar::Declaration::Template_>()) {
            check_doc_state = false;
            Reference<SemaEntity> template_params = new SemaEntity;
            template_params->type = SemaEntity::TemplateParamList;
            for (const cpp::grammar::ParamDeclarationWithComma& g_param :
                 template_->params.params) {
                Reference<SemaEntity> s_param = new SemaEntity;
                s_param->type = SemaEntity::TemplateParam;
                if (auto ident = g_param.dcor.qid.unqual.identifier()) {
                    s_param->name = ident->name.identifier;
                }
                s_param->single_decl =
                    cpp::sema_from_param(g_param, this->parser->pp->visited_files);
                template_params->child_seq.append(s_param);
                if (s_param->name) {
                    template_params->name_to_child.insert(s_param);
                }
            }
            cur_stack_entry = template_params;
            return;
        } else if (node.is<cpp::grammar::Declaration::Simple>()) {
            // Will be handled in on_got_declaration
            check_doc_state = false;
            return;
        }
    }

    virtual void exit(AnyObject node) override {
        if (node.safe_cast<cpp::grammar::DeclSpecifier::Record>() ||
            node.safe_cast<cpp::grammar::Declaration::Namespace_>()) {
            if (this->doc_state.markdown_loc >= 0) {
                this->error(Error::StrayMarkdown, {}, this->doc_state.markdown_loc);
                this->doc_state.markdown = {};
                this->doc_state.markdown_loc = -1;
            }
            if (this->doc_state.group_directive_loc >= 0) {
                this->error(Error::UnterminatedGroup, {},
                            this->doc_state.group_directive_loc);
                this->doc_state.group_directive_loc = -1;
                this->doc_state.group_entry = nullptr;
            }
            this->doc_state.category_index = -1;
            this->doc_state.add_to_class = nullptr;
        }
        this->sema_scope_stack.pop();
    }

    void parse_doc_string(const cpp::Token& token) {
        ScopeInfo scope_info = this->get_scope_info(this->scope_stack.num_items() - 1);

        // Figure out indent
        cpp::ExpandedFileLocation exp = cpp::expand_file_location(
            this->parser->pp->visited_files, token.linear_loc);
        u32 indent = exp.file_loc.column_number;
        MemOutStream mout;
        PLY_ASSERT(token.identifier.starts_with("/*!"));
        PLY_ASSERT(token.identifier.ends_with("*/"));
        const char* cur_byte = token.identifier.bytes + 3;
        const char* end_byte = token.identifier.end() - 2;
        bool skip_indentation = false;
        while (cur_byte < end_byte) {
            char c = *cur_byte;
            if (!is_white(c))
                break;
            cur_byte++;
            if (c == '\n') {
                skip_indentation = true;
                break;
            }
        }

        // Loop over each line
        bool got_non_group_directive = false;
        bool got_any_markdown = false;
        bool log_indentation_error = true;
        for (;;) {
            // Skip indentation on new lines
            u32 column = 1;
            if (skip_indentation) {
                for (;;) {
                    if (cur_byte >= end_byte)
                        goto end_of_comment;
                    char c = *cur_byte++;
                    if (c == '\n') {
                        if (mout.get_seek_pos() > 0) {
                            // Add blank line to the markdown
                            mout << c;
                            column = 1;
                        }
                    } else if (c == ' ') {
                        column++;
                    } else if (c == '\t') {
                        u32 tab_size = 4; // FIXME: Make configurable
                        column += tab_size - (column % tab_size);
                    } else {
                        // Got non-whitespace
                        cur_byte--;
                        break;
                    }
                }
            } else {
                column = indent;
                skip_indentation = true;
            }

            // There is text on this line
            cpp::LinearLocation line_loc =
                token.linear_loc + (cur_byte - token.identifier.bytes);
            if (log_indentation_error && column < indent) {
                this->error(Error::BadIndentInDocumentationComment, {}, line_loc);
                log_indentation_error = false;
            }

            // Get remaining part of the line
            const char* end_of_line = cur_byte;
            while (end_of_line < end_byte) {
                if (*end_of_line++ == '\n')
                    break;
            }
            StringView line = StringView::from_range(cur_byte, end_of_line);
            cur_byte = end_of_line;

            // Check for directives
            if (line.starts_with("\\")) {
                // This is a directive
                if (mout.get_seek_pos() > 0) {
                    // Finish reading markdown
                    this->doc_state.markdown = mout.move_to_string();
                    mout = {};
                }
                ViewInStream dr{line.sub_str(1)};
                StringView directive = dr.read_view<fmt::Identifier>();
                dr.parse<fmt::Whitespace>();
                if (directive != "beginGroup") {
                    got_non_group_directive = true;
                }
                if (!directive) {
                    this->error(Error::EmptyDirective, {}, line_loc);
                } else if (directive == "beginGroup") {
                    // \begin_group directive
                    if (dr.view_available().trim(is_white)) {
                        this->error(Error::DirectiveDoesNotTakeArguments, directive,
                                    line_loc);
                    }
                    if (this->doc_state.group_directive_loc >= 0) {
                        this->error(Error::AlreadyInsideGroup, {}, line_loc);
                        this->error(Error::UnterminatedGroup, {},
                                    this->doc_state.group_directive_loc);
                        this->doc_state.group_directive_loc = -1;
                        this->doc_state.group_entry = nullptr;
                    }
                    if ((scope_info.parent_scope &&
                         scope_info.parent_scope->type == SemaEntity::Class) ||
                        this->doc_state.add_to_class) {
                        this->doc_state.group_directive_loc = line_loc;
                    } else {
                        this->error(Error::DirectiveOnlyValidWithinClass, directive,
                                    line_loc);
                    }
                } else if (directive == "endGroup") {
                    // \end_group directive
                    if (dr.view_available().trim(is_white)) {
                        this->error(Error::DirectiveDoesNotTakeArguments, directive,
                                    line_loc);
                    }
                    if (this->doc_state.group_directive_loc < 0) {
                        this->error(Error::EndGroupOutsideGroup, directive, line_loc);
                    }
                    this->doc_state.group_directive_loc = -1;
                    this->doc_state.group_entry = nullptr;
                } else if (directive == "category") {
                    if ((scope_info.parent_scope &&
                         scope_info.parent_scope->type == SemaEntity::Class) ||
                        this->doc_state.add_to_class) {
                        SemaEntity* for_class = this->doc_state.add_to_class;
                        if (!this->doc_state.add_to_class) {
                            for_class = scope_info.parent_scope;
                        }
                        StringView category_desc = dr.view_available().trim(is_white);
                        s32 category_index = find(for_class->doc_info->categories,
                                                  [&](const DocInfo::Category& cat) {
                                                      return cat.desc == category_desc;
                                                  });
                        if (category_index < 0) {
                            category_index =
                                for_class->doc_info->categories.num_items();
                            for_class->doc_info->categories.append(category_desc);
                        }
                        this->doc_state.category_index = category_index;
                    } else {
                        this->error(Error::DirectiveOnlyValidWithinClass, directive,
                                    line_loc);
                    }
                } else if (directive == "addToClass") {
                    // There are serious limitations on \add_to_class right now.
                    if (scope_info.parent_scope &&
                        scope_info.parent_scope->type == SemaEntity::Class) {
                        this->error(Error::DirectiveNotValidWithinClass, directive,
                                    line_loc);
                    } else {
                        StringView class_name = dr.read_view<fmt::Identifier>();
                        if (class_name.is_empty()) {
                            this->error(Error::ExpectedClassName, directive, line_loc);
                        } else {
                            if (dr.view_available().trim(is_white)) {
                                this->error(Error::UnexpectedAfterClassName, directive,
                                            line_loc);
                            }
                            s32 i = find(this->extract_apiresult->extracted_classes,
                                         [&](const SemaEntity* ent) {
                                             return ent->name == class_name;
                                         });
                            if (i >= 0) {
                                this->doc_state.add_to_class =
                                    this->extract_apiresult->extracted_classes[i];
                            } else {
                                this->error(Error::ClassNotFound, class_name, line_loc);
                            }
                        }
                    }
                } else {
                    this->error(Error::BadDirective, directive, line_loc);
                }
            } else {
                // Not a directive; treat as markdown
                got_any_markdown = true;
                if (mout.get_seek_pos() == 0) {
                    if (this->doc_state.group_entry) {
                        this->error(Error::StrayMarkdownInGroup, {}, line_loc);
                    } else {
                        if (this->doc_state.markdown_loc >= 0) {
                            this->error(Error::StrayMarkdown, {},
                                        this->doc_state.markdown_loc);
                        }
                        this->doc_state.markdown.clear();
                        this->doc_state.markdown_loc = line_loc;
                    }
                }
                for (u32 i = indent; i < column; i++) {
                    mout << ' ';
                }
                mout << line;
            }
        }
    end_of_comment:
        if (mout.get_seek_pos() > 0) {
            PLY_ASSERT(!this->doc_state.markdown);
            PLY_ASSERT(this->doc_state.markdown_loc >= 0);
            this->doc_state.markdown = mout.move_to_string();
        }
        if (!got_any_markdown && !got_non_group_directive) {
            // Allow empty markdown in member documentation (eg. Float2::x and y).
            // FIXME: Would be better to store the LinearLocation of the start of the
            // last line of the documentation string.
            this->doc_state.markdown_loc = token.linear_loc;
            PLY_ASSERT(!this->doc_state.markdown);
        }
    }

    virtual void on_got_declaration(const cpp::grammar::Declaration& decl) override {
        PLY_ON_SCOPE_EXIT({
            if (this->doc_state.markdown_loc >= 0) {
                this->error(Error::StrayMarkdown, {}, this->doc_state.markdown_loc);
                this->doc_state.markdown = {};
                this->doc_state.markdown_loc = -1;
            }
        });

        ScopeInfo scope_info = this->get_scope_info(this->scope_stack.num_items() - 1);
        if (this->doc_state.group_entry) {
            if ((!scope_info.parent_scope ||
                 scope_info.parent_scope->type != SemaEntity::Class) &&
                !this->doc_state.add_to_class)
                return;
        } else if (this->doc_state.markdown_loc >= 0) {
            if ((!scope_info.parent_scope ||
                 scope_info.parent_scope->type != SemaEntity::Class) &&
                !this->doc_state.add_to_class) {
                this->error(Error::MarkdownOnlyValidWithinClass, {},
                            this->doc_state.markdown_loc);
                this->doc_state.markdown = {};
                this->doc_state.markdown_loc = -1;
                return;
            }
        } else {
            // Skip this declaration
            return;
        }
        PLY_ASSERT((scope_info.parent_scope->type == SemaEntity::Class) ||
                   this->doc_state.add_to_class);

        if (auto simple = decl.simple()) {
            Array<cpp::sema::SingleDeclaration> sema_decls =
                ply::cpp::sema_from_parse_tree(*simple.get(),
                                               this->parser->pp->visited_files);
            PLY_ASSERT(sema_decls.num_items() == simple->init_declarators.num_items());
            for (u32 i = 0; i < sema_decls.num_items(); i++) {
                cpp::sema::SingleDeclaration& sema_decl = sema_decls[i];
                if (sema_decl.dcor.qid.nested_name.num_items() > 0) {
                    const cpp::grammar::QualifiedID& bad_qid =
                        simple->init_declarators[i].dcor.qid;
                    this->error(Error::NestedNamesNotSupported, bad_qid.to_string(),
                                bad_qid.get_first_token().linear_loc);
                    continue;
                }
                HybridString indexed_name;
                if (auto ident = sema_decl.dcor.qid.unqual.identifier()) {
                    indexed_name = ident->name;
                }
                Reference<SemaEntity> member_ent = new SemaEntity;
                member_ent->set_parent(scope_info.parent_scope);
                member_ent->type = SemaEntity::Member;
                member_ent->name = indexed_name;
                if (scope_info.template_idx >= 0) {
                    member_ent->template_params =
                        this->sema_scope_stack[scope_info.template_idx];
                }
                if (scope_info.parent_scope->type == SemaEntity::Class) {
                    // Don't add ctors to the name_to_child map:
                    if (member_ent->name != member_ent->parent->name) {
                        scope_info.parent_scope->name_to_child.insert(member_ent);
                    }
                    scope_info.parent_scope->child_seq.append(member_ent);
                } else {
                    scope_info.parent_scope->name_to_child.insert(member_ent);
                    this->extract_apiresult->extracted_at_namespace_scope.append(
                        member_ent);
                }

                // Add function parameters as child SemaEntities
                if (sema_decl.dcor.prod) {
                    auto func = sema_decl.dcor.prod->function();
                    if (func) {
                        for (const cpp::sema::SingleDeclaration& param : func->params) {
                            PLY_ASSERT(param.dcor.qid.nested_name.is_empty());
                            PLY_ASSERT(param.dcor.qid.unqual.identifier());
                            Reference<SemaEntity> s_param = new SemaEntity;
                            s_param->parent = member_ent;
                            s_param->type = SemaEntity::FunctionParam;
                            s_param->name = param.dcor.qid.unqual.identifier()->name;
                            member_ent->child_seq.append(s_param);
                            member_ent->name_to_child.insert(s_param);
                        }
                    }
                }
                member_ent->single_decl = std::move(sema_decl);

                DocInfo* doc_info = nullptr;
                if (this->doc_state.add_to_class) {
                    doc_info = this->doc_state.add_to_class->doc_info;
                } else {
                    doc_info = scope_info.parent_scope->doc_info;
                    PLY_ASSERT(doc_info->class_ == scope_info.parent_scope);
                }

                DocInfo::Entry* entry = this->doc_state.group_entry;
                if (!entry) {
                    entry = &doc_info->entries.append();
                    if (this->doc_state.group_directive_loc >= 0) {
                        this->doc_state.group_entry = entry;
                        if (this->doc_state.markdown_loc < 0) {
                            this->error(Error::GroupHasNoMarkdown, {},
                                        this->doc_state.group_directive_loc);
                        } else {
                            entry->markdown_desc = std::move(this->doc_state.markdown);
                            this->doc_state.markdown_loc = -1;
                        }
                    } else {
                        PLY_ASSERT(this->doc_state.markdown_loc >= 0);
                        entry->markdown_desc = std::move(this->doc_state.markdown);
                        this->doc_state.markdown_loc = -1;
                    }
                    entry->category_index = this->doc_state.category_index;
                }

                DocInfo::Entry::Title* title = &entry->titles.append();
                title->member = member_ent;
                // Set file and line number
                // FIXME: If the member is declared inside a macro or macros, get the
                // file and line containing the macro(s) instead
                cpp::ExpandedFileLocation file_loc = cpp::expand_file_location(
                    this->parser->pp->visited_files,
                    simple->init_declarators[i].dcor.qid.get_first_token().linear_loc);
                title->src_path = file_loc.src_file->abs_path;
                title->line_number = file_loc.file_loc.line_number;
            }
        }
    }

    virtual void got_macro_or_comment(cpp::Token token) override {
        if (token.type == cpp::Token::CStyleComment &&
            token.identifier.starts_with("/*!")) {
            this->parse_doc_string(token);
        }
    }

    virtual bool handle_error(Owned<cpp::BaseError>&& err) override {
        MemOutStream mout;
        err->write_message(&mout, this->parser->pp->visited_files);
        this->extract_apiresult->add_error(mout.move_to_string());
        return true;
    }
};
PLY_DECLARE_TYPE_DESCRIPTOR(APIExtractor::Error::Type)

void APIExtractor::Error::write_message(
    OutStream* outs, const cpp::PPVisitedFiles* visited_files) const {
    if (this->linear_loc >= 0) {
        outs->format("{}: ",
                     expand_file_location(visited_files, this->linear_loc).to_string());
    }
    *outs << "error: ";
    if (this->type >= APIExtractor::Error::BeginParseTitleError &&
        this->type < APIExtractor::Error::EndParseTitleError) {
        write_parse_title_error(
            outs,
            (ParseTitleError) (this->type - APIExtractor::Error::BeginParseTitleError),
            this->arg);
    } else {
        switch (this->type) {
            case APIExtractor::Error::AlreadyDefined: {
                outs->format("'{}' already defined\n", this->arg);
                break;
            }
            case APIExtractor::Error::StrayMarkdown: {
                outs->format("markdown text must be followed by a declaration\n");
                break;
            }
            case APIExtractor::Error::NestedNamesNotSupported: {
                outs->format("documentation comment on nested name \"{}\" is not "
                             "currently supported\n",
                             this->arg);
                break;
            }
            case APIExtractor::Error::DirectivesMustBeAtStartOfDocumentationComment: {
                *outs << "directives must be at start of documentation comment\n";
                break;
            }
            case APIExtractor::Error::EmptyDirective: {
                *outs << "expected directive after '\\'\n";
                break;
            }
            case APIExtractor::Error::BadDirective: {
                outs->format("unrecognized directive '\\{}'\n", this->arg);
                break;
            }
            case APIExtractor::Error::DirectiveDoesNotTakeArguments: {
                outs->format("\\{} directive does not accept any arguments\n",
                             this->arg);
                break;
            }
            case APIExtractor::Error::UnterminatedGroup: {
                *outs << "unterminated \\beginGroup directive\n";
                break;
            }
            case APIExtractor::Error::AlreadyInsideGroup: {
                *outs << "already insde a \\beginGroup directive\n";
                break;
            }
            case APIExtractor::Error::EndGroupOutsideGroup: {
                *outs << "\\endGroup must be preceded by a \\beginGroup directive\n";
                break;
            }
            case APIExtractor::Error::GroupHasNoMarkdown: {
                *outs << "\\beginGroup directive without markdown description\n";
                break;
            }
            case APIExtractor::Error::StrayMarkdownInGroup: {
                // FIXME: Log the location of the opening \\begin_group block as
                // additional information
                *outs << "illegal markdown inside \\beginGroup block\n";
                break;
            }
            case APIExtractor::Error::EmptyDocumentationComment: {
                *outs << "empty documentation comment\n";
                break;
            }
            case APIExtractor::Error::DirectiveOnlyValidWithinClass: {
                outs->format("\\{} directive is only allowed inside a class\n",
                             this->arg);
                break;
            }
            case APIExtractor::Error::MarkdownOnlyValidWithinClass: {
                *outs << "markdown is only supported for classes and class members\n";
                break;
            }
            case APIExtractor::Error::DirectiveNotValidWithinClass: {
                outs->format("\\{} directive not allowed inside a class\n", this->arg);
                break;
            }
            case APIExtractor::Error::ExpectedClassName: {
                outs->format("expected class name after \\{} directive\n", this->arg);
                break;
            }
            case APIExtractor::Error::UnexpectedAfterClassName: {
                outs->format("unexpected text after class name in \\{} directive\n",
                             this->arg);
                break;
            }
            case APIExtractor::Error::ClassNotFound: {
                outs->format("class '{}' not found\n", this->arg);
                break;
            }
            default: {
                *outs << "error message not implemented!\n";
                break;
            }
        }
    }
}

void ExtractAPI_cook(cook::CookResult* cook_result_, AnyObject) {
    cook::DependencyTracker* dep_tracker = cook::DependencyTracker::current();
    WebCookerIndex* user_data = dep_tracker->user_data.cast<WebCookerIndex>();
    PLY_ASSERT(user_data->global_scope);

    // FIXME: implement safe cast
    PLY_ASSERT(cook_result_->job->id.type == &CookJobType_ExtractAPI);
    CookResult_ExtractAPI* extract_apiresult =
        static_cast<CookResult_ExtractAPI*>(cook_result_);

    APIExtractor visor;
    visor.sema_scope_stack.append(user_data->global_scope);
    visor.extract_apiresult = extract_apiresult;

    cpp::PPVisitedFiles visited_files;
    cpp::parse_plywood_src_file(
        Path.join(Workspace.path, extract_apiresult->job->id.desc), &visited_files,
        &visor);
}

cook::CookJobType CookJobType_ExtractAPI = {
    "extractAPI",
    get_type_descriptor<CookResult_ExtractAPI>(),
    nullptr,
    ExtractAPI_cook,
};

} // namespace docs
} // namespace ply

#include "codegen/CookResult_ExtractAPI.inl" //%%
