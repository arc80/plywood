/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repository/Common.h>

namespace ply {
namespace build {
namespace latest {

Common* g_common = nullptr;

void Common::initialize() {
    PLY_ASSERT(!g_common);
    g_common = new Common;
    g_common->moduleKey = g_labelStorage.insert("module");
    g_common->executableKey = g_labelStorage.insert("executable");
    g_common->includeDirectoriesKey = g_labelStorage.insert("include_directories");
    g_common->sourceFilesKey = g_labelStorage.insert("source_files");
    g_common->dependenciesKey = g_labelStorage.insert("dependencies");
    g_common->linkLibrariesKey = g_labelStorage.insert("link_libraries");
    g_common->publicKey = g_labelStorage.insert("public");
    g_common->privateKey = g_labelStorage.insert("private");
    g_common->configOptionsKey = g_labelStorage.insert("config_options");
    g_common->configListKey = g_labelStorage.insert("config_list");
    g_common->configKey = g_labelStorage.insert("config");
}

} // namespace latest
} // namespace build
} // namespace ply

#include "codegen/Common.inl"
