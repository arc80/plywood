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

String ExternProvider::getQualifiedName() const {
    return String::format("{}.{}", this->extern_->name, this->providerName);
}

PLY_NO_INLINE Tuple<ExternResult, ExternFolder*>
ExternProviderArgs::findExistingExternFolder(StringView desc) const {
    if (ExternFolder* folder = ExternFolderRegistry::get()->find(desc))
        return {ExternResult{ExternResult::Installed, {}}, folder};
    else
        return {ExternResult{ExternResult::SupportedButNotInstalled, {}}, nullptr};
}

PLY_NO_INLINE ExternFolder* ExternProviderArgs::createExternFolder(StringView desc) const {
    return ExternFolderRegistry::get()->create(desc);
}

} // namespace build
} // namespace ply
