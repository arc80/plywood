/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-web-cook-docs/Core.h>
#include <ply-web-cook-docs/WebCookerIndex.h>
#include <ply-cook/CookJob.h>

namespace ply {
namespace docs {

void SymbolPagePair::add_to_index() {
    PLY_ASSERT(this->sema_ent->name);
    WebCookerIndex* wci =
        cook::DependencyTracker::current()->user_data.cast<WebCookerIndex>();
    wci->extract_page_meta.insert(this);
}

SymbolPagePair::~SymbolPagePair() {
    PLY_ASSERT(this->sema_ent->name);
    WebCookerIndex* wci =
        cook::DependencyTracker::current()->user_data.cast<WebCookerIndex>();
    auto iter =
        wci->extract_page_meta.find_first_greater_or_equal_to(this->sema_ent->name);
    while (iter.is_valid() && iter.get_item() != this) {
        iter.next();
    }
    PLY_ASSERT(iter.is_valid() && iter.get_item() == this);
    wci->extract_page_meta.remove(iter);
}

} // namespace docs
} // namespace ply

#include "codegen/WebCookerIndex.inl" //%%
