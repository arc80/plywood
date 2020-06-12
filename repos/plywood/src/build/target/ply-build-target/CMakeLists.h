/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>

namespace ply {
namespace build {

struct BuildTarget;

struct CMakeGeneratorOptions {
    PLY_REFLECT()
    String generator;
    String platform;
    String toolset;
    // For multiconfig generators (Visual Studio, Xcode), buildType is the default build type.
    // For single-config generators (Unix Makefiles), it's the only build type
    String buildType;
    // ply reflect off

    PLY_INLINE bool isValid() const {
        return generator && buildType;
    }
};

struct CMakeBuildFolder {
    String solutionName;
    String absPath;
    Array<BuildTarget*> targets;
    String sourceFolderPrefix; // Mainly for generating the bootstrap CMakeLists.txt
    bool forBootstrap = false;
};

extern CMakeGeneratorOptions NativeToolchain;
void writeCMakeLists(StringWriter* sw, CMakeBuildFolder* cbf);
Tuple<s32, String> generateCMakeProject(StringView cmakeListsFolder,
                                        const CMakeGeneratorOptions& generatorOpts,
                                        Functor<void(StringView)> errorCallback = {});
Tuple<s32, String> buildCMakeProject(StringView cmakeListsFolder,
                                     const CMakeGeneratorOptions& generatorOpts,
                                     StringView buildType, bool captureOutput = true);
String getTargetOutputPath(const BuildTarget* buildTarget, StringView buildFolderPath,
                           const CMakeGeneratorOptions& cmakeOptions, StringView buildType = {});

} // namespace build
} // namespace ply
