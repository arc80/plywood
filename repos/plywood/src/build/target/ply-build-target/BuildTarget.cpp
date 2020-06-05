/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-target/BuildTarget.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-runtime/algorithm/Sort.h>

namespace ply {
namespace build {

PLY_NO_INLINE void BuildTarget::addIncludeDir(Visibility visibility, StringView absIncludeDir) {
    PLY_ASSERT(NativePath::isAbsolute(absIncludeDir) && NativePath::isNormalized(absIncludeDir));
    if (visibility == Visibility::Public) {
        this->includeDirs.append(absIncludeDir);
    }
    this->privateIncludeDirs.append(std::move(absIncludeDir));
}

PLY_NO_INLINE void BuildTarget::addSourceFiles(StringView absSourcePath, bool recursive) {
    PLY_ASSERT(NativePath::isAbsolute(absSourcePath) && NativePath::isNormalized(absSourcePath));
    Array<StringView> sourceExts = {".c", ".cpp"};
    Array<StringView> headerExts = {".h"};

    Array<String> relFiles;
    for (const WalkTriple& triple : FileSystem::native()->walk(absSourcePath)) {
        static Array<StringView> extensions = {".c", ".cpp", ".h"};
        for (const WalkTriple::FileInfo& file : triple.files) {
            if (file.name.endsWith(".modules.cpp")) // Skip *.modules.cpp files
                continue;
            String ext = NativePath::splitExt(file.name).second.lowerAsc();
            bool isSourceFile = (findItem(sourceExts.view(), ext) >= 0);
            if (isSourceFile) {
                this->anySourceFiles = true;
            }
            if (isSourceFile || (findItem(headerExts.view(), ext) >= 0)) {
                String fullPath = NativePath::join(triple.dirPath, file.name);
                relFiles.append(NativePath::makeRelative(absSourcePath, fullPath));
            }
        }
        if (!recursive)
            break;
    }
    if (!relFiles.isEmpty()) {
        sort(relFiles.view());
        this->sourceFiles.append({absSourcePath, std::move(relFiles)});
    }
}

PLY_NO_INLINE void BuildTarget::addSourceFilesWhenImported(StringView absSourceRoot,
                                                           ArrayView<const StringView> relPaths) {
    PLY_ASSERT(NativePath::isAbsolute(absSourceRoot) && NativePath::isNormalized(absSourceRoot));
    Array<StringView> sourceExts = {".c", ".cpp"};
    for (StringView relPath : relPaths) {
        String ext = NativePath::splitExt(relPath).second.lowerAsc();
        bool isSourceFile = (findItem(sourceExts.view(), ext) >= 0);
        if (isSourceFile) {
            this->anySourceFiles = true;
        }
    }
    this->sourceFilesWhenImported.append({absSourceRoot, Array<String>{relPaths}});
}

PLY_NO_INLINE bool BuildTarget::setPrecompiledHeader(StringView absGeneratorSource,
                                                     StringView pchInclude) {
    PLY_ASSERT(NativePath::isAbsolute(absGeneratorSource) &&
               NativePath::isNormalized(absGeneratorSource));
    bool foundGenerator = false;
    for (const SourceFilesPair& pair : this->sourceFiles) {
        for (StringView relFile : pair.relFiles) {
            if (NativePath::join(pair.root, relFile) == absGeneratorSource) {
                foundGenerator = true;
                break;
            }
        }
    }
    if (!foundGenerator) {
        return false;
    }
    this->precompiledHeader = {absGeneratorSource, pchInclude};
    return true;
}

PLY_NO_INLINE bool BuildTarget::setPreprocessorDefinition(Visibility visibility, StringView key,
                                                          StringView value) {
    bool wasNotDefined = true;
    s32 existingIndex = find(this->privateDefines.view(),
                             [&](const PreprocessorDefinition& ppDef) { return ppDef.key == key; });
    if (existingIndex >= 0) {
        wasNotDefined = false;
        this->privateDefines[existingIndex].value = value;
    } else {
        this->privateDefines.append({key, value});
    }

    if (visibility == Visibility::Public) {
        existingIndex = find(this->defines.view(),
                             [&](const PreprocessorDefinition& ppDef) { return ppDef.key == key; });
        if (existingIndex >= 0) {
            PLY_ASSERT(this->defines[existingIndex].value == value);
        } else {
            this->defines.append({key, value});
        }
    }
    return wasNotDefined;
}

} // namespace build
} // namespace ply
