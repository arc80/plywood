/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-provider/ExternFolderRegistry.h>
#include <pylon/Parse.h>
#include <pylon/Write.h>
#include <pylon-reflect/Import.h>
#include <pylon-reflect/Export.h>

namespace ply {
namespace build {

Owned<ExternFolderRegistry> ExternFolderRegistry::instance_;

PLY_NO_INLINE Owned<ExternFolder> ExternFolder::load(String&& path) {
    String infoPath = NativePath::join(path, "info.pylon");
    String strContents = FileSystem::native()->loadTextAutodetect(infoPath).first;
    if (FileSystem::native()->lastResult() != FSResult::OK)
        return nullptr;

    auto aRoot = pylon::Parser{}.parse(strContents).root;
    if (!aRoot->isValid())
        return nullptr;

    Owned<ExternFolder> info = pylon::import<ExternFolder>(aRoot);
    info->path = std::move(path);
    return info;
}

PLY_NO_INLINE bool ExternFolder::save() const {
    auto aRoot = pylon::exportObj(AnyObject::bind(this));
    String strContents = pylon::toString(aRoot);
    String infoPath = NativePath::join(this->path, "info.pylon");
    FSResult rc = FileSystem::native()->makeDirsAndSaveTextIfDifferent(
        infoPath, strContents, TextFormat::platformPreference());
    return (rc == FSResult::OK || rc == FSResult::Unchanged);
}

Owned<ExternFolderRegistry> ExternFolderRegistry::create() {
    PLY_ASSERT(!instance_);
    Owned<ExternFolderRegistry> externFolders = new ExternFolderRegistry;
    externFolders->folders.clear();
    String buildFolderRoot = NativePath::join(PLY_WORKSPACE_FOLDER, "data/extern");
    // FIXME: Use native() or compat() consistently
    // FIXME: Detect and report duplicate extern folders
    for (const DirectoryEntry& entry : FileSystem::native()->listDir(buildFolderRoot, 0)) {
        if (!entry.isDir)
            continue;
        PLY_ASSERT(!entry.name.isEmpty());
        String folderPath = NativePath::join(buildFolderRoot, entry.name);
        Owned<ExternFolder> folderInfo = ExternFolder::load(std::move(folderPath));
        if (!folderInfo)
            continue;
        externFolders->folders.append(std::move(folderInfo));
    }
    return externFolders;
}

ExternFolder* ExternFolderRegistry::find(StringView providerName, StringView folderArgs) const {
    for (ExternFolder* info : this->folders) {
        if (info->providerName == providerName && info->folderArgs == folderArgs) {
            return info;
        }
    }
    return nullptr;
}

} // namespace build
} // namespace ply

#include "codegen/ExternFolderRegistry.inl" //%%
