/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-web-cook-docs/Core.h>
#include <ply-web-cook-docs/Sema.h>
#include <ply-web-cook-docs/SemaEntity.h>
#include <ply-runtime/container/BTree.h>
#include <ply-runtime/container/Hash128.h>
#include <ply-web-cook-docs/CookResult_ExtractPageMeta.h>

namespace ply {
namespace docs {

struct SymbolPagePair {
    PLY_REFLECT()
    // ply reflect off

    Reference<SemaEntity> semaEnt;
    String linkDestination;

    void addToIndex();
    ~SymbolPagePair();
};

struct WebCookerIndex {
    struct ExtractPageMetaTraits {
        using Index = StringView;
        using Item = SymbolPagePair*; // Owned by CookResult_ExtractPageMeta
        static constexpr u32 NodeCapacity = 8;
        static Index getIndex(Item symbolPagePair) {
            return symbolPagePair->semaEnt->name;
        }
        static bool less(Index a, Index b) {
            return a < b;
        }
        static void onItemMoved(Item, void*) {
        }
    };

    struct LinkIDTraits {
        using Index = StringView;
        using Item = CookResult_ExtractPageMeta*;
        static constexpr u32 NodeCapacity = 8;
        static Index getIndex(Item pageMetaJob) {
            return pageMetaJob->linkID;
        }
        static bool less(Index a, Index b) {
            return a < b;
        }
        static void onItemMoved(Item, void*) {
        }
    };

    PLY_REFLECT()
    Reference<SemaEntity> globalScope;
    // ply reflect off

    BTree<ExtractPageMetaTraits> extractPageMeta;
    BTree<LinkIDTraits> linkIDMap;
};

} // namespace docs
} // namespace ply
