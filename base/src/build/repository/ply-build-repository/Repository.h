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
        String path;
        crowbar::Tokenizer tkr; // For tokenData and FileLocationMap (only)
        Owned<crowbar::StatementBlock> contents;
    };

    struct Module {
        PLY_REFLECT()
        // ply reflect off

        Plyfile* plyfile = nullptr;
        u32 fileOffset = 0;
        crowbar::Statement::CustomBlock* block = nullptr;
        Owned<crowbar::Statement> configBlock;
    };

    struct ModuleMapTraits {
        using Key = Label;
        using Item = Owned<Module>;
        static bool match(const Item& item, Label name) {
            return item->block->name == name;
        }
    };

    Common common;
    Array<Owned<Plyfile>> plyfiles;
    HashMap<ModuleMapTraits> moduleMap;

    static Owned<Repository> instance;
    static void create();
};

bool parsePlyfile(StringView path);

} // namespace latest
} // namespace build
} // namespace ply
