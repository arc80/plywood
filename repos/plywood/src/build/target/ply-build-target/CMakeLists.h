/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>

namespace ply {
namespace build {

struct Dependency;
enum class BuildTargetType;

struct CMakeGeneratorOptions {
    PLY_REFLECT()
    String generator;     // passed directly to -G
    String platform;      // passed directly to -A
    String toolset;       // passed directly to -T
    String toolchainFile; // currently "ios" or blank
    // ply reflect off

    template <typename Hasher>
    void appendTo(Hasher& h) const {
        h.append(this->generator.bufferView());
        h.append(this->platform.bufferView());
        h.append(this->toolset.bufferView());
        h.append(this->toolchainFile.bufferView());
    }
};

struct CMakeBuildFolder {
    String solutionName;
    String absPath;
    Array<Dependency*> targets;
    String sourceFolderPrefix; // Mainly for generating the bootstrap CMakeLists.txt
    bool forBootstrap = false;
};

extern CMakeGeneratorOptions NativeToolchain;
extern String DefaultNativeConfig;
void writeCMakeLists(StringWriter* sw, CMakeBuildFolder* cbf);
bool isMultiConfigCMakeGenerator(StringView generator);
bool cmakeBuildSystemExists(StringView cmakeListsFolder, const CMakeGeneratorOptions& generatorOpts,
                            String solutionName, StringView config);
Tuple<s32, String> generateCMakeProject(StringView cmakeListsFolder,
                                        const CMakeGeneratorOptions& generatorOpts,
                                        StringView config,
                                        Functor<void(StringView)> errorCallback = {});
Tuple<s32, String> buildCMakeProject(StringView cmakeListsFolder,
                                     const CMakeGeneratorOptions& generatorOpts, StringView config,
                                     StringView targetName, bool captureOutput);
String getTargetOutputPath(BuildTargetType targetType, StringView targetName,
                           StringView buildFolderPath, StringView config);

} // namespace build
} // namespace ply
