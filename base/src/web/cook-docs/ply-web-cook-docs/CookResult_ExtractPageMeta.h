/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-web-cook-docs/Core.h>
#include <ply-cook/CookJob.h>

namespace ply {
namespace docs {

struct SymbolPagePair;

extern cook::CookJobType CookJobType_ExtractPageMeta;

struct CookResult_ExtractPageMeta : cook::CookResult {
    PLY_REFLECT()
    bool markdown_exists = false;
    String link_id;
    String title;
    Array<Reference<cook::CookJob>> child_pages;
    String synopsis;
    Array<Owned<SymbolPagePair>> symbol_page_pairs;
    // ply reflect off

    PLY_INLINE String get_link_destination() const {
        PLY_ASSERT(this->job->id.desc.starts_with("/"));
        if (!markdown_exists)
            return {};
        if (this->job->id.desc.ends_with("/index")) {
            if (this->job->id.desc == "/index") {
                return "/";
            } else {
                return StringView{"/docs"} + this->job->id.desc.shortened_by(6);
            }
        } else {
            return StringView{"/docs"} + this->job->id.desc;
        }
    }
    PLY_INLINE String get_markdown_path() const {
        PLY_ASSERT(this->job->id.desc.starts_with("/"));
        return Path.join(Workspace.path, "repos/plywood/docs",
                         this->job->id.desc.ltrim([](char c) { return c == '/'; }) +
                             ".md");
    }
};

} // namespace docs
} // namespace ply
