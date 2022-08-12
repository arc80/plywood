/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-repository/Repository.h>

namespace ply {
namespace build {
namespace latest {

Owned<Repository> Repository::instance;

void Repository::create() {
    Repository::instance = Owned<Repository>::create();

    for (const DirectoryEntry& entry : FileSystem::native()->listDir(PLY_WORKSPACE_FOLDER, 0)) {
        if (!entry.isDir)
            continue;
        if (entry.name.startsWith("."))
            continue;
        if (entry.name == "data")
            continue;

        // Recursively find all Plyfiles
        String repoFolder = NativePath::join(PLY_WORKSPACE_FOLDER, entry.name);
        for (const WalkTriple& triple : FileSystem::native()->walk(repoFolder)) {
            for (const WalkTriple::FileInfo& file : triple.files) {
                if (file.name == "Plyfile") {
                    parsePlyfile(NativePath::join(triple.dirPath, file.name));
                }
            }
        }
    }
}

} // namespace latest
} // namespace build
} // namespace ply
