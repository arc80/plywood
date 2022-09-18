/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>
#include <ply-runtime/string/Label.h>

namespace ply {
namespace build {
namespace latest {

// Common contains data that is shared between the parser and the interpreter.
struct Common {
    // String keys for all the extra keywords used by build system scripts: 'module',
    // 'include_directories', 'dependencies', etc.
    Label moduleKey;
    Label executableKey;
    Label externKey;
    Label includeDirectoriesKey;
    Label sourceFilesKey;
    Label dependenciesKey;
    Label linkLibrariesKey;
    Label publicKey;
    Label privateKey;
    Label configOptionsKey;
    Label configListKey;
    Label configKey;

    static void initialize();
};

extern Common* g_common;

// ExpressionTraits is a type of object created at parse time (via ParseHooks) and consumed by
// the interpreter (via InterpreterHooks). It contains extra information about each entry inside
// a 'include_directories' or 'dependencies' block.
struct ExpressionTraits {
    PLY_REFLECT()
    s32 visibilityTokenIdx = -1;
    bool isPublic = false;
    // ply reflect off
};

} // namespace latest
} // namespace build
} // namespace ply
