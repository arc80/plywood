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
        String src;
        crowbar::Tokenizer tkr; // For tokenData and FileLocationMap (only)
        Owned<crowbar::StatementBlock> contents;
    };

    struct ConfigOptions {
        LabelMap<AnyOwnedObject> map;
    };

    struct ModuleOrFunction {
        PLY_REFLECT()
        // ply reflect off

        Plyfile* plyfile = nullptr;
        Owned<crowbar::Statement> stmt;
        Owned<crowbar::Statement> generateBlock;
        bool generatedOnce = false;
        Owned<ConfigOptions> defaultOptions;
        Owned<ConfigOptions> currentOptions;
    };

    struct ModuleConfigBlock {
        ModuleOrFunction* mod = nullptr;
        Owned<crowbar::Statement> block;
    };

    struct ConfigList {
        Plyfile* plyfile = nullptr;
        u32 fileOffset = 0;
        Owned<crowbar::Statement> blockStmt;
    };

    Array<Owned<Plyfile>> plyfiles;
    Array<Owned<ModuleOrFunction>> modules;
    Array<Owned<ModuleOrFunction>> functions;
    LabelMap<ModuleOrFunction*> globalScope; // Maps names to modules & functions
    Array<ModuleConfigBlock> moduleConfigBlocks;
    Owned<ConfigList> configList;

    static void create();
};

extern Repository* g_repository;

bool parsePlyfile(StringView path);

} // namespace latest
} // namespace build

PLY_DECLARE_TYPE_DESCRIPTOR(build::latest::Repository::ConfigOptions)

} // namespace ply
