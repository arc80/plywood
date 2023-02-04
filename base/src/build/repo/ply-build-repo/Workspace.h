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
    String currentBuildFolder;
    build::CMakeGeneratorOptions defaultCMakeOptions;
    String defaultConfig;
    String sourceNewLines;
    // ply reflect off

    void load();
    bool save() const;

    TextFormat getSourceTextFormat() const;
};

extern Workspace_t Workspace;

} // namespace ply
