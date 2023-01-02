/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                    ┃
┃    ╱   ╱╲    Plywood C++ Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/           ┃
┃    └──┴┴┴┘                                  ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#pragma once
#include "core.h"
#include <ply-build-repo/BuildFolder.h>

using namespace ply::build;

struct Workspace_t {
    String path; // absolute path to folder

    PLY_REFLECT()
    String currentBuildFolder;
    CMakeGeneratorOptions defaultCMakeOptions;
    String defaultConfig;
    String sourceNewLines;
    // ply reflect off

    void load();
    bool save() const;

    TextFormat getSourceTextFormat() const;
};

extern Workspace_t Workspace;
