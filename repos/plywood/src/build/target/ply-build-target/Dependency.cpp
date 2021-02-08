/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-target/Dependency.h>
#include <ply-build-target/TargetError.h>
#include <ply-runtime/algorithm/Find.h>

namespace ply {
namespace build {

PLY_NO_INLINE bool Dependency::addDependency(Visibility visibility, Dependency* dep) {
    // Check if it's already a dependency
    s32 i = find(this->dependencies, [&](const auto& p) { return p.second == dep; });
    if (i >= 0) {
        if (visibility == Visibility::Public) {
            this->dependencies[i].first = visibility;
        }
        return false;
    }

    this->dependencies.append(visibility, dep);
    return true;
}

template <typename T>
PLY_NO_INLINE void merge(Array<T>& dstItems, ArrayView<const T> srcItems) {
    for (const auto& srcItem : srcItems) {
        if (find(dstItems, srcItem) < 0) {
            dstItems.append(srcItem);
        }
    }
}

PLY_NO_INLINE void mergeDefines(Array<PreprocessorDefinition>& dstItems,
                                ArrayView<const PreprocessorDefinition> srcItems) {
    for (const auto& srcItem : srcItems) {
        s32 foundIndex =
            find(dstItems, [&](const PreprocessorDefinition& ppDef) {
                return ppDef.key == srcItem.key;
            });
        if (foundIndex >= 0) {
            if (dstItems[foundIndex].value != srcItem.value) {
                TargetError::log(String::format("Clashing definitions for \"{}\"", srcItem.key));
            }
        } else {
            dstItems.append(srcItem);
        }
    }
}

PLY_NO_INLINE void propagateDependencyProperties(Dependency* dep) {
    if (dep->hasBeenPropagated)
        return;

    BuildTarget* depTarget = dep->buildTarget;

    Array<Dependency*> buildDeps;
    buildDeps.append(dep);
    dep->visibleDeps.append(dep);

    for (const Tuple<Visibility, Dependency*>& child : dep->dependencies) {
        propagateDependencyProperties(child.second);

        if (child.first == Visibility::Public) {
            merge<Dependency*>(dep->visibleDeps, child.second->visibleDeps);
        }
        merge<Dependency*>(buildDeps, child.second->visibleDeps);

        BuildTarget* childTarget = child.second->buildTarget;

        // Merge includes
        if (depTarget) {
            merge<String>(depTarget->privateIncludeDirs, child.second->includeDirs);
            mergeDefines(depTarget->privateDefines, child.second->defines);
            merge<String>(depTarget->privateAbstractFlags, child.second->abstractFlags);
        }
        if (child.first == Visibility::Public) {
            merge<String>(dep->includeDirs, child.second->includeDirs);
            mergeDefines(dep->defines, child.second->defines);
            merge<String>(dep->abstractFlags, child.second->abstractFlags);
        }

        bool staticLink = true;
        if (depTarget && childTarget &&
            depTarget->sharedContainer != childTarget->sharedContainer) {
            staticLink = false;
        }

        if (staticLink) {
            merge<String>(dep->libs, child.second->libs);
            merge<String>(dep->dlls, child.second->dlls);
            merge<String>(dep->frameworks, child.second->frameworks);

            if (childTarget) {
                if (childTarget->targetType != BuildTargetType::HeaderOnly) {
                    String depName = childTarget->name;
                    if (childTarget->targetType == BuildTargetType::ObjectLib) {
                        depName = String::format("$<TARGET_OBJECTS:{}>", childTarget->name);
                    }
                    if (find(dep->libs, depName) < 0) {
                        dep->libs.append(std::move(depName));
                    }
                }
            }
        } else {
            PLY_ASSERT(childTarget->sharedContainer); // FIXME: Report an error instead of asserting
            HybridString ctrName = childTarget->sharedContainer->name;
            if (find(dep->libs, ctrName) < 0) {
                dep->libs.append(ctrName);
            }
        }
    }

    // Import/Export definitions
    if (depTarget) {
        for (Dependency* buildDep : buildDeps) {
            if (BuildTarget* bdTarget = buildDep->buildTarget) {
                if (bdTarget->sharedContainer) {
                    if (!bdTarget->dynamicLinkPrefix.isEmpty()) {
                        if (bdTarget->sharedContainer == depTarget->sharedContainer) {
                            depTarget->setPreprocessorDefinition(
                                Visibility::Private, bdTarget->dynamicLinkPrefix + "_EXPORTING",
                                "1");
                        } else {
                            depTarget->setPreprocessorDefinition(
                                Visibility::Private, bdTarget->dynamicLinkPrefix + "_IMPORTING",
                                "1");
                        }
                    }
                } else {
                    // FIXME: Report an error instead of asserting
                    PLY_ASSERT(!depTarget->sharedContainer);
                }
            }
        }
    }

    dep->hasBeenPropagated = true;
}

} // namespace build
} // namespace ply

#include "codegen/Dependency.inl" //%%
