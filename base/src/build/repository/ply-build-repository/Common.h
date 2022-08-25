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
    Label includeDirectoriesKey;
    Label sourceFilesKey;
    Label dependenciesKey;
    Label linkLibrariesKey;
    Label publicKey;
    Label privateKey;
    Label configOptionsKey;
    Label configListKey;
    Label configKey;

    Common() {
        this->moduleKey = LabelMap::instance.insertOrFind("module");
        this->executableKey = LabelMap::instance.insertOrFind("executable");
        this->includeDirectoriesKey = LabelMap::instance.insertOrFind("include_directories");
        this->sourceFilesKey = LabelMap::instance.insertOrFind("source_files");
        this->dependenciesKey = LabelMap::instance.insertOrFind("dependencies");
        this->linkLibrariesKey = LabelMap::instance.insertOrFind("link_libraries");
        this->publicKey = LabelMap::instance.insertOrFind("public");
        this->privateKey = LabelMap::instance.insertOrFind("private");
        this->configOptionsKey = LabelMap::instance.insertOrFind("config_options");
        this->configListKey = LabelMap::instance.insertOrFind("config_list");
        this->configKey = LabelMap::instance.insertOrFind("config");
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
