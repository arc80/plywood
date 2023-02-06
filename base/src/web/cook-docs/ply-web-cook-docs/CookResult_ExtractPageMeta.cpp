/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-web-cook-docs/Core.h>
#include <ply-web-cook-docs/CookResult_ExtractPageMeta.h>
#include <ply-runtime/io/text/LiquidTags.h>
#include <ply-web-cook-docs/WebCookerIndex.h>

namespace ply {
namespace docs {

extern cook::CookJobType CookJobType_ExtractPageMeta;

void cook_ExtractPageMeta(cook::CookResult* cook_result_, AnyObject job_arg) {
    cook::CookContext* ctx = cook::CookContext::current();
    WebCookerIndex* wci = ctx->dep_tracker->user_data.safe_cast<WebCookerIndex>();
    PLY_ASSERT(cook_result_->job->id.type == &CookJobType_ExtractPageMeta);
    auto extract_page_meta_result =
        static_cast<CookResult_ExtractPageMeta*>(cook_result_);

    // Take ownership of child page list
    // FIXME: Add dependency on this list!
    auto* child_pages_in_folder = job_arg.cast<Array<Reference<cook::CookJob>>>();
    if (child_pages_in_folder) {
        for (cook::CookJob* job : *child_pages_in_folder) {
            PLY_ASSERT(job->cast_result<CookResult_ExtractPageMeta>());
            PLY_UNUSED(job);
        }
        extract_page_meta_result->child_pages = std::move(*child_pages_in_folder);
    }

    {
        // Set default page title && link_id
        auto split_path = PosixPath.split(cook_result_->job->id.desc);
        if (split_path.second == "index") {
            extract_page_meta_result->title = PosixPath.split(split_path.first).second;
        } else {
            extract_page_meta_result->title = split_path.second;
        }
        extract_page_meta_result->link_id = extract_page_meta_result->title;
    }

    Owned<InStream> ins = extract_page_meta_result->open_file_as_dependency(
        extract_page_meta_result->get_markdown_path());
    if (ins) {
        extract_page_meta_result->markdown_exists = true;

        String src = FileIOWrappers::load_text_autodetect(std::move(ins)).first;
        ViewInStream ins{src};

        // Extract liquid tags
        MemOutStream mout; // Note: This could be some kind of "null writer" if such a
                           // thing existed
        extract_liquid_tags(&mout, &ins, [&](StringView tag, StringView section) {
            ViewInStream vins{section};
            vins.parse<fmt::Whitespace>();
            StringView command = vins.read_view(fmt::Identifier{});
            vins.parse<fmt::Whitespace>();
            if (command == "title") {
                extract_page_meta_result->title = vins.parse(fmt::QuotedString{});
            } else if (command == "linkID") {
                extract_page_meta_result->link_id =
                    vins.view_available().rtrim(is_white);
            } else if (command == "childOrder") {
                // Override child page order
                Array<Reference<cook::CookJob>> orig_order =
                    std::move(extract_page_meta_result->child_pages);
                for (;;) {
                    vins.parse<fmt::Whitespace>();
                    StringView child_id = vins.read_view(fmt::Identifier{});
                    if (!child_id)
                        break;
                    s32 child_index = find(orig_order, [&](const cook::CookJob* child) {
                        return child->cast_result<CookResult_ExtractPageMeta>()
                                   ->link_id == child_id;
                    });
                    if (child_index < 0) {
                        // FIXME: Dump file/line number
                        extract_page_meta_result->add_error(
                            String::format("'{}' is not a child page\n", child_id));
                    } else {
                        extract_page_meta_result->child_pages.append(
                            std::move(orig_order[child_index]));
                        orig_order.erase_quick(child_index);
                    }
                }
                if (orig_order.num_items() > 0) {
                    // FIXME: Dump file/line number
                    Array<String> child_ids;
                    for (const cook::CookJob* child : orig_order) {
                        child_ids.append(String::format(
                            "'{}'",
                            child->cast_result<CookResult_ExtractPageMeta>()->link_id));
                    }
                    extract_page_meta_result->add_error(String::format(
                        "child pages not listed: {}",
                        StringView{", "}.join(Array<StringView>{child_ids})));
                }
            } else if (command == "dumpExtractedMembers") {
                String class_fqid = vins.view_available().trim(is_white);
                Array<StringView> components = class_fqid.split_byte(':');
                SemaEntity* ent = wci->global_scope->lookup_chain(components);
                if (!ent)
                    return; // Ignore here; error will be logged by CookResult_Page
                auto pair = Owned<SymbolPagePair>::create();
                pair->sema_ent = ent;
                pair->link_destination =
                    extract_page_meta_result->get_link_destination();
                pair->add_to_index();
                extract_page_meta_result->symbol_page_pairs.append(std::move(pair));
            } else if (command == "synopsis") {
                extract_page_meta_result->synopsis = vins.view_available();
            }
        });
    }

    // FIXME: Check for duplicate link_ids
    wci->link_idmap.insert(extract_page_meta_result);
}

cook::CookJobType CookJobType_ExtractPageMeta = {
    "extractPageMeta",
    get_type_descriptor<CookResult_ExtractPageMeta>(),
    get_type_descriptor<Array<Reference<cook::CookJob>>>(),
    cook_ExtractPageMeta,
};

} // namespace docs
} // namespace ply

#include "codegen/CookResult_ExtractPageMeta.inl" //%%
