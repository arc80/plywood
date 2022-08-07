/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-repo/ProjectInstantiator.h>
#include <ply-build-repo/ProjectInstantiationEnv.h>
#include <ply-build-repo/ModuleArgs.h>
#include <ply-build-repo/ExternProvider.h>
#include <ply-build-repo/RepoRegistry.h>
#include <ply-build-target/CMakeLists.h>
#include <ply-runtime/algorithm/Find.h>

namespace ply {
namespace build {

Dependency* ProjectInstantiator::instantiate(const DependencySource* depSrc, bool isSharedRoot) {
    auto depCursor = this->result->dependencyMap.find(depSrc, &this->result->dependencies);
    if (depCursor.wasFound()) {
        Dependency* dep = this->result->dependencies[*depCursor].dep;
        if (dep && !dep->initialized) {
            instantiatorError(
                String::format("Loop detected for '{}'",
                               depSrc->type == DependencySource::Extern ? "extern" : "target",
                               RepoRegistry::get()->getShortDepSourceName(depSrc)));
            return nullptr;
        }
        // It was already instantiated.
        if (this->depTreeNode) {
            DependencyTree* depTreeChild = &this->depTreeNode->children.append();
            depTreeChild->desc =
                String::format("({})", RepoRegistry::get()->getShortDepSourceName(depSrc));
        }
        return dep;
    }

    DependencyTree* depTreeChild = nullptr;
    if (this->depTreeNode) {
        depTreeChild = &this->depTreeNode->children.append();
        depTreeChild->desc = RepoRegistry::get()->getShortDepSourceName(depSrc);
    }
    PLY_SET_IN_SCOPE(this->depTreeNode, depTreeChild);

    // Prepare dummy entry to help detect dependency loops
    u32 depIndex = this->result->dependencies.numItems();
    auto cursor = this->result->dependencyMap.insertOrFind(depSrc, &this->result->dependencies);
    PLY_ASSERT(!cursor.wasFound());
    *cursor = depIndex;

    if (depSrc->type == DependencySource::Target) {
        // Create and instantiate the BuildTarget
        const TargetInstantiator* targetInst = static_cast<const TargetInstantiator*>(depSrc);
        Dependency* dep = new Dependency;
        dep->buildTarget = new BuildTarget{dep};
        BuildTarget* buildTarget = dep->buildTarget;

        // FIXME: Make sure the name is unique across the project.
        // There could be clashes when multiple repos are used.
        buildTarget->name = depSrc->name;
        this->result->dependencies.append(depSrc, dep);
        if (isSharedRoot) {
            this->sharedContainer = buildTarget;
        }
        buildTarget->sharedContainer = this->sharedContainer;

        ModuleArgs args;
        args.projInst = this;
        args.targetInst = targetInst;
        args.buildTarget = buildTarget;
        targetInst->moduleFunc(&args);
        if (buildTarget->targetType == BuildTargetType::Lib) {
            if (isSharedRoot) {
                buildTarget->targetType = BuildTargetType::DLL;
            } else if (this->sharedContainer && !buildTarget->dynamicLinkPrefix.isEmpty()) {
                // Libraries that export from DLL/EXE must have type ObjectLib
                buildTarget->targetType = BuildTargetType::ObjectLib;
            }
        }

        // Special case for targets with no source files
        if (!buildTarget->anySourceFiles) {
            switch (buildTarget->targetType) {
                case BuildTargetType::Lib:
                case BuildTargetType::ObjectLib: {
                    // Change it to a header-only lib
                    buildTarget->targetType = BuildTargetType::HeaderOnly;
                    break;
                }

                case BuildTargetType::DLL:
                case BuildTargetType::EXE: {
                    // Add an empty source file
                    String codeGenPath =
                        NativePath::join(this->env->buildFolderPath, "codegen", targetInst->name);
                    FileSystem::native()->makeDirsAndSaveBinaryIfDifferent(
                        NativePath::join(codeGenPath, "Null.cpp"), {});
                    buildTarget->sourceFiles.append({codeGenPath, {"Null.cpp"}});
                    break;
                }

                default:
                    break;
            }
        }

        PLY_ASSERT(!dep->initialized);
        dep->initialized = true;
        return dep;
    } else {
        PLY_ASSERT(depSrc->type == DependencySource::Extern);

        // Default is nullptr in case no provider is selected & installed:
        this->result->dependencies.append(depSrc, nullptr);

        // Find a selector for this extern:
        s32 i = find(this->env->externSelectors,
                     [&](const ExternProvider* p) { return p->extern_ == depSrc; });
        if (i < 0) {
            // Not selected
            if (find(this->result->unselectedExterns, depSrc) < 0) {
                this->result->unselectedExterns.append(depSrc);
            }
            if (depTreeChild) {
                depTreeChild->desc += " [unselected extern]";
            }
            return nullptr;
        }

        // Get the selected provider
        const ExternProvider* externProvider = this->env->externSelectors[i];
        PLY_ASSERT(externProvider);

        // Try to instantiate the selected provider
        // Will return nullptr if not installed
        Owned<Dependency> dep = new Dependency;
        ExternProviderArgs args;
        args.toolchain = this->env->toolchain;
        args.projInst = this;
        args.provider = externProvider;
        args.dep = dep;
        ExternResult er = externProvider->externFunc(ExternCommand::Instantiate, &args);
        if (er.code != ExternResult::Instantiated) {
            if (find(this->result->uninstalledProviders, externProvider) < 0) {
                this->result->uninstalledProviders.append(externProvider);
            }
            if (depTreeChild) {
                // FIXME: Check the ExternResult and be more specific here
                depTreeChild->desc =
                    String::format("{} [uninstalled extern => {}]", depTreeChild->desc,
                                   RepoRegistry::get()->getShortProviderName(externProvider));
            }
            return nullptr;
        }

        PLY_ASSERT(!dep->initialized);
        if (depTreeChild) {
            depTreeChild->desc =
                String::format("{} [extern => {}]", depTreeChild->desc,
                               RepoRegistry::get()->getShortProviderName(externProvider));
        }
        dep->initialized = true;
        this->result->dependencies[depIndex].dep = std::move(dep);
        return this->result->dependencies[depIndex].dep;
    }
}

void ProjectInstantiator::propagateAllDependencies() {
    for (const auto& depEntry : this->result->dependencies) {
        if (depEntry.dep) {
            propagateDependencyProperties(depEntry.dep);
        }
    }
}

void writeCMakeLists(OutStream* outs, StringView solutionName, StringView buildFolderPath,
                     const ProjectInstantiationResult* instResult, bool forBootstrap) {
    CMakeBuildFolder cbf;
    cbf.solutionName = solutionName;
    cbf.absPath = buildFolderPath;
    cbf.forBootstrap = forBootstrap;
    for (const auto& depEntry : instResult->dependencies) {
        if (depEntry.dep->buildTarget) {
            cbf.targets.append(depEntry.dep);
        }
    }
    writeCMakeLists(outs, &cbf);
}

} // namespace build
} // namespace ply
