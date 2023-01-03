/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-repo/Common.h>
#include <ply-biscuit/ParseTree.h>

namespace ply {
namespace build {

struct Repository {
    struct Plyfile {
        String src;
        biscuit::Tokenizer tkr; // For tokenData and FileLocationMap (only)
        Owned<biscuit::StatementBlock> contents;
    };

    struct ConfigOptions {
        LabelMap<AnyOwnedObject> map;
    };

    struct Function {
        PLY_REFLECT()
        // ply reflect off

        Plyfile* plyfile = nullptr;
        Owned<biscuit::Statement> stmt;
        Owned<biscuit::Statement> generateBlock;
        bool generatedOnce = false;
        Owned<ConfigOptions> defaultOptions;
        Owned<ConfigOptions> currentOptions;
    };

    struct TargetConfigBlock {
        Function* target_func = nullptr;
        Owned<biscuit::Statement> block;
    };

    struct ConfigList {
        Plyfile* plyfile = nullptr;
        u32 fileOffset = 0;
        Owned<biscuit::Statement> blockStmt;
    };

    Array<Owned<Plyfile>> plyfiles;
    Array<Owned<Function>> targets;
    Array<Owned<Function>> functions;
    LabelMap<Function*> globalScope; // Maps names to targets & functions
    Array<TargetConfigBlock> targetConfigBlocks;
    Owned<ConfigList> configList;

    static void create();
};

extern Repository* g_repository;

bool parsePlyfile(StringView path);

} // namespace build

PLY_DECLARE_TYPE_DESCRIPTOR(build::Repository::ConfigOptions)

} // namespace ply
