/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-cpp/Core.h>
#include <ply-runtime/io/text/FileLocationMap.h>
#include <ply-runtime/container/BTree.h>
#include <ply-cpp/LinearLocation.h>

namespace ply {
namespace cpp {

struct PPVisitedFiles {
    struct SourceFile {
        HybridString contents;
        FileLocationMap file_location_map;
    };
    Array<SourceFile> source_files;

    struct MacroExpansion {
        u32 from_file : 1;
        u32 file_idx : 31;
        union {
            struct {
                u32 start_ofs = 0;
                u32 end_ofs = 0;
            } range;
            String str;
        };
        bool takes_args = false;

        PLY_INLINE MacroExpansion() : from_file{1}, file_idx{0} {
            new (&this->range) decltype(this->range);
        }
        void destruct();
        PLY_INLINE ~MacroExpansion() {
            this->destruct();
        }
        PLY_INLINE void set_string(StringView value) {
            destruct();
            this->from_file = 0;
            this->file_idx = 0;
            new (&this->str) String{value};
        }
    };
    Array<MacroExpansion> macro_expansions;

    struct IncludeChain {
        u32 is_macro_expansion : 1;
        u32 file_or_exp_idx : 31;
        s32 parent_idx = -1;

        PLY_INLINE IncludeChain() : is_macro_expansion{0}, file_or_exp_idx{0} {
        }
    };
    Array<IncludeChain> include_chains;

    struct LocationMapTraits {
        struct Item {
            LinearLocation linear_loc = -1;
            u32 include_chain_idx = 0;
            u32 offset = 0; // linear_loc corresponds to this file offset
        };
        using Index = LinearLocation;
        static constexpr u32 NodeCapacity = 8;
        static PLY_INLINE Index get_index(const Item& item) {
            return item.linear_loc;
        }
        static PLY_INLINE bool less(Index a, Index b) {
            return a < b;
        }
        static PLY_INLINE void on_item_moved(const Item, void*) {
        }
        static PLY_INLINE bool match(const Item& a, const Item& b) {
            return a.linear_loc == b.linear_loc;
        }
    };
    BTree<LocationMapTraits> location_map;

    StringView get_contents(u32 include_chain_index) const;
};

} // namespace cpp
} // namespace ply
