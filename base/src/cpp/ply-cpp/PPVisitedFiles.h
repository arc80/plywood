/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
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
        FileLocationMap fileLocationMap;
    };
    Array<SourceFile> sourceFiles;

    struct MacroExpansion {
        u32 fromFile : 1;
        u32 fileIdx : 31;
        union {
            struct {
                u32 startOfs = 0;
                u32 endOfs = 0;
            } range;
            String str;
        };
        bool takesArgs = false;

        PLY_INLINE MacroExpansion() : fromFile{1}, fileIdx{0} {
            new (&this->range) decltype(this->range);
        }
        void destruct();
        PLY_INLINE ~MacroExpansion() {
            this->destruct();
        }
        PLY_INLINE void setString(StringView value) {
            destruct();
            this->fromFile = 0;
            this->fileIdx = 0;
            new (&this->str) String{value};
        }
    };
    Array<MacroExpansion> macroExpansions;

    struct IncludeChain {
        u32 isMacroExpansion : 1;
        u32 fileOrExpIdx : 31;
        s32 parentIdx = -1;

        PLY_INLINE IncludeChain() : isMacroExpansion{0}, fileOrExpIdx{0} {
        }
    };
    Array<IncludeChain> includeChains;

    struct LocationMapTraits {
        struct Item {
            LinearLocation linearLoc = -1;
            u32 includeChainIdx = 0;
            u32 offset = 0; // linearLoc corresponds to this file offset
        };
        using Index = LinearLocation;
        static constexpr u32 NodeCapacity = 8;
        static PLY_INLINE Index getIndex(const Item& item) {
            return item.linearLoc;
        }
        static PLY_INLINE bool less(Index a, Index b) {
            return a < b;
        }
        static PLY_INLINE void onItemMoved(const Item, void*) {
        }
        static PLY_INLINE bool match(const Item& a, const Item& b) {
            return a.linearLoc == b.linearLoc;
        }
    };
    BTree<LocationMapTraits> locationMap;

    StringView getContents(u32 includeChainIndex) const;
};

} // namespace cpp
} // namespace ply
