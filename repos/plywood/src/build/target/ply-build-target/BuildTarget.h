/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>
#include <ply-build-target/Dependency.h>

namespace ply {
namespace build {

enum class BuildTargetType {
    HeaderOnly,
    Lib,
    ObjectLib,
    DLL,
    EXE,
};

struct BuildTarget : Dependency {
    struct SourceFilesPair {
        String root;
        Array<String> relFiles;
    };

    struct PrecompiledHeaderPair {
        String generatorSourcePath;
        String pchInclude;
    };

    BuildTargetType targetType = BuildTargetType::Lib;
    String name;
    BuildTarget* sharedContainer = nullptr;
    String dynamicLinkPrefix;
    Array<SourceFilesPair> sourceFiles;
    Array<SourceFilesPair> sourceFilesWhenImported;
    Array<String> privateIncludeDirs;
    Array<PreprocessorDefinition> privateDefines;
    Array<String> privateAbstractFlags;
    PrecompiledHeaderPair precompiledHeader;
    bool anySourceFiles = false;

    PLY_INLINE BuildTarget() : Dependency{DependencyType::Target} {
    }
    PLY_NO_INLINE void addIncludeDir(Visibility visibility, StringView absIncludeDir);
    PLY_NO_INLINE void addSourceFiles(StringView absSourcePath, bool recursive = true);
    PLY_NO_INLINE void addSourceFilesWhenImported(StringView absSourceRoot,
                                                  ArrayView<const StringView> relPaths);
    // Returns false if absGeneratorSource is not a source file:
    PLY_NO_INLINE bool setPrecompiledHeader(StringView absGeneratorSource, StringView pchInclude);
    PLY_BUILD_ENTRY bool setPreprocessorDefinition(Visibility visibility, StringView key,
                                                   StringView value);
};

} // namespace build
} // namespace ply
