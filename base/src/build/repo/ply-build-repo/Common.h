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
