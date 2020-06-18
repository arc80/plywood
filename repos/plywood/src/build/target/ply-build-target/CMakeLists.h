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
    // ply reflect off
};

struct CMakeBuildFolder {
    String solutionName;
    String absPath;
    Array<BuildTarget*> targets;
    String sourceFolderPrefix; // Mainly for generating the bootstrap CMakeLists.txt
    bool forBootstrap = false;
};

extern CMakeGeneratorOptions NativeToolchain;
extern String DefaultNativeConfig;
void writeCMakeLists(StringWriter* sw, CMakeBuildFolder* cbf);
bool cmakeBuildSystemExists(StringView cmakeListsFolder, const CMakeGeneratorOptions& generatorOpts,
                            String solutionName, StringView config);
Tuple<s32, String> generateCMakeProject(StringView cmakeListsFolder,
                                        const CMakeGeneratorOptions& generatorOpts,
                                        StringView config,
                                        Functor<void(StringView)> errorCallback = {});
Tuple<s32, String> buildCMakeProject(StringView cmakeListsFolder,
                                     const CMakeGeneratorOptions& generatorOpts, StringView config,
                                     StringView targetName, bool captureOutput);
String getTargetOutputPath(const BuildTarget* buildTarget, StringView buildFolderPath,
                           const CMakeGeneratorOptions& cmakeOptions, StringView config);

} // namespace build
} // namespace ply
