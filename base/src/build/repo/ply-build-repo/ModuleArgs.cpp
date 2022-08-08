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

PLY_BUILD_ENTRY void ModuleArgs::addNonParticipatingFiles(StringView sourceRoot,
                                                          ArrayView<const StringView> relPaths) {
    String absSourceRoot = NativePath::join(this->targetInst->instantiatorPath, sourceRoot);
    this->buildTarget->addNonParticipatingFiles(absSourceRoot, relPaths);
}

PLY_NO_INLINE void ModuleArgs::setPrecompiledHeader(StringView generatorSource,
                                                    StringView pchInclude) {
    String absGeneratorSource =
        NativePath::join(this->targetInst->instantiatorPath, generatorSource);
    this->buildTarget->setPrecompiledHeader(absGeneratorSource, pchInclude);
}

// This function is called when a dependency instantiator function does something wrong
PLY_NO_INLINE void instantiatorError(StringView message) {
    StdErr::text() << message << '\n';
    PLY_FORCE_CRASH(); // FIXME: Decide how to report such errors
}

PLY_NO_INLINE void addDependency(ModuleArgs* targetArgs, const DependencySource* depSrc,
                                 Visibility visibility) {
    // Instantiate the dependency
    Dependency* dep = targetArgs->projInst->instantiate(depSrc, false);
    if (!dep)
        return; // unselected or uninstalled extern

    // Check if it's already a dependency
    if (find(targetArgs->buildTarget->dep->dependencies,
             [&](const auto& p) { return p.second == dep; }) >= 0) {
        instantiatorError(String::format(
            "{} '{}' is already a dependency",
            depSrc->type == DependencySource::Extern ? "extern" : "target", depSrc->name));
        return;
    }

    targetArgs->buildTarget->dep->dependencies.append({visibility, dep});
}

PLY_NO_INLINE void ModuleArgs::addTarget(Visibility visibility, StringView qualifiedName) {
    const TargetInstantiator* targetInst =
        RepoRegistry::get()->findTargetInstantiator(qualifiedName);
    if (!targetInst)
        return;
    addDependency(this, targetInst, visibility);
}

PLY_NO_INLINE void ModuleArgs::addExtern(Visibility visibility, StringView qualifiedName) {
    const DependencySource* depSrc = RepoRegistry::get()->findExtern(qualifiedName);
    if (!depSrc)
        return;
    addDependency(this, depSrc, visibility);
}

} // namespace build
} // namespace ply
