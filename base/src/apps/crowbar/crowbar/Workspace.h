/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <Core.h>
#include <ply-build-folder/BuildFolder.h>

struct Workspace_ {
    String path;

    PLY_REFLECT()
    String currentBuildFolder;
    build::CMakeGeneratorOptions defaultCMakeOptions;
    String defaultConfig;
    String sourceNewLines;
    // ply reflect off

    void load();
    void save() const;

    TextFormat getSourceTextFormat() const;
};

extern Workspace_ Workspace;
