/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <Core.h>
#include <ply-build-target/CMakeLists.h>

namespace ply {

struct WorkspaceSettings {
    PLY_REFLECT()
    String currentBuildFolder;
    build::CMakeGeneratorOptions defaultCMakeOptions;
    String defaultConfig;
    // ply reflect off

    static String getPath() {
        return NativePath::join(PLY_WORKSPACE_FOLDER, "workspace-settings.pylon");
    }

    bool load();
    bool save() const;
};

} // namespace ply
