/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-folder/BuildFolder.h>
#include <pylon/Parse.h>
#include <pylon/Write.h>
#include <pylon-reflect/Import.h>
#include <pylon-reflect/Export.h>
#include <ply-build-repo/RepoRegistry.h>
#include <ply-build-repo/ProjectInstantiator.h>
#include <ply-build-repo/ExternProvider.h>
#include <ply-build-repo/ErrorHandler.h>
#include <ply-build-repo/ProjectInstantiationEnv.h>
#include <ply-build-target/BuildTarget.h>
#include <ply-build-target/CMakeLists.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-build-provider/HostTools.h>
#include <ply-build-provider/ToolchainInfo.h>

namespace ply {
namespace build {

struct BuildFolderName {
    static PLY_NO_INLINE String getFullPath(StringView buildFolderName) {
        if (buildFolderName.isEmpty()) {
            return NativePath::join(PLY_WORKSPACE_FOLDER, "data/dummy_build_folder/");
        } else {
            return NativePath::join(PLY_WORKSPACE_FOLDER, "data/build", buildFolderName, "");
        }
    }

    static PLY_NO_INLINE String getInfoPath(StringView buildFolderName) {
        PLY_ASSERT(!buildFolderName.isEmpty());
        return NativePath::join(getFullPath(buildFolderName), "info.pylon");
    }
};

String BuildFolder::getAbsPath() const {
    return BuildFolderName::getFullPath(this->buildFolderName);
}

PLY_NO_INLINE Owned<BuildFolder> BuildFolder::create(StringView buildFolderName,
                                                     StringView solutionName) {
    BuildFolder* bf = new BuildFolder;
    bf->buildFolderName = buildFolderName;
    bf->solutionName = solutionName;
    // Don't save it yet
    return bf;
}

PLY_NO_INLINE Owned<BuildFolder> BuildFolder::load(StringView buildFolderName) {
    String infoPath = BuildFolderName::getInfoPath(buildFolderName);
    String strContents = FileSystem::native()->loadTextAutodetect(infoPath).first;
    if (FileSystem::native()->lastResult() != FSResult::OK)
        return nullptr;

    auto aRoot = pylon::Parser{}.parse(strContents);
    if (!aRoot.isValid())
        return nullptr;

    Owned<BuildFolder> info = pylon::import<BuildFolder>(aRoot);
    info->buildFolderName = buildFolderName;
    return info;
}

PLY_NO_INLINE bool BuildFolder::save() const {
    auto aRoot = pylon::exportObj(TypedPtr::bind(this));
    String strContents = pylon::toString(aRoot);
    String infoPath = BuildFolderName::getInfoPath(this->buildFolderName);
    FSResult rc = FileSystem::native()->makeDirsAndSaveTextIfDifferent(
        infoPath, strContents, TextFormat::platformPreference());
    return (rc == FSResult::OK || rc == FSResult::Unchanged);
}

ToolchainInfo toolchainInfoFromCMakeOptions(const CMakeGeneratorOptions& cmakeOpts) {
    ToolchainInfo toolchain;
    if (!cmakeOpts.generator) {
        // Generator name is allowed to be blank when generating the bootstrap
    } else if (cmakeOpts.generator.startsWith("Visual Studio")) {
        toolchain.compiler.name = "msvc";
        toolchain.targetPlatform.name = "windows";
        toolchain.crt.name = "msvc";
        toolchain.cpprt.name = "msvc";
        toolchain.arch = (cmakeOpts.platform == "Win32" ? "x86" : "x64");
    } else if (cmakeOpts.generator == "Xcode") {
        toolchain.compiler.name = "clang";
        toolchain.targetPlatform.name = "macos";
        toolchain.crt.name = "";
        toolchain.cpprt.name = "";
        toolchain.arch = "x64";
    } else if (cmakeOpts.generator == "Unix Makefiles") {
        // FIXME: Detect the actual compiler that will be used
        // FIXME: Add support for CMAKE_CXX_COMPILER and CMAKE_C_COMPILER
        toolchain.compiler.name = "gcc";
        // FIXME: Add support for cross-compilation
#if PLY_KERNEL_LINUX
        toolchain.targetPlatform.name = "linux";
#if PLY_CPU_X64
        toolchain.arch = "x64";
#elif PLY_CPU_X86
        toolchain.arch = "x86";
#else
        // FIXME: Handle more gracefully
        PLY_ASSERT(0); // Unsupported target platform
#endif
#elif PLY_TARGET_APPLE
        toolchain.targetPlatform.name = "macos";
#else
        // FIXME: Handle more gracefully
        PLY_ASSERT(0); // Unsupported target system
#endif
        toolchain.crt.name = "glibc";
        toolchain.cpprt.name = "libstdc++";
    } else {
        // FIXME: Handle more gracefully
        PLY_ASSERT(0); // Unsupported generator
    }
    return toolchain;
}

Owned<ProjectInstantiationEnv> BuildFolder::createEnvironment() const {
    Owned<ProjectInstantiationEnv> env = new ProjectInstantiationEnv;
    env->cmakeOptions = &this->cmakeOptions;
    env->toolchain = toolchainInfoFromCMakeOptions(this->cmakeOptions);
    env->buildFolderPath = BuildFolderName::getFullPath(this->buildFolderName);
    for (StringView selName : this->externSelectors) {
        const ExternProvider* prov = RepoRegistry::get()->getExternProvider(selName);
        if (!prov) {
            ErrorHandler::log(
                ErrorHandler::Error,
                String::format("Invalid extern provider '{}' selected in build folder '{}'\n",
                               selName, this->buildFolderName));
            env->isValid = false;
            return env;
        }
        env->externSelectors.append(prov);
    }
    return env;
}

PLY_NO_INLINE ProjectInstantiationResult
BuildFolder::instantiateAllTargets(bool isGenerating) const {
    Owned<ProjectInstantiationEnv> env = this->createEnvironment();
    env->isGenerating = isGenerating;
    ProjectInstantiationResult instResult;
    ProjectInstantiator projInst;
    projInst.env = env;
    projInst.result = &instResult;

    for (StringView targetName : this->rootTargets) {
        const TargetInstantiator* targetInst =
            RepoRegistry::get()->findTargetInstantiator(targetName);
        if (!targetInst) {
            ErrorHandler::log(ErrorHandler::Error,
                              String::format("Invalid root target '{}' in build folder '{}'\n",
                                             targetName, this->buildFolderName));
            instResult.isValid = false;
            return instResult;
        }
        bool makeShared = findItem(this->makeShared.view(), targetName) >= 0;
        projInst.instantiate(targetInst, makeShared);
    }

    projInst.propagateAllDependencies();
    return instResult;
}

PLY_NO_INLINE DependencyTree BuildFolder::buildDepTree() const {
    Owned<ProjectInstantiationEnv> env = this->createEnvironment();
    ProjectInstantiationResult instResult;
    ProjectInstantiator projInst;
    DependencyTree depTree;
    projInst.env = env;
    projInst.result = &instResult;
    projInst.depTreeNode = &depTree;

    for (StringView targetName : this->rootTargets) {
        const TargetInstantiator* targetInst =
            RepoRegistry::get()->findTargetInstantiator(targetName);
        /*
        FIXME: Report bad root targets somehow
        if (!targetInst) {
            ErrorHandler::log(ErrorHandler::Error,
                              String::format("Invalid root target '{}' in build folder '{}'\n",
                                             targetName, this->buildFolderName));
            instResult.isValid = false;
            return instResult;
        }
        */
        bool makeShared = findItem(this->makeShared.view(), targetName) >= 0;
        projInst.instantiate(targetInst, makeShared);
    }
    return depTree;
}

PLY_NO_INLINE bool BuildFolder::generate(const ProjectInstantiationResult* instResult) const {
    ErrorHandler::log(ErrorHandler::Info,
                      String::format("Generating build system for '{}'...\n", this->solutionName));

    String buildFolderPath = BuildFolderName::getFullPath(this->buildFolderName);
    {
        StringWriter sw;
        writeCMakeLists(&sw, this->solutionName, buildFolderPath, instResult, false);
        FSResult result = FileSystem::native()->makeDirsAndSaveTextIfDifferent(
            NativePath::join(buildFolderPath, "CMakeLists.txt"), sw.moveToString(),
            TextFormat::platformPreference());
        if (result != FSResult::OK && result != FSResult::Unchanged) {
            ErrorHandler::log(
                ErrorHandler::Error,
                String::format("Generating build system for '{}'...\n", this->solutionName));
        }
    }

    Tuple<s32, String> result =
        generateCMakeProject(buildFolderPath, this->cmakeOptions, [&](StringView errMsg) {
            ErrorHandler::log(ErrorHandler::Error, errMsg);
        });
    if (result.first != 0) {
        ErrorHandler::log(
            ErrorHandler::Error,
            String::format("Failed to generate build system for '{}':\n", this->solutionName) +
                result.second);
        return false;
    }
    return true;
}

PLY_NO_INLINE bool BuildFolder::build(StringView buildType, StringView targetName,
                                      bool captureOutput) const {
    // Note: Should we check that targetName actually exists in the build folder before invoking
    // CMake? If targetName isn't a root target, this would require us to instaniate all
    // dependencies first.
    ErrorHandler::log(ErrorHandler::Info, String::format("Building '{}'...\n", this->solutionName));

    String cmakeListsFolder = BuildFolderName::getFullPath(this->buildFolderName);
    Tuple<s32, String> result = buildCMakeProject(cmakeListsFolder, this->cmakeOptions, buildType,
                                                  targetName, captureOutput);
    if (result.first != 0) {
        ErrorHandler::log(ErrorHandler::Error,
                          String::format("Failed to build '{}':\n", this->solutionName) +
                              result.second);
        return false;
    }
    return true;
}

PLY_NO_INLINE Array<Owned<BuildFolder>> BuildFolder::getList() {
    Array<Owned<BuildFolder>> result;
    String buildFolderRoot = NativePath::join(PLY_WORKSPACE_FOLDER, "data/build");
    for (const DirectoryEntry& entry : FileSystem::native()->listDir(buildFolderRoot)) {
        if (!entry.isDir)
            continue;
        PLY_ASSERT(!entry.name.isEmpty());
        Owned<BuildFolder> folderInfo = BuildFolder::load(entry.name);
        if (!folderInfo)
            continue;
        result.append(std::move(folderInfo));
    }
    return result;
}

} // namespace build
} // namespace ply

#include "codegen/BuildFolder.inl" //%%
