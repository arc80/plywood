/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-build-repo/Common.h>

namespace ply {
namespace build {

Common* g_common = nullptr;

void Common::initialize() {
    PLY_ASSERT(!g_common);
    g_common = new Common;
    g_common->library_key = g_labelStorage.insert("library");
    g_common->executable_key = g_labelStorage.insert("executable");
    g_common->include_directories_key = g_labelStorage.insert("include_directories");
    g_common->preprocessor_definitions_key =
        g_labelStorage.insert("preprocessor_definitions");
    g_common->source_files_key = g_labelStorage.insert("source_files");
    g_common->dependencies_key = g_labelStorage.insert("dependencies");
    g_common->link_libraries_key = g_labelStorage.insert("link_libraries");
    g_common->prebuild_step_key = g_labelStorage.insert("prebuild_step");
    g_common->public_key = g_labelStorage.insert("public");
    g_common->private_key = g_labelStorage.insert("private");
    g_common->config_options_key = g_labelStorage.insert("config_options");
    g_common->config_list_key = g_labelStorage.insert("config_list");
    g_common->config_key = g_labelStorage.insert("config");
    g_common->compile_options_key = g_labelStorage.insert("compile_options");
    g_common->optimization_key = g_labelStorage.insert("optimization");
    g_common->generate_key = g_labelStorage.insert("generate");
    g_common->link_objects_directly_key =
        g_labelStorage.insert("link_objects_directly");
    g_common->preprocess_labels_key = g_labelStorage.insert("preprocess_labels");
}

} // namespace build
} // namespace ply

#include "codegen/Common.inl"
