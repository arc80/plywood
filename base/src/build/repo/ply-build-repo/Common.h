/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-repo/Core.h>
#include <ply-runtime/string/Label.h>

namespace ply {
namespace build {

// Common contains data that is shared between the parser and the interpreter.
struct Common {
    // String keys for all the extra keywords used by build system scripts: 'library',
    // 'include_directories', 'dependencies', etc.
    Label libraryKey;
    Label executableKey;
    Label sourceFilesKey;
    Label includeDirectoriesKey;
    Label preprocessorDefinitionsKey;
    Label compileOptionsKey;
    Label linkLibrariesKey;
    Label dependenciesKey;
    Label publicKey;
    Label privateKey;
    Label configOptionsKey;
    Label configListKey;
    Label configKey;
    Label optimizationKey;
    Label generateKey;

    static void initialize();
};

extern Common* g_common;

// StatementAttributes is a type of object created at parse time (via ParseHooks) and consumed by
// the interpreter (via InterpreterHooks). It contains extra information about each entry inside
// a 'include_directories' or 'dependencies' block.
struct StatementAttributes {
    PLY_REFLECT()
    s32 visibilityTokenIdx = -1;
    bool isPublic = false;
    // ply reflect off
};

} // namespace build
} // namespace ply
