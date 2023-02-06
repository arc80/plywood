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

    Reference<SemaEntity> sema_ent;
    String link_destination;

    void add_to_index();
    ~SymbolPagePair();
};

struct WebCookerIndex {
    struct ExtractPageMetaTraits {
        using Index = StringView;
        using Item = SymbolPagePair*; // Owned by CookResult_ExtractPageMeta
        static constexpr u32 NodeCapacity = 8;
        static Index get_index(Item symbol_page_pair) {
            return symbol_page_pair->sema_ent->name;
        }
        static bool less(Index a, Index b) {
            return a < b;
        }
        static void on_item_moved(Item, void*) {
        }
    };

    struct LinkIDTraits {
        using Index = StringView;
        using Item = CookResult_ExtractPageMeta*;
        static constexpr u32 NodeCapacity = 8;
        static Index get_index(Item page_meta_job) {
            return page_meta_job->link_id;
        }
        static bool less(Index a, Index b) {
            return a < b;
        }
        static void on_item_moved(Item, void*) {
        }
    };

    PLY_REFLECT()
    Reference<SemaEntity> global_scope;
    // ply reflect off

    BTree<ExtractPageMetaTraits> extract_page_meta;
    BTree<LinkIDTraits> link_idmap;
};

} // namespace docs
} // namespace ply
