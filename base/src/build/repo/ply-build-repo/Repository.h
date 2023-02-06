/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-build-repo/Common.h>
#include <ply-biscuit/ParseTree.h>

namespace ply {
namespace build {

struct Repository {
    struct Plyfile {
        String src;
        biscuit::Tokenizer tkr; // For token_data and FileLocationMap (only)
        Owned<biscuit::StatementBlock> contents;
    };

    struct ConfigOptions {
        Map<Label, AnyOwnedObject> map;
    };

    struct Function {
        PLY_REFLECT()
        // ply reflect off

        Plyfile* plyfile = nullptr;
        Owned<biscuit::Statement> stmt;
        Owned<biscuit::Statement> generate_block;
        bool generated_once = false;
        Owned<ConfigOptions> default_options;
        Owned<ConfigOptions> current_options;
    };

    struct TargetConfigBlock {
        Function* target_func = nullptr;
        Owned<biscuit::Statement> block;
    };

    struct ConfigList {
        Plyfile* plyfile = nullptr;
        u32 file_offset = 0;
        Owned<biscuit::Statement> block_stmt;
    };

    Array<Owned<Plyfile>> plyfiles;
    Array<Owned<Function>> targets;
    Array<Owned<Function>> functions;
    Map<Label, Function*> global_scope; // Maps names to targets & functions
    Array<TargetConfigBlock> target_config_blocks;
    Owned<ConfigList> config_list;

    static void create();
};

extern Repository* g_repository;

bool parse_plyfile(StringView path);

} // namespace build

PLY_DECLARE_TYPE_DESCRIPTOR(build::Repository::ConfigOptions)

} // namespace ply
