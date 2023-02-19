/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-build-repo/Core.h>

namespace ply {
namespace build {

// Common contains data that is shared between the parser and the interpreter.
struct Common {
    // String keys for all the extra keywords used by build system scripts: 'library',
    // 'include_directories', 'dependencies', etc.
    Label library_key;
    Label executable_key;
    Label source_files_key;
    Label include_directories_key;
    Label preprocessor_definitions_key;
    Label compile_options_key;
    Label link_libraries_key;
    Label prebuild_step_key;
    Label dependencies_key;
    Label public_key;
    Label private_key;
    Label config_options_key;
    Label config_list_key;
    Label config_key;
    Label optimization_key;
    Label generate_key;
    Label link_objects_directly_key;

    static void initialize();
};

extern Common* g_common;

// StatementAttributes is a type of object created at parse time (via ParseHooks) and
// consumed by the interpreter (via InterpreterHooks). It contains extra information
// about each entry inside a 'include_directories' or 'dependencies' block.
struct StatementAttributes {
    PLY_REFLECT()
    s32 visibility_token_idx = -1;
    bool is_public = false;
    // ply reflect off
};

} // namespace build
} // namespace ply
