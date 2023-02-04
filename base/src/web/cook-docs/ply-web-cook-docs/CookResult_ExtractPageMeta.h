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
    bool markdownExists = false;
    String linkID;
    String title;
    Array<Reference<cook::CookJob>> childPages;
    String synopsis;
    Array<Owned<SymbolPagePair>> symbolPagePairs;
    // ply reflect off

    PLY_INLINE String getLinkDestination() const {
        PLY_ASSERT(this->job->id.desc.startsWith("/"));
        if (!markdownExists)
            return {};
        if (this->job->id.desc.endsWith("/index")) {
            if (this->job->id.desc == "/index") {
                return "/";
            } else {
                return StringView{"/docs"} + this->job->id.desc.shortenedBy(6);
            }
        } else {
            return StringView{"/docs"} + this->job->id.desc;
        }
    }
    PLY_INLINE String getMarkdownPath() const {
        PLY_ASSERT(this->job->id.desc.startsWith("/"));
        return Path.join(Workspace.path, "repos/plywood/docs",
                         this->job->id.desc.ltrim([](char c) { return c == '/'; }) +
                             ".md");
    }
};

} // namespace docs
} // namespace ply
