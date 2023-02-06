/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-reflect/Base.h>
#include <ply-runtime/Base.h>
#include <pylon/Parse.h>
#include <pylon/Write.h>
#include <pylon-reflect/Import.h>
#include <pylon-reflect/Export.h>
#include <ply-runtime/process/Subprocess.h>
#include <ply-runtime/io/text/LiquidTags.h>
#include <web-sass/Sass.h>
#include <ply-cook/CookJob.h>
#include <ply-web-cook-docs/SemaEntity.h>
#include <ply-web-cook-docs/WebCookerIndex.h>
#include <ply-web-cook-docs/CookResult_ExtractPageMeta.h>
#include <web-documentation/Contents.h>

namespace ply {
namespace docs {
extern cook::CookJobType CookJobType_CopyStatic;
extern cook::CookJobType CookJobType_ExtractAPI;
extern cook::CookJobType CookJobType_ExtractPageMeta;
extern cook::CookJobType CookJobType_Page;
extern cook::CookJobType CookJobType_StyleSheetID;
void init_cook_job_types();
} // namespace docs
} // namespace ply

using namespace ply;

Array<Reference<cook::CookJob>> copy_static_files(cook::CookContext* ctx,
                                                  StringView src_root) {
    Array<Reference<cook::CookJob>> copy_jobs;
    for (WalkTriple& triple : FileSystem.walk(src_root)) {
        for (const WalkTriple::FileInfo& file : triple.files) {
            String relative_dir = Path.make_relative(src_root, triple.dir_path);
            copy_jobs.append(ctx->cook({&ply::docs::CookJobType_CopyStatic,
                                        Path.join(relative_dir, file.name)}));
        }
    }
    return copy_jobs;
}

Array<String> get_source_file_keys(StringView src_root) {
    Array<String> src_keys;
    for (WalkTriple& triple : FileSystem.walk(src_root)) {
        for (const WalkTriple::FileInfo& file : triple.files) {
            if (file.name.ends_with(".cpp") || file.name.ends_with(".h")) {
                // FIXME: Eliminate exclusions
                for (StringView exclude : {
                         "Sort.h",
                         "Functor.h",
                         "DirectoryWatcher_Mac.h",
                         "DirectoryWatcher_Win32.h",
                         "Heap.cpp",
                         "Pool.h",
                     }) {
                    if (file.name == exclude)
                        goto skip_it;
                }
                {
                    String relative_dir =
                        Path.make_relative(Workspace.path, triple.dir_path);
                    src_keys.append(Path.join(relative_dir, file.name));
                }
            skip_it:;
            }
        }
        for (StringView exclude : {"Shell_iOS", "opengl-support"}) {
            s32 i = find(triple.dir_names, exclude);
            if (i >= 0) {
                triple.dir_names.erase(i);
            }
        }
    }
    return src_keys;
}

Reference<cook::CookJob> extract_page_metas_from_folder(cook::CookContext* ctx,
                                                        StringView rel_path) {
    PLY_ASSERT(rel_path.starts_with("/"));
    Reference<cook::CookJob> page_meta_job = ctx->dep_tracker->get_or_create_cook_job(
        {&docs::CookJobType_ExtractPageMeta, PosixPath.join(rel_path, "index")});
    Array<Reference<cook::CookJob>> child_jobs;
    String abs_path =
        Path.join(Workspace.path, "repos/plywood/docs", rel_path.sub_str(1));

    // By default, sort child pages by filename
    // The order can be overridden for each page using the <% child_order %> tag
    Array<DirectoryEntry> all_entries;
    for (const DirectoryEntry& entry : FileSystem.list_dir(abs_path)) {
        all_entries.append(entry);
    }
    sort(all_entries, [](const DirectoryEntry& a, const DirectoryEntry& b) {
        return a.name < b.name;
    });

    // Add child entries
    for (const DirectoryEntry& entry : all_entries) {
        if (entry.is_dir) {
            child_jobs.append(extract_page_metas_from_folder(
                ctx, PosixPath.join(rel_path, entry.name)));
        } else if (entry.name.ends_with(".md") && entry.name != "index.md") {
            StringView base_name = entry.name.shortened_by(3);
            child_jobs.append(ctx->cook({&docs::CookJobType_ExtractPageMeta,
                                         PosixPath.join(rel_path, base_name)}));
        }
    }

    ctx->ensure_cooked(page_meta_job, AnyObject::bind(&child_jobs));
    return page_meta_job;
}

void visit_page_metas(
    const cook::CookJob* page_meta_job,
    const Functor<void(const docs::CookResult_ExtractPageMeta*)>& visitor) {
    const docs::CookResult_ExtractPageMeta* page_meta_result =
        page_meta_job->cast_result<docs::CookResult_ExtractPageMeta>();
    PLY_ASSERT(page_meta_result);
    visitor(page_meta_result);
    for (const cook::CookJob* child_job : page_meta_result->child_pages) {
        visit_page_metas(child_job, visitor);
    }
}

Owned<web::Contents> convert_contents(const cook::CookJob* page_meta_job) {
    Owned<web::Contents> dst_node = new web::Contents;
    const docs::CookResult_ExtractPageMeta* page_meta_result =
        page_meta_job->cast_result<docs::CookResult_ExtractPageMeta>();
    PLY_ASSERT(page_meta_result);

    // Set title
    dst_node->title = page_meta_result->title;
    dst_node->link_destination = page_meta_result->get_link_destination();
    for (const cook::CookJob* child_meta_job : page_meta_result->child_pages) {
        dst_node->children.append(convert_contents(child_meta_job));
    }
    return dst_node;
}

int main() {
    ply::docs::init_cook_job_types();

    cook::DependencyTracker db;
    docs::WebCookerIndex* wci = new docs::WebCookerIndex;
    wci->global_scope = new docs::SemaEntity;
    db.user_data = AnyOwnedObject::bind(wci);
    // String db_path = Path.join(Workspace.path, "data/docsite-cache/depTracker.db");

    cook::CookContext ctx;
    ctx.dep_tracker = &db;
    ctx.begin_cook();

    // Copy static files
    Array<Reference<cook::CookJob>> copy_jobs = copy_static_files(
        &ctx, Path.join(Workspace.path, "repos/plywood/src/web/theme"));

    // Extract API documentation from the source code
    Array<Reference<cook::CookJob>> root_refs;
    Array<String> src_keys = get_source_file_keys(
        Path.join(Workspace.path, "repos/plywood/src/runtime/ply-runtime/io"));
    src_keys.extend(get_source_file_keys(
        Path.join(Workspace.path, "repos/plywood/src/runtime/ply-runtime/container")));
    src_keys.extend(get_source_file_keys(
        Path.join(Workspace.path, "repos/plywood/src/runtime/ply-runtime/string")));
    src_keys.extend(get_source_file_keys(
        Path.join(Workspace.path, "repos/plywood/src/runtime/ply-runtime/filesystem")));
    src_keys.extend(get_source_file_keys(
        Path.join(Workspace.path, "repos/plywood/src/math/math/ply-math")));
    for (StringView src_key : src_keys) {
        root_refs.append(ctx.cook({&ply::docs::CookJobType_ExtractAPI, src_key}));
    }

    // Extract page metas
    Reference<cook::CookJob> contents_root = extract_page_metas_from_folder(&ctx, "/");

    // Cook all pages
    visit_page_metas(contents_root,
                     [&](const docs::CookResult_ExtractPageMeta* page_meta_result) {
                         root_refs.append(ctx.cook({&ply::docs::CookJobType_Page,
                                                    page_meta_result->job->id.desc}));
                     });

    ctx.cook_deferred();

    // Save contents (FIXME: Skip this step if dependencies haven't changed)
    Array<Owned<web::Contents>> contents;
    {
        Owned<web::Contents> converted = convert_contents(contents_root);
        web::Contents* home = contents.append(new web::Contents);
        home->title = "Home";
        home->link_destination = "/";
        contents.move_extend(converted->children);
    }
    {
        auto a_root = pylon::export_obj(AnyObject::bind(&contents));
        MemOutStream mout;
        pylon::write(&mout, a_root);
        FileSystem.make_dirs_and_save_text_if_different(
            Path.join(Workspace.path, "data/docsite/contents.pylon"),
            mout.move_to_string(), TextFormat::unix_utf8());
    }

    // Cook stylesheet
    root_refs.append(ctx.cook({&ply::docs::CookJobType_StyleSheetID, {}}));

    db.set_root_references(std::move(root_refs));
    contents_root.clear();
    copy_jobs.clear();
    ctx.end_cook();
    return 0;
}
