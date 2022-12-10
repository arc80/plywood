/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <Core.h>
#include <ply-build-folder/BuildFolder.h>

struct Workspace_ {
    PLY_REFLECT()
    String currentBuildFolder;
    build::CMakeGeneratorOptions defaultCMakeOptions;
    String defaultConfig;
    String sourceNewLines;
    // ply reflect off

    static String getPath() {
        return NativePath::join(PLY_WORKSPACE_FOLDER, "workspace-settings.pylon");
    }

    bool load();
    bool save() const;

    TextFormat getSourceTextFormat() const;
};

extern Workspace_ Workspace;
