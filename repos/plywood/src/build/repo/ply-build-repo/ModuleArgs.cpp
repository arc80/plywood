/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-repo/ModuleArgs.h>
#include <ply-build-repo/ProjectInstantiator.h>
#include <ply-build-repo/RepoRegistry.h>
#include <ply-build-repo/ExternProvider.h>
#include <ply-build-repo/ErrorHandler.h>
#include <ply-runtime/algorithm/Find.h>

namespace ply {
namespace build {

PLY_NO_INLINE void ModuleArgs::addIncludeDir(Visibility visibility, StringView includeDir) {
    String absIncludeDir = NativePath::join(this->targetInst->instantiatorPath, includeDir);
    this->buildTarget->addIncludeDir(visibility, absIncludeDir);
}

PLY_NO_INLINE void ModuleArgs::addSourceFiles(StringView sourcePath, bool recursive) {
    String absSourcePath = NativePath::join(this->targetInst->instantiatorPath, sourcePath);
    this->buildTarget->addSourceFiles(absSourcePath, recursive);
}

PLY_NO_INLINE void ModuleArgs::addSourceFilesWhenImported(StringView sourceRoot,
                                                          ArrayView<const StringView> relPaths) {
    String absSourceRoot = NativePath::join(this->targetInst->instantiatorPath, sourceRoot);
    this->buildTarget->addSourceFilesWhenImported(absSourceRoot, relPaths);
}

PLY_NO_INLINE void ModuleArgs::setPrecompiledHeader(StringView generatorSource,
                                                    StringView pchInclude) {
    String absGeneratorSource =
        NativePath::join(this->targetInst->instantiatorPath, generatorSource);
    this->buildTarget->setPrecompiledHeader(absGeneratorSource, pchInclude);
}

// This function is called when a dependency instantiator function does something wrong
PLY_NO_INLINE void instantiatorError(StringView message) {
    StdErr::createStringWriter() << message << '\n';
    PLY_FORCE_CRASH(); // FIXME: Decide how to report such errors
}

PLY_NO_INLINE void addDependency(ModuleArgs* targetArgs, const DependencySource* depSrc,
                                 Visibility visibility) {
    // Instantiate the dependency
    Dependency* dep = targetArgs->projInst->instantiate(depSrc, false);
    if (!dep)
        return; // unselected or uninstalled extern

    // Check if it's already a dependency
    if (find(targetArgs->buildTarget->dep->dependencies.view(),
             [&](const auto& p) { return p.second == dep; }) >= 0) {
        instantiatorError(
            String::format("{} '{}' is already a dependency",
                           depSrc->type == DependencySource::Extern ? "extern" : "target",
                           RepoRegistry::get()->getShortDepSourceName(depSrc)));
        return;
    }

    targetArgs->buildTarget->dep->dependencies.append({visibility, dep});
}

PLY_NO_INLINE void ModuleArgs::addTarget(Visibility visibility, StringView targetName) {
    Array<StringView> ownComps = splitName(targetName, "target name");
    ArrayView<StringView> comps = ownComps.view();
    if (comps.isEmpty())
        return;
    const Repo* childRepo = this->targetInst->repo->findChildRepo(comps[0]);
    const TargetInstantiator* targetInst = nullptr;
    if (childRepo) {
        comps.offsetHead(1);
        if (comps.isEmpty()) {
            ErrorHandler::log(
                ErrorHandler::Error,
                String::format("'{}' is a repo; expected a module name\n", targetName));
            return;
        }
        targetInst = childRepo->findTargetInstantiatorImm(comps[0]);
        if (!targetInst) {
            ErrorHandler::log(ErrorHandler::Error,
                              String::format("Can't find module '{}' in repo '{}'\n", comps[0],
                                             childRepo->repoName));
            return;
        }
        comps.offsetHead(1);
    } else {
        targetInst = this->targetInst->repo->findTargetInstantiatorRecursive(comps[0]);
        if (!targetInst) {
            ErrorHandler::log(ErrorHandler::Error,
                              String::format("Can't find module '{}' in any repo\n", comps[0]));
            return;
        }
        comps.offsetHead(1);
    }

    PLY_ASSERT(targetInst);
    addDependency(this, targetInst, visibility);
}

PLY_NO_INLINE void ModuleArgs::addExtern(Visibility visibility, StringView externName) {
    Array<StringView> ownComps = splitName(externName, "extern name");
    ArrayView<StringView> comps = ownComps.view();
    if (comps.isEmpty())
        return;

    const DependencySource* depSrc = this->targetInst->repo->findExtern(comps);
    if (!depSrc)
        return;
    if (!comps.isEmpty()) {
        ErrorHandler::log(ErrorHandler::Error,
                          String::format("Too many components in extern name '{}'\n", externName));
        return;
    }
    addDependency(this, depSrc, visibility);
}

} // namespace build
} // namespace ply
