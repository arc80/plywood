/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>
#include <ply-build-target/BuildTarget.h>

namespace ply {
namespace build {

struct ProjectInstantiator;
struct TargetInstantiator;

//------------------------------------------------------
// ModuleArgs
//------------------------------------------------------
struct ModuleArgs {
    ProjectInstantiator* projInst = nullptr;
    const TargetInstantiator* targetInst = nullptr;
    BuildTarget* buildTarget = nullptr;

    PLY_BUILD_ENTRY void addIncludeDir(Visibility visibility, StringView includeDir);
    PLY_BUILD_ENTRY void addSourceFiles(StringView sourcePath, bool recursive = true);
    PLY_BUILD_ENTRY void addSourceFilesWhenImported(StringView sourceRoot,
                                                    ArrayView<const StringView> relPaths);
    PLY_BUILD_ENTRY void setPrecompiledHeader(StringView generatorSource, StringView pchInclude);
    PLY_BUILD_ENTRY void addTarget(Visibility visibility, StringView targetName);
    PLY_BUILD_ENTRY void addExtern(Visibility visibility, StringView externName);
    PLY_INLINE void setPreprocessorDefinition(Visibility visibility, StringView key,
                                              StringView value) {
        this->buildTarget->setPreprocessorDefinition(visibility, key, value);
    }
};

} // namespace build
} // namespace ply
