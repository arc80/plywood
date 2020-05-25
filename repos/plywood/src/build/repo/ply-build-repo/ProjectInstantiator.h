/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>
#include <ply-build-target/Dependency.h>
#include <ply-build-repo/ExternProvider.h>

namespace ply {
namespace build {

struct ProjectInstantiationEnv;
struct TargetInstantiator;
struct BuildTarget;
struct Repo;
struct DependencySource;

struct ProjectInstantiationResult {
    struct DependencyEntry {
        const DependencySource* depSrc = nullptr;
        Owned<Dependency> dep;
    };

    struct DependencyMapTraits {
        using Key = const DependencySource*;
        using Item = s32;
        using Context = Array<DependencyEntry>;
        static PLY_INLINE const Key& comparand(Item item, const Context& ctx) {
            return ctx[item].depSrc;
        }
    };

    bool isValid = true;
    HashMap<DependencyMapTraits> dependencyMap;
    Array<DependencyEntry> dependencies;
    Array<const DependencySource*> unselectedExterns;
    Array<const ExternProvider*> uninstalledProviders;
};

struct DependencyTree {
    String desc;
    Array<DependencyTree> children;
};

struct ProjectInstantiator {
    // These are constant during instantiation:
    const ProjectInstantiationEnv* env = nullptr;

    // This is used internally:
    BuildTarget* sharedContainer = nullptr;

    // This is modified during instantiation:
    ProjectInstantiationResult* result = nullptr;
    DependencyTree* depTreeNode = nullptr;

    Dependency* instantiate(const DependencySource* depSrc, bool isSharedRoot);
    void propagateAllDependencies();
};

void writeCMakeLists(StringWriter* sw, StringView solutionName, StringView buildFolderPath,
                     const ProjectInstantiationResult* instResult, bool forBootstrap);

void instantiatorError(StringView message);

} // namespace build
} // namespace ply
