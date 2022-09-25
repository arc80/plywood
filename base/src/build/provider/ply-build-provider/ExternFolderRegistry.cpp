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

    auto aRoot = pylon::Parser{}.parse(infoPath, strContents).root;
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

ExternFolder* ExternFolderRegistry::find(StringView desc) const {
    for (ExternFolder* info : this->folders) {
        if (info->desc == desc)
            return info;
    }
    return nullptr;
}

PLY_NO_INLINE String makeUniqueFileName(StringView parentFolder, StringView prefix) {
    u32 number = 0;
    String suffix;
    for (;;) {
        String path = NativePath::join(parentFolder, prefix + suffix);
        if (FileSystem::native()->exists(path) == ExistsResult::NotFound)
            return path;
        number++;
        suffix = String::from(number);
        u32 numZeroDigits = max<s32>(3 - suffix.numBytes, 0);
        suffix = String::format(".{}{}", StringView{"0"} * numZeroDigits, suffix);
    }
}

PLY_NO_INLINE ExternFolder* ExternFolderRegistry::create(StringView desc) {
    // Make directory
    String folderPath =
        makeUniqueFileName(NativePath::join(PLY_WORKSPACE_FOLDER, "data/extern"), desc);
    FSResult fsResult = FileSystem::native()->makeDirs(folderPath);
    if (!(fsResult == FSResult::OK || fsResult == FSResult::AlreadyExists)) {
        PLY_ASSERT(0);  // Don't bother to handle gracefully; this module will be deleted soon
        return nullptr;
    }

    // Create ExternFolder object
    ExternFolder* folder = new ExternFolder;
    folder->path = std::move(folderPath);
    folder->desc = desc;
    this->folders.append(folder);
    return folder;
}

} // namespace build
} // namespace ply

#include "codegen/ExternFolderRegistry.inl" //%%
