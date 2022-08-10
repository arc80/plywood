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
    Label includeDirectoriesKey;
    Label sourceFilesKey;
    Label publicKey;
    Label privateKey;

    Common() {
        this->moduleKey = LabelMap::instance.insertOrFind("module");
        this->includeDirectoriesKey = LabelMap::instance.insertOrFind("include_directories");
        this->sourceFilesKey = LabelMap::instance.insertOrFind("source_files");
        this->publicKey = LabelMap::instance.insertOrFind("public");
        this->privateKey = LabelMap::instance.insertOrFind("private");
    }
};

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
