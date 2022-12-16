/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-repository/Common.h>
#include <ply-biscuit/ParseTree.h>

namespace ply {
namespace build2 {

struct Repository {
    struct Plyfile {
        String src;
        biscuit::Tokenizer tkr; // For tokenData and FileLocationMap (only)
        Owned<biscuit::StatementBlock> contents;
    };

    struct ConfigOptions {
        LabelMap<AnyOwnedObject> map;
    };

    struct ModuleOrFunction {
        PLY_REFLECT()
        // ply reflect off

        Plyfile* plyfile = nullptr;
        Owned<biscuit::Statement> stmt;
        Owned<biscuit::Statement> generateBlock;
        bool generatedOnce = false;
        Owned<ConfigOptions> defaultOptions;
        Owned<ConfigOptions> currentOptions;
    };

    struct ModuleConfigBlock {
        ModuleOrFunction* mod = nullptr;
        Owned<biscuit::Statement> block;
    };

    struct ConfigList {
        Plyfile* plyfile = nullptr;
        u32 fileOffset = 0;
        Owned<biscuit::Statement> blockStmt;
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

} // namespace build2

PLY_DECLARE_TYPE_DESCRIPTOR(build2::Repository::ConfigOptions)

} // namespace ply
