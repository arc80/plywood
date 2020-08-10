/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>

namespace ply {
namespace build {

struct BuildTarget;

enum class Visibility {
    Private,
    Public,
};

enum class DynamicLinkage : u8 {
    // ply reflect enum
    None,
    Import,
    Export,
};
PLY_REFLECT_ENUM(, DynamicLinkage)

struct PreprocessorDefinition {
    String key;
    String value;
};

struct Dependency {
    Array<Tuple<Visibility, Dependency*>> dependencies;
    bool initialized = false;
    bool hasBeenPropagated = false;
    Array<Dependency*> visibleDeps;
    Array<String> includeDirs;
    Array<PreprocessorDefinition> defines;
    Array<String> libs;
    Array<String> dlls;
    Array<String> frameworks; // Used in Xcode
    Array<String> abstractFlags;
    Owned<BuildTarget> buildTarget;

    // Returns false if dep was already a dependency:
    PLY_BUILD_ENTRY bool addDependency(Visibility visibility, Dependency* dep);
};

PLY_NO_INLINE void propagateDependencyProperties(Dependency* dep);

enum class BuildTargetType {
    HeaderOnly,
    Lib,
    ObjectLib,
    DLL,
    EXE,
};

struct BuildTarget {
    struct SourceFilesPair {
        String root;
        Array<String> relFiles;
    };

    struct PrecompiledHeaderPair {
        String generatorSourcePath;
        String pchInclude;
    };

    Dependency* dep = nullptr;
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

    PLY_INLINE BuildTarget(Dependency* dep) : dep{dep} {
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
