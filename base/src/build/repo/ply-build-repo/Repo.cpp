/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-repo/Repo.h>
#include <ply-build-repo/ErrorHandler.h>
#include <ply-runtime/algorithm/Find.h>

namespace ply {
namespace build {

PLY_BUILD_ENTRY void Repo::addTargetInstantiator(Owned<TargetInstantiator>&& targetInst) {
    auto cursor = this->targetInstantiators.insertOrFind(targetInst->name);
    if (cursor.wasFound()) {
        ErrorHandler::log(ErrorHandler::Fatal,
                          String::format("Duplicate module name '{}' in repo '{}'\n",
                                         targetInst->name, this->repoName));
        return; // shouldn't get here
    }
    *cursor = std::move(targetInst);
}

PLY_NO_INLINE void Repo::addExternProvider(StringView externName, StringView providerName,
                                           ExternProvider::ExternFunc* externFunc) {
    const DependencySource* depSrc = this->findOrCreateExtern(externName, true);
    if (!depSrc) {
        exit(1); // fatal, message was already logged
    }

    Owned<ExternProvider> provider = new ExternProvider;
    provider->extern_ = depSrc;
    provider->providerName = providerName;
    provider->externFunc = externFunc;
    this->externProviders.append(std::move(provider));
}

const DependencySource* Repo::findExtern(StringView externName) const {
    s32 i = find(this->externs,
                 [&](const DependencySource* depSrc) { return depSrc->name == externName; });
    if (i >= 0) {
        return this->externs[i];
    }
    return nullptr;
}

const DependencySource* Repo::findOrCreateExtern(StringView externName, bool allowCreate) {
    {
        const DependencySource* depSrc = this->findExtern(externName);
        if (depSrc)
            return depSrc;
        if (!allowCreate) {
            ErrorHandler::log(ErrorHandler::Error,
                              String::format("Can't find extern '{}' in any repo\n", externName));
            return nullptr;
        }
    }

    // Create a new extern
    PLY_ASSERT(allowCreate);
    PLY_ASSERT(externName);
    DependencySource* depSrc = new DependencySource{DependencySource::Extern};
    depSrc->name = externName;
    this->externs.append(depSrc);
    return depSrc;
}

const TargetInstantiator* Repo::findTargetInstantiator(StringView targetInstName) const {
    auto instCursor = this->targetInstantiators.find(targetInstName);
    if (instCursor.wasFound())
        return *instCursor;
    return nullptr;
}

} // namespace build
} // namespace ply
