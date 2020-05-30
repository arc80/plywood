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

PLY_NO_INLINE void Repo::addExternProvider(StringView providerName,
                                           ExternProvider::ExternFunc* externFunc) {
    Array<StringView> ownComps = splitName(providerName, "extern provider name");
    ArrayView<StringView> comps = ownComps.view();
    if (comps.isEmpty()) {
        exit(1); // fatal, message was already logged
    }

    const DependencySource* depSrc = this->findOrCreateExtern(comps, true);
    if (!depSrc) {
        exit(1); // fatal, message was already logged
    }

    if (comps.isEmpty()) {
        // Does not return:
        ErrorHandler::log(
            ErrorHandler::Fatal,
            String::format("'{}' is an extern name; expected an extern provider name\n",
                           providerName));
    }
    if (comps.numItems > 1) {
        // Does not return:
        ErrorHandler::log(
            ErrorHandler::Fatal,
            String::format("Too many components in extern provider name '{}'\n", providerName));
    }

    Owned<ExternProvider> provider = new ExternProvider;
    provider->extern_ = depSrc;
    provider->providerName = comps[0];
    provider->repo = this;
    provider->externFunc = externFunc;
    this->externProviders.append(std::move(provider));
}

struct RepoVisitor {
    Array<const Repo*> checkedRepos;
    bool preChildren = true;

    RepoVisitor(bool preChildren = true) {
    }

    PLY_NO_INLINE void visit(const Repo* repo, const LambdaView<bool(const Repo*)>& callback) {
        if (ply::findItem(this->checkedRepos.view(), repo) >= 0)
            return;
        this->checkedRepos.append(repo);
        if (this->preChildren) {
            if (!callback(repo))
                return;
        }
        for (Repo* childRepo : repo->childRepos) {
            visit(childRepo, callback);
        }
        if (!this->preChildren) {
            callback(repo);
        }
    }
};

const Repo* Repo::findChildRepo(StringView childName) const {
    const Repo* result = nullptr;
    RepoVisitor{}.visit(this, [&](const Repo* r) {
        if (r->repoName == childName) {
            result = r;
            return false;
        }
        return true;
    });
    return result;
}

const DependencySource* Repo::findExternImm(StringView externName) const {
    s32 i = find(this->externs.view(),
                 [&](const DependencySource* depSrc) { return depSrc->name == externName; });
    if (i >= 0) {
        return this->externs[i];
    }
    return nullptr;
}

const DependencySource* Repo::findExternRecursive(StringView externName) const {
    const DependencySource* extern_ = nullptr;
    bool conflict = false;
    RepoVisitor{}.visit(this, [&](const Repo* r) {
        s32 i = find(r->externs.view(),
                     [&](const DependencySource* depSrc) { return depSrc->name == externName; });
        if (i >= 0) {
            if (extern_) {
                conflict = true;
            } else {
                extern_ = r->externs[i];
            }
        }
        return true;
    });
    if (conflict)
        return nullptr;
    return extern_;
}

const DependencySource* Repo::findOrCreateExtern(ArrayView<StringView>& comps, bool allowCreate) {
    PLY_ASSERT(!comps.isEmpty());
    const Repo* childRepo = this->findChildRepo(comps[0]);
    StringView externName;
    if (childRepo) {
        comps.offsetHead(1);
        if (comps.isEmpty()) {
            ErrorHandler::log(
                ErrorHandler::Error,
                String::format("'{}' is a repo; expected an extern name\n", childRepo->repoName));
            return nullptr;
        }
        externName = comps[0];
        comps.offsetHead(1);
        const DependencySource* depSrc = childRepo->findExternImm(externName);
        if (depSrc)
            return depSrc;
        if (!(childRepo == this && allowCreate)) {
            ErrorHandler::log(ErrorHandler::Error,
                              String::format("Can't find extern '{}' in repo '{}'\n", externName,
                                             childRepo->repoName));
            return nullptr;
        }
        // Create the extern below
    } else {
        childRepo = this;
        externName = comps[0];
        comps.offsetHead(1);
        // FIXME: Should warn here in ambiguous cases, such as when an unqualified extern name is
        // used but the same extern name is defined in multiple repos. Just need to decide which
        // cases are considered ambiguous and which cases are not...
        const DependencySource* depSrc = this->findExternRecursive(externName);
        if (depSrc)
            return depSrc;
        if (!(childRepo == this && allowCreate)) {
            ErrorHandler::log(ErrorHandler::Error,
                              String::format("Can't find extern '{}' in any repo\n", externName));
            return nullptr;
        }
        // Create the extern below
    }
    // Create a new extern
    PLY_ASSERT(allowCreate);
    PLY_ASSERT(externName);
    DependencySource* depSrc = new DependencySource{DependencyType::Extern};
    depSrc->name = externName;
    depSrc->repo = this;
    this->externs.append(depSrc);
    return depSrc;
}

const TargetInstantiator* Repo::findTargetInstantiatorImm(StringView targetInstName) const {
    auto instCursor = this->targetInstantiators.find(targetInstName);
    if (instCursor.wasFound())
        return *instCursor;
    return nullptr;
}

const TargetInstantiator* Repo::findTargetInstantiatorRecursive(StringView targetInstName) const {
    const TargetInstantiator* targetInst = nullptr;
    bool conflict = false;
    RepoVisitor{}.visit(this, [&](const Repo* r) {
        auto instCursor = r->targetInstantiators.find(targetInstName);
        if (instCursor.wasFound()) {
            if (targetInst) {
                conflict = true;
            } else {
                targetInst = *instCursor;
            }
        }
        return true;
    });
    if (conflict)
        return nullptr;
    return targetInst;
}

} // namespace build
} // namespace ply
