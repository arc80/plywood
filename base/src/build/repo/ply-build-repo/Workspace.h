/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-build-repo/Core.h>
#include <ply-build-repo/BuildFolder.h>

namespace ply {

struct Workspace_t {
    String path; // absolute path to folder

    PLY_REFLECT()
    String current_build_folder;
    build::CMakeGeneratorOptions default_cmake_options;
    String default_config;
    String source_new_lines;
    // ply reflect off

    void load();
    bool save() const;

    TextFormat get_source_text_format() const;
};

extern Workspace_t Workspace;

} // namespace ply
