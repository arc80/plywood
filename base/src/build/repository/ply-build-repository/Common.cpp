/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-repository/Common.h>

namespace ply {
namespace build {
namespace latest {

Common* g_common = nullptr;

void Common::initialize() {
    PLY_ASSERT(!g_common);
    g_common = new Common;
    g_common->moduleKey = LabelMap::instance.insertOrFind("module");
    g_common->executableKey = LabelMap::instance.insertOrFind("executable");
    g_common->includeDirectoriesKey = LabelMap::instance.insertOrFind("include_directories");
    g_common->sourceFilesKey = LabelMap::instance.insertOrFind("source_files");
    g_common->dependenciesKey = LabelMap::instance.insertOrFind("dependencies");
    g_common->linkLibrariesKey = LabelMap::instance.insertOrFind("link_libraries");
    g_common->publicKey = LabelMap::instance.insertOrFind("public");
    g_common->privateKey = LabelMap::instance.insertOrFind("private");
    g_common->configOptionsKey = LabelMap::instance.insertOrFind("config_options");
    g_common->configListKey = LabelMap::instance.insertOrFind("config_list");
    g_common->configKey = LabelMap::instance.insertOrFind("config");
}

} // namespace latest
} // namespace build
} // namespace ply

#include "codegen/Common.inl"
