/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-web-cook-docs/Core.h>
#include <ply-web-cook-docs/CookResult_Page.h>
#include <ply-web-cook-docs/CookResult_ExtractPageMeta.h>
#include <web-markdown/Markdown.h>
#include <ply-runtime/io/text/LiquidTags.h>
#include <ply-web-cook-docs/SemaToString.h>
#include <ply-runtime/io/text/FileLocationMap.h> // This should be moved to a different library

namespace ply {
namespace docs {

extern cook::CookJobType CookJobType_Page;

void visit_all(markdown::Node* node, const Functor<bool(markdown::Node*)>& callback) {
    if (callback(node)) {
        for (markdown::Node* child : node->children) {
            visit_all(child, callback);
        }
    }
}

SemaEntity* resolve_class_scope(StringView class_scope_text) {
    cook::CookContext* ctx = cook::CookContext::current();
    WebCookerIndex* wci = ctx->dep_tracker->user_data.safe_cast<WebCookerIndex>();
    Array<StringView> comps = class_scope_text.split_byte(':');
    return wci->global_scope->lookup_chain(comps);
}

String get_link_destination(const SemaEntity* target_sema) {
    cook::DependencyTracker* dep_tracker = cook::DependencyTracker::current();
    WebCookerIndex* wci = dep_tracker->user_data.safe_cast<WebCookerIndex>();
    String member_suffix;
    if (target_sema->type == SemaEntity::Member) {
        member_suffix = StringView{"#"} + target_sema->name;
        target_sema = target_sema->parent;
        if (target_sema->type != SemaEntity::Class)
            return {};
    }
    auto iter =
        wci->extract_page_meta.find_first_greater_or_equal_to(target_sema->name);
    for (; iter.is_valid() && iter.get_item()->sema_ent->name == target_sema->name;
         iter.next()) {
        if (iter.get_item()->sema_ent == target_sema) {
            return iter.get_item()->link_destination + member_suffix;
        }
    }
    return {};
}

HybridString wrap_with_link(StringView html, const SemaEntity* target_sema) {
    String link_destination = get_link_destination(target_sema);
    if (link_destination) {
        return String::format("<a href=\"{}\">{}</a>", fmt::XMLEscape{link_destination},
                              html);
    } else {
        return html;
    }
}

struct LookupContext {
    SemaEntity* for_class = nullptr;
    ArrayView<SemaEntity* const> sema_group;

    bool should_show_link(SemaEntity* sema_to_link) const {
        // Avoid unnecessary/unhelpful/"noisy" links
        if (sema_to_link == this->for_class)
            return false;
        for (SemaEntity* check_sema : this->sema_group) {
            if (sema_to_link == check_sema || sema_to_link->parent == check_sema)
                return false;
        }
        return true;
    }
};

String get_link_destination_from_span(StringView code_span_text,
                                      const LookupContext& lookup_ctx) {
    if (code_span_text.ends_with("()")) {
        code_span_text = code_span_text.shortened_by(2);
    }
    Array<StringView> name_comps = code_span_text.split_byte(':');
    if (name_comps.is_empty())
        return {};
    PLY_ASSERT(!code_span_text.starts_with(":")); // Not supported yet
    for (SemaEntity* from_sema : lookup_ctx.sema_group) {
        SemaEntity* found_sema = from_sema->lookup_chain(name_comps);
        if (found_sema) {
            if (!lookup_ctx.should_show_link(found_sema))
                return {};
            return get_link_destination(found_sema);
        }
    }
    if (lookup_ctx.for_class) {
        // Try class itself last
        SemaEntity* found_sema = lookup_ctx.for_class->lookup_chain(name_comps);
        if (found_sema) {
            if (!lookup_ctx.should_show_link(found_sema))
                return {};
            return get_link_destination(found_sema);
        }
    }
    // No potential link destination found
    return {};
}

Owned<markdown::Node> parse_markdown(StringView markdown,
                                     const LookupContext& lookup_ctx) {
    using namespace markdown;
    Owned<Node> document = parse(markdown);
    WebCookerIndex* wci =
        cook::DependencyTracker::current()->user_data.safe_cast<WebCookerIndex>();
    visit_all(document, [&](Node* node) -> bool {
        if (node->type == Node::Link) {
            // Fixup link destinations
            if (!node->text.starts_with("/") && node->text.find_byte(':') < 0) {
                s32 anchor_pos = node->text.find_byte('#');
                if (anchor_pos < 0) {
                    anchor_pos = node->text.num_bytes;
                }
                StringView link_id = node->text.left(anchor_pos);
                if (link_id) {
                    auto iter = wci->link_idmap.find_first_greater_or_equal_to(link_id);
                    if (iter.is_valid() && iter.get_item()->link_id == link_id) {
                        node->text = iter.get_item()->get_link_destination() +
                                     node->text.sub_str(anchor_pos);
                    }
                }
            }
            return false;
        } else if (node->type == Node::CodeSpan) {
            String link_destination =
                get_link_destination_from_span(node->text, lookup_ctx);
            if (!link_destination)
                return false;
            Node* parent = node->parent;
            s32 i = find(parent->children, node);
            PLY_ASSERT(i >= 0);
            Owned<Node> moved_node = std::move(parent->children[i]);
            PLY_ASSERT(moved_node == node);
            Owned<Node> link = new Node{nullptr, Node::Link};
            link->text = std::move(link_destination);
            link->parent = node;
            link->children.append(std::move(moved_node));
            parent->children[i] = std::move(link);
            node->parent = link;
            return false;
        } else if (node->type == Node::Heading) {
            if (node->children.num_items() == 1) {
                Node* link_node = node->children[0];
                if (link_node->type == Node::Link && link_node->text.starts_with("#")) {
                    // Convert this to an anchor
                    node->id = link_node->text.sub_str(1);
                    Array<Owned<Node>> promote_children =
                        std::move(link_node->children);
                    node->children = std::move(promote_children);
                }
            }
        }
        return true;
    });
    return document;
}

String convert_markdown_to_html(StringView markdown, const LookupContext& lookup_ctx) {
    Owned<markdown::Node> document = parse_markdown(markdown, lookup_ctx);
    MemOutStream mout;
    markdown::HTMLOptions options;
    options.child_anchors = true;
    convert_to_html(&mout, document, options);
    return mout.move_to_string();
}

void dump_member_title(const DocInfo::Entry::Title& title, OutStream& html_writer,
                       bool prepend_class_name, const LookupContext& lookup_ctx) {
    const SemaEntity* template_params = title.member->template_params;
    const cpp::sema::SingleDeclaration* single_decl = &title.member->single_decl;

    bool in_root_declarator = false;
    using C = cpp::sema::Stringifier::Component;
    if (template_params) {
        html_writer << "<code class=\"template\">template &lt;";
        bool first = true;
        for (const SemaEntity* param : template_params->child_seq) {
            if (!first) {
                html_writer << ", ";
            }
            first = false;
            for (const C& comp :
                 cpp::sema::to_string_comps(param->single_decl, nullptr, false)) {
                html_writer << fmt::XMLEscape{comp.text};
            }
        }
        html_writer << "&gt;</code><br>\n";
    }
    html_writer << "<code>";
    for (const C& comp :
         cpp::sema::to_string_comps(*single_decl, title.member, prepend_class_name)) {
        if (comp.type == C::BeginRootDeclarator) {
            html_writer << "<strong>";
            in_root_declarator = true;
        } else if (comp.type == C::EndRootDeclarator) {
            html_writer << "</strong>";
            in_root_declarator = false;
        } else {
            String link_destination;
            if (!in_root_declarator && comp.sema) {
                PLY_ASSERT(comp.type == C::KeywordOrIdentifier);
                if (lookup_ctx.should_show_link(comp.sema)) {
                    link_destination = get_link_destination(comp.sema);
                }
            }
            if (link_destination) {
                html_writer.format("<a href=\"{}\">", fmt::XMLEscape{link_destination});
            }
            html_writer << fmt::XMLEscape{comp.text};
            if (link_destination) {
                html_writer << "</a>";
            }
        }
    }
    html_writer << "</code>\n";
}

void dump_base_classes(OutStream& html_writer, SemaEntity* class_ent,
                       const LookupContext& lookup_ctx) {
    // Dump base classes
    for (const cpp::sema::QualifiedID& qid : class_ent->base_classes) {
        Array<StringView> comps = qid.get_simplified_components();
        SemaEntity* base_ent = class_ent->lookup_chain(comps);
        if (!base_ent)
            continue;
        if (!base_ent->doc_info)
            continue;
        if (!base_ent->doc_info->entries)
            continue;

        html_writer.format("<h2>Members Inherited From {}</h2>\n",
                           wrap_with_link(String::format("<code>{}</code>",
                                                         base_ent->get_qualified_id()),
                                          base_ent));
        html_writer << "<ul>\n";
        for (const DocInfo::Entry& entry : base_ent->doc_info->entries) {
            for (const DocInfo::Entry::Title& title : entry.titles) {
                html_writer << "<li>";
                dump_member_title(title, html_writer, true,
                                  {base_ent, lookup_ctx.sema_group});
                html_writer << "</li>\n";
            }
        }
        html_writer << "</ul>\n";

        dump_base_classes(html_writer, base_ent, {base_ent, lookup_ctx.sema_group});
    }
};

void dump_extracted_members(OutStream& html_writer, SemaEntity* class_ent) {
    PLY_ASSERT(class_ent);
    PLY_ASSERT(class_ent->type == SemaEntity::Class);
    const DocInfo* doc_info = class_ent->doc_info;
    PLY_ASSERT(doc_info);

    auto dump_member_entry = [&](const DocInfo::Entry& entry, bool prepend_class_name) {
        Array<SemaEntity*> sema_group;
        for (const DocInfo::Entry::Title& title : entry.titles) {
            sema_group.append(title.member);
        }
        LookupContext lookup_ctx{class_ent, sema_group};

        html_writer << "<dt>";
        for (const DocInfo::Entry::Title& title : entry.titles) {
            String anchor;
            String permalink;
            if (title.member->name) {
                html_writer.format(
                    "<div class=\"defTitle anchored\"><span class=\"anchor\" "
                    "id=\"{}\">&nbsp;</span>",
                    fmt::XMLEscape{title.member->name});
                /*
                permalink =
                    String::format(" <a class=\"headerlink\" href=\"#{}\"
                title=\"Permalink to " "this definition\">[link]</a>",
                                   fmt::XMLEscape{title.member->name});
                */
            } else {
                html_writer.format("<div class=\"defTitle\"{}>", anchor);
            }

            dump_member_title(title, html_writer, prepend_class_name, lookup_ctx);

            // Close <code> tag, write optional permalink & source code link, close
            // <div>
            PLY_ASSERT(title.src_path.starts_with(Path.normalize(Workspace.path)));
            String src_path = PosixPath.from<NativePath>(
                Path.make_relative(Workspace.path, title.src_path));
#if WEBCOOKDOCS_LINK_TO_GITHUB
            // FIXME: Link to a specific commit
            StringView src_link_prefix = "https://github.com/arc80/plywood/tree/main/";
#else
            StringView src_link_prefix = "/file/";
#endif
            String src_location = String::format(
                " <a class=\"headerlink\" href=\"{}{}#L{}\" "
                "title=\"Go to source code\">[code]</a>",
                src_link_prefix, fmt::XMLEscape{src_path}, title.line_number);
            html_writer.format("{}{}</div>\n", permalink, src_location);
        }
        html_writer << "</dt>\n";
        html_writer << "<dd>\n";
        html_writer << convert_markdown_to_html(entry.markdown_desc, lookup_ctx);
        html_writer << "</dd>\n";
    };

    // Dump data members
    bool wrote_section_header = false;
    for (const DocInfo::Entry& entry : doc_info->entries) {
        if (!entry.titles[0].member->is_function()) {
            if (!wrote_section_header) {
                html_writer << "<h2>Data Members</h2>\n";
                html_writer << "<dl>\n";
                wrote_section_header = true;
            }
            dump_member_entry(entry, false);
        }
    }
    if (wrote_section_header) {
        html_writer << "</dl>\n\n";
    }

    // Dump member functions in categories first
    for (u32 c = 0; c < doc_info->categories.num_items(); c++) {
        wrote_section_header = false;
        for (const DocInfo::Entry& entry : doc_info->entries) {
            if (entry.titles[0].member->is_function() &&
                entry.category_index == (s32) c) {
                if (!wrote_section_header) {
                    html_writer.format("<h2>{}</h2>\n", doc_info->categories[c].desc);
                    html_writer << "<dl>\n";
                    wrote_section_header = true;
                }
                dump_member_entry(entry, true);
            }
        }
    }

    // Dump uncategorized member functions
    wrote_section_header = false;
    for (const DocInfo::Entry& entry : doc_info->entries) {
        if (entry.titles[0].member->is_function() && entry.category_index < 0) {
            if (!wrote_section_header) {
                html_writer << "<h2>Member Functions</h2>\n";
                html_writer << "<dl>\n";
                wrote_section_header = true;
            }
            dump_member_entry(entry, true);
        }
    }
    if (wrote_section_header) {
        html_writer << "</dl>\n\n";
    }

    dump_base_classes(html_writer, class_ent, {class_ent, {}});
}

//---------------------------

u128 get_class_hash(StringView class_fqid) {
    cook::DependencyTracker* dep_tracker = cook::DependencyTracker::current();
    WebCookerIndex* wci = dep_tracker->user_data.cast<WebCookerIndex>();
    SemaEntity* class_ent = wci->global_scope->lookup_chain(class_fqid.split_byte(':'));
    if (!class_ent || class_ent->type != SemaEntity::Class)
        return {};
    return class_ent->hash;
}

extern cook::DependencyType DependencyType_ExtractedClassAPI;

struct Dependency_ExtractedClassAPI : cook::Dependency {
    String class_fqid;
    u128 class_hash;

    PLY_INLINE Dependency_ExtractedClassAPI(StringView class_fqid)
        : class_fqid{class_fqid} {
        this->class_hash = get_class_hash(class_fqid);
    }
};

cook::DependencyType DependencyType_ExtractedClassAPI{
    // has_changed
    [](cook::Dependency* dep_, cook::CookResult*, AnyObject) -> bool { //
        // FIXME: Implement safe cast
        Dependency_ExtractedClassAPI* dep_eca =
            static_cast<Dependency_ExtractedClassAPI*>(dep_);
        return dep_eca->class_hash != get_class_hash(dep_eca->class_fqid);
    },
};

//---------------------------

void Page_cook(cook::CookResult* cook_result_, AnyObject) {
    cook::CookContext* ctx = cook::CookContext::current();
    PLY_ASSERT(cook_result_->job->id.type == &CookJobType_Page);
    auto page_result = static_cast<CookResult_Page*>(cook_result_);

    CookResult_ExtractPageMeta* extract_meta_result =
        static_cast<CookResult_ExtractPageMeta*>(ctx->get_already_cooked_result(
            {&CookJobType_ExtractPageMeta, page_result->job->id.desc}));

    String page_src_path = extract_meta_result->get_markdown_path();
    Owned<InStream> ins = page_result->open_file_as_dependency(page_src_path);
    if (!ins) {
        // FIXME: Shouldn't create CookJobs for pages that don't exist
        if (Path.split(page_src_path).second != "index.md") {
            page_result->add_error(
                String::format("Unable to open \"{}\"\n", page_src_path));
        }
        return;
    }
    String src = FileIOWrappers::load_text_autodetect(std::move(ins)).first;
    FileLocationMap src_file_loc_map = FileLocationMap::from_view(src);
    ViewInStream src_vins{src};

    // Extract liquid tags
    MemOutStream mout;
    MemOutStream html_writer;
    Array<String> child_page_names;
    String class_scope_text;
    // FIXME: don't hardcode class_scope
    WebCookerIndex* wci = ctx->dep_tracker->user_data.safe_cast<WebCookerIndex>();
    SemaEntity* class_scope = wci->global_scope->lookup({"ply"});
    bool in_members = false;
    auto flush_markdown = [&] {
        String page = mout.move_to_string();
        html_writer << convert_markdown_to_html(page, {class_scope, {}});
        mout = MemOutStream{};
    };
    extract_liquid_tags(&mout, &src_vins, [&](StringView tag, StringView section) {
        ViewInStream vins{section};
        vins.parse<fmt::Whitespace>();
        StringView command = vins.read_view(fmt::Identifier{});
        vins.parse<fmt::Whitespace>();
        if (command == "title" || command == "linkID" || command == "synopsis" ||
            command == "childOrder") {
            // Handled by CookResult_ExtractPageMeta
        } else if (command == "note") {
            flush_markdown();
            html_writer << "<div class=\"note\"><img src=\"/static/info-icon.svg\" "
                           "class=\"icon\"/>\n";
            html_writer << convert_markdown_to_html(vins.view_available(),
                                                    {class_scope, {}});
            html_writer << "</div>\n";
        } else if (command == "member") {
            flush_markdown();
            if (!in_members) {
                html_writer << "<dl>\n";
                in_members = true;
            } else {
                html_writer << "</dd>\n";
            }
            html_writer << "<dt><code>";
            // FIXME: handle errors here
            Array<TitleSpan> spans =
                parse_title(vins.view_available().rtrim(is_white),
                            [](ParseTitleError, StringView, const char*) {});
            write_alt_member_title(html_writer, spans, {class_scope, {}},
                                   get_link_destination_from_span);
            html_writer << "</code></dt>\n";
            html_writer << "<dd>\n";
        } else if (command == "endMembers") {
            if (in_members) {
                flush_markdown();
                html_writer << "</dd>\n";
                html_writer << "</dl>\n";
                in_members = false;
            } else {
                PLY_ASSERT(0); // FIXME: Handle gracefully
            }
        } else if (command == "setClassScope") {
            class_scope_text = vins.view_available().trim(is_white);
            class_scope = resolve_class_scope(class_scope_text);
            if (!class_scope) {
                FileLocation src_file_loc = src_file_loc_map.get_file_location(
                    check_cast<u32>(tag.bytes - src.bytes));
                page_result->add_error(
                    String::format("{}({}, {}): error: class '{}' not found\n",
                                   page_src_path, src_file_loc.line_number,
                                   src_file_loc.column_number, class_scope_text));
            } else {
                if (class_scope->doc_info->class_markdown_desc) {
                    html_writer << convert_markdown_to_html(
                        class_scope->doc_info->class_markdown_desc, {class_scope, {}});
                }
            }
        } else if (command == "dumpExtractedMembers") {
            flush_markdown();
            String class_fqid = vins.view_available().trim(is_white);
            if (class_fqid != class_scope_text) {
                FileLocation src_file_loc = src_file_loc_map.get_file_location(
                    check_cast<u32>(tag.bytes - src.bytes));
                page_result->add_error(String::format(
                    "{}({}, {}): error: dumpExtractedMembers tag '{}' does not "
                    "match earlier setClassScope tag '{}'\n",
                    page_src_path, src_file_loc.line_number, src_file_loc.column_number,
                    class_fqid, class_scope_text));
            }
            page_result->dependencies.append(
                new Dependency_ExtractedClassAPI{class_fqid});
            SemaEntity* class_ent =
                wci->global_scope->lookup_chain(class_fqid.split_byte(':'));
            if (!class_ent) {
                // FIXME: It would be cool to set the column_number to the exact
                // location of the class name within the liquid tag, but that will
                // require a way to map offsets in the liquid tag to offsets in the file
                // that accounts for escape characters. (Although there aren't any
                // escape chracters in liquid tags yet...) For now, we'll just use the
                // location of the tag itself.
                FileLocation src_file_loc = src_file_loc_map.get_file_location(
                    check_cast<u32>(tag.bytes - src.bytes));
                page_result->add_error(String::format(
                    "{}({}, {}): error: class '{}' not found\n", page_src_path,
                    src_file_loc.line_number, src_file_loc.column_number, class_fqid));
                html_writer.format(
                    "<div class=\"error\">Error: Class \"{}\" not found</div>",
                    fmt::XMLEscape{class_fqid});
                return;
            }
            dump_extracted_members(html_writer, class_ent);
        } else if (command == "html") {
            flush_markdown();
            html_writer << vins.view_available();
        } else {
            FileLocation src_file_loc = src_file_loc_map.get_file_location(
                check_cast<u32>(tag.bytes - src.bytes));
            page_result->add_error(String::format(
                "{}({}, {}): error: unrecognized tag '{}'\n", page_src_path,
                src_file_loc.line_number, src_file_loc.column_number, command));
        }
    });

    // Save page html with single line title header
    // FIXME: Implement strategy to delete orphaned HTML files
    flush_markdown();
    String final_html = String::format("{}\n", extract_meta_result->title) +
                        html_writer.move_to_string();
    PLY_ASSERT(page_result->job->id.desc.starts_with("/"));
    String html_path = Path.join(Workspace.path, "data/docsite/pages",
                                 page_result->job->id.desc.sub_str(1) + ".html");
    FileSystem.make_dirs_and_save_text_if_different(html_path, final_html,
                                                    TextFormat::unix_utf8());
}

cook::CookJobType CookJobType_Page = {
    "page",
    get_type_descriptor<CookResult_Page>(),
    nullptr,
    Page_cook,
};

} // namespace docs
} // namespace ply

#include "codegen/CookResult_Page.inl" //%%
