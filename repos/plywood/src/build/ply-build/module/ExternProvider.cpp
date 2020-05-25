/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-repo/ExternProvider.h>
#include <ply-build-repo/DependencySource.h>
#include <ply-build-repo/Repo.h>
#include <ply-build-repo/RepoRegistry.h>
#include <ply-build-repo/ProjectInstantiator.h>
#include <ply-build-repo/ErrorHandler.h>
#include <ply-build-provider/ExternFolderRegistry.h>

namespace ply {
namespace build {

String ExternProvider::getFullyQualifiedName() const {
    if (this->repo == this->extern_->repo) {
        return String::format("{}.{}.{}", this->repo->repoName, this->extern_->name,
                              this->providerName);
    } else {
        return String::format("{}.{}.{}.{}", this->extern_->repo->repoName, this->extern_->name,
                              this->repo->repoName, this->providerName);
    }
}

PLY_NO_INLINE Tuple<ExternResult, ExternFolder*>
ExternProviderArgs::findExistingExternFolder(StringView folderArgs) const {
    String providerName = this->provider->getFullyQualifiedName();
    ExternFolder* folder = this->externFolders->find(providerName, folderArgs);
    if (!folder) {
        return {ExternResult{ExternResult::SupportedButNotInstalled, {}}, nullptr};
    }
    return {
        ExternResult{folder->success ? ExternResult::Installed : ExternResult::InstallFailed, {}},
        folder};
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

PLY_NO_INLINE ExternFolder* ExternProviderArgs::createExternFolder(StringView folderArgs) const {
    // Make directory
    String baseName = this->repoReg->getShortProviderName(this->provider);
    if (folderArgs) {
        baseName = String::format("{}.{}", baseName, folderArgs);
    }
    String folderPath =
        makeUniqueFileName(NativePath::join(PLY_WORKSPACE_FOLDER, "data/extern"), baseName);
    FSResult fsResult = FileSystem::native()->makeDirs(folderPath);
    if (!(fsResult == FSResult::OK || fsResult == FSResult::AlreadyExists)) {
        ErrorHandler::log(ErrorHandler::Fatal,
                          String::format("Can't create folder '{}'\n", folderPath));
        return nullptr; // Shouldn't get here
    }

    // Create ExternFolder object
    ExternFolder* folder = new ExternFolder;
    folder->path = std::move(folderPath);
    folder->providerName = this->provider->getFullyQualifiedName();
    folder->folderArgs = folderArgs;
    this->externFolders->folders.append(folder);
    return folder;
}

} // namespace build
} // namespace ply
