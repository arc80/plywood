/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-repository/Common.h>
#include <ply-crowbar/ParseTree.h>

namespace ply {
namespace build {
namespace latest {

struct Repository {
    struct Plyfile {
        crowbar::Tokenizer tkr; // For tokenData and FileLocationMap (only)
        Owned<crowbar::StatementBlock> contents;
    };

    struct ConfigOptions {
        struct Traits {
            using Key = Label;
            struct Item {
                Label identifier;
                AnyOwnedObject obj;
                Item(Label identifier) : identifier{identifier} {
                }
            };
            static PLY_INLINE bool match(const Item& item, const Label& identifier) {
                return item.identifier == identifier;
            }
        };

        HashMap<Traits> map;
    };

    struct Module {
        PLY_REFLECT()
        // ply reflect off

        Plyfile* plyfile = nullptr;
        u32 fileOffset = 0;
        crowbar::Statement::CustomBlock* block = nullptr;
        Owned<crowbar::Statement> configBlock;
        Owned<ConfigOptions> defaultOptions;
        Owned<ConfigOptions> currentOptions;
    };

    struct ModuleMapTraits {
        using Key = Label;
        using Item = Owned<Module>;
        static bool match(const Item& item, Label name) {
            return item->block->name == name;
        }
    };

    struct ConfigList {
        Plyfile* plyfile = nullptr;
        u32 fileOffset = 0;
        Owned<crowbar::StatementBlock> block;
    };

    Common common;
    Array<Owned<Plyfile>> plyfiles;
    HashMap<ModuleMapTraits> moduleMap;
    Owned<ConfigList> configList;

    static Owned<Repository> instance;
    static void create();
};

bool parsePlyfile(StringView path);

} // namespace latest
} // namespace build
} // namespace ply
