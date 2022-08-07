/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-web-cook-docs/Core.h>
#include <ply-web-cook-docs/WebCookerIndex.h>
#include <ply-cook/CookJob.h>

namespace ply {
namespace docs {

void SymbolPagePair::addToIndex() {
    PLY_ASSERT(this->semaEnt->name);
    WebCookerIndex* wci = cook::DependencyTracker::current()->userData.cast<WebCookerIndex>();
    wci->extractPageMeta.insert(this);
}

SymbolPagePair::~SymbolPagePair() {
    PLY_ASSERT(this->semaEnt->name);
    WebCookerIndex* wci = cook::DependencyTracker::current()->userData.cast<WebCookerIndex>();
    auto iter = wci->extractPageMeta.findFirstGreaterOrEqualTo(this->semaEnt->name);
    while (iter.isValid() && iter.getItem() != this) {
        iter.next();
    }
    PLY_ASSERT(iter.isValid() && iter.getItem() == this);
    wci->extractPageMeta.remove(iter);
}

} // namespace docs
} // namespace ply

#include "codegen/WebCookerIndex.inl" //%%
