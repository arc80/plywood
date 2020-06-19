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
#include <ply-build-provider/ExternFolderRegistry.h>
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
    env->config = this->activeConfig.view();
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

PLY_NO_INLINE bool BuildFolder::isGenerated(StringView config) const {
    String cmakeListsFolder = BuildFolderName::getFullPath(this->buildFolderName);
    if (!config) {
        config = this->activeConfig;
        if (!config) {
            ErrorHandler::log(
                ErrorHandler::Fatal,
                String::format(
                    "Active config is not set in folder '{}'. Try recreating the folder.\n",
                    this->buildFolderName));
        }
    }
    return cmakeBuildSystemExists(cmakeListsFolder, this->cmakeOptions, this->solutionName, config);
}

PLY_NO_INLINE bool BuildFolder::generate(StringView config,
                                         const ProjectInstantiationResult* instResult) const {
    ErrorHandler::log(ErrorHandler::Info,
                      String::format("Generating build system for '{}'...\n", this->solutionName));

    String buildFolderPath = BuildFolderName::getFullPath(this->buildFolderName);
    {
        StringWriter sw;
        writeCMakeLists(&sw, this->solutionName, buildFolderPath, instResult, false);
        String cmakeListsPath = NativePath::join(buildFolderPath, "CMakeLists.txt");
        FSResult result = FileSystem::native()->makeDirsAndSaveTextIfDifferent(
            cmakeListsPath, sw.moveToString(), TextFormat::platformPreference());
        if (result != FSResult::OK && result != FSResult::Unchanged) {
            ErrorHandler::log(ErrorHandler::Error,
                              String::format("Can't write '{}'\n", cmakeListsPath));
        }
    }

    if (!config) {
        config = this->activeConfig;
        if (!config) {
            ErrorHandler::log(
                ErrorHandler::Fatal,
                String::format(
                    "Active config is not set in folder '{}'. Try recreating the folder.\n",
                    this->buildFolderName));
        }
    }

    Tuple<s32, String> result =
        generateCMakeProject(buildFolderPath, this->cmakeOptions, config, [&](StringView errMsg) {
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

PLY_NO_INLINE bool BuildFolder::generateLoop(StringView config) const {
    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());
    PLY_SET_IN_SCOPE(ExternFolderRegistry::instance_, ExternFolderRegistry::create());
    PLY_SET_IN_SCOPE(HostTools::instance_, HostTools::create());

    for (;;) {
        ProjectInstantiationResult instResult = this->instantiateAllTargets(false);
        if (!instResult.isValid) {
            return false;
        }
        bool canGenerate = true;
        u32 numUnselected = instResult.unselectedExterns.numItems();
        if (numUnselected > 0) {
            canGenerate = false;
            StringWriter sw = StdOut::createStringWriter();
            for (const DependencySource* unselectedExtern : instResult.unselectedExterns) {
                sw.format("Can't generate build system in folder '{}' because extern '{}' is not "
                          "selected.\n",
                          this->buildFolderName,
                          RepoRegistry::get()->getShortDepSourceName(unselectedExtern));
                Array<Tuple<const ExternProvider*, bool>> candidates;
                for (const Repo* repo : RepoRegistry::get()->repos) {
                    for (const ExternProvider* externProvider : repo->externProviders) {
                        if (externProvider->extern_ != unselectedExtern)
                            continue;
                        ToolchainInfo toolchain = toolchainInfoFromCMakeOptions(this->cmakeOptions);
                        ExternProviderArgs args;
                        args.toolchain = &toolchain;
                        args.provider = externProvider;
                        ExternResult er = externProvider->externFunc(ExternCommand::Status, &args);
                        if (er.isSupported()) {
                            candidates.append({externProvider, er.code == ExternResult::Installed});
                        }
                    }
                }
                if (candidates.isEmpty()) {
                    sw.format("No compatible providers are available for extern '{}'.\n",
                              RepoRegistry::get()->getShortDepSourceName(unselectedExtern));
                } else {
                    u32 n = candidates.numItems();
                    sw.format("{} compatible provider{} available:\n", n, n == 1 ? " is" : "s are");
                    for (Tuple<const ExternProvider*, bool> pair : candidates) {
                        sw.format("    {} ({})\n",
                                  RepoRegistry::get()->getShortProviderName(pair.first),
                                  pair.second ? "installed" : "not installed");
                    }
                }
            }
        }
        if (instResult.uninstalledProviders.numItems() > 0) {
            canGenerate = false;
            StringWriter sw = StdOut::createStringWriter();
            for (const ExternProvider* prov : instResult.uninstalledProviders) {
                sw.format("Can't generate build system in folder '{}' because extern provider "
                          "'{}' is selected, but not installed.\n",
                          this->buildFolderName, RepoRegistry::get()->getShortProviderName(prov));
            }
        }
        if (canGenerate) {
            // Reinstantiate, but this time pass isGenerating = true:
            instResult = this->instantiateAllTargets(true);
            if (this->generate(config, &instResult)) {
                StdOut::createStringWriter().format(
                    "Successfully generated build system in folder '{}'.\n", this->buildFolderName);
                return true;
            }
        }
        break;
    }
    return false;
}

PLY_NO_INLINE bool BuildFolder::build(StringView config, StringView targetName,
                                      bool captureOutput) const {
    // Note: Should we check that targetName actually exists in the build folder before invoking
    // CMake? If targetName isn't a root target, this would require us to instaniate all
    // dependencies first.
    if (!config) {
        config = this->activeConfig;
        if (!config) {
            ErrorHandler::log(
                ErrorHandler::Fatal,
                String::format(
                    "Active config is not set in folder '{}'. Try recreating the folder.\n",
                    this->buildFolderName));
        }
    }
    ErrorHandler::log(ErrorHandler::Info,
                      String::format("Building {} configuration of '{}'...\n", config,
                                     targetName ? targetName : this->solutionName.view()));

    String cmakeListsFolder = BuildFolderName::getFullPath(this->buildFolderName);
    Tuple<s32, String> result =
        buildCMakeProject(cmakeListsFolder, this->cmakeOptions, config, targetName, captureOutput);
    if (result.first != 0) {
        ErrorHandler::log(ErrorHandler::Error,
                          String::format("Failed to build '{}':\n",
                                         targetName ? targetName : this->solutionName.view()) +
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
