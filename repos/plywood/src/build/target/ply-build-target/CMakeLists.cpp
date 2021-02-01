/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-target/CMakeLists.h>
#include <ply-build-target/Dependency.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-runtime/thread/Affinity.h>

namespace ply {
namespace build {

#include "ply-build-target/NativeToolchain.inl"

void writeCMakeLists(OutStream* outs, CMakeBuildFolder* cbf) {
    PLY_ASSERT(NativePath::isNormalized(cbf->absPath));
    PLY_ASSERT(NativePath::endsWithSep(cbf->absPath));

    HybridString sourceFolderPrefix = cbf->sourceFolderPrefix.view();
    if (!sourceFolderPrefix) {
        sourceFolderPrefix = NativePath::join(PLY_WORKSPACE_FOLDER, "repos/plywood/src/");
    }

    // Define lambda expression that will filter filenames when generating CMakeLists.txt in
    // bootstrap mode:
    auto filterPath = [&](StringView filePath) -> HybridString {
        PLY_ASSERT(NativePath::isNormalized(filePath));
        if (cbf->forBootstrap) {
            if (filePath.startsWith(sourceFolderPrefix)) {
                return StringView{"${SRC_FOLDER}"} +
                       PosixPath::from<NativePath>(filePath.subStr(sourceFolderPrefix.numBytes));
            } else if (filePath.startsWith(cbf->absPath)) {
                return StringView{"${BUILD_FOLDER}"} +
                       PosixPath::from<NativePath>(filePath.subStr(cbf->absPath.numBytes));
            }
        }
        return PosixPath::from<NativePath>(filePath);
    };

    // CMakeLists 3.8 supports generator expressions such as "$<CONFIG>", which is used in
    // COMPILE_FLAGS source property on Win32
    *outs << "cmake_minimum_required(VERSION 3.8)\n";
    *outs << "set(CMAKE_CONFIGURATION_TYPES \"Debug;RelWithAsserts;RelWithDebInfo\" CACHE "
           "INTERNAL "
           "\"Build configs\")\n";
    outs->format("project({})\n", cbf->solutionName);
    if (cbf->forBootstrap) {
        *outs << R"(set(WORKSPACE_FOLDER "<<<WORKSPACE_FOLDER>>>")
set(SRC_FOLDER "<<<SRC_FOLDER>>>")
set(BUILD_FOLDER "<<<BUILD_FOLDER>>>")
include("${CMAKE_CURRENT_LIST_DIR}/Helper.cmake")
)";
    } else {
        outs->format("include(\"{}\")\n",
                   fmt::EscapedString(PosixPath::from<NativePath>(NativePath::join(
                       PLY_WORKSPACE_FOLDER, "repos/plywood/scripts/Helper.cmake"))));
    }

    // Iterate over all targets
    for (const Dependency* dep : cbf->targets) {
        BuildTarget* buildTarget = dep->buildTarget;
        PLY_ASSERT(buildTarget);
        /* FIXME
        if (buildTarget->key.dynLinkage == DynamicLinkage::Import &&
            buildTarget->sourceFilesWhenImported.isEmpty())
            continue;
        */

        StringView uniqueTargetName = buildTarget->name;
        outs->format("\n# {}\n", uniqueTargetName);

        // Define a CMake variable for each group of source files (usually, there's just one
        // group)
        Array<String> sourceVarNames;
        ArrayView<const BuildTarget::SourceFilesPair> sourceFiles = buildTarget->sourceFiles.view();
        /* FIXME
        if (buildTarget->key.dynLinkage == DynamicLinkage::Import) {
            sourceFiles = buildTarget->sourceFilesWhenImported.view();
        }
        */
        for (const BuildTarget::SourceFilesPair& sfPair : sourceFiles) {
            String varName = uniqueTargetName.upperAsc() + "_SOURCES";
            sourceVarNames.append(varName);
            outs->format("SetSourceFolders({} \"{}\"\n", varName,
                       fmt::EscapedString(filterPath(sfPair.root)));
            for (StringView relPath : sfPair.relFiles) {
                outs->format("    \"{}\"\n", fmt::EscapedString(filterPath(relPath)));
            }
            *outs << ")\n";
        }

        // Add this target
        BuildTargetType targetType = buildTarget->targetType;
        switch (targetType) {
            case BuildTargetType::HeaderOnly: {
                outs->format("add_custom_target({} SOURCES\n", uniqueTargetName);
                break;
            }
            case BuildTargetType::Lib: {
                outs->format("add_library({}\n", uniqueTargetName);
                break;
            }
            case BuildTargetType::ObjectLib: {
                // OBJECT libraries ensure that __declspec(dllexport) works correctly (eg. for
                // PLY_DLL_ENTRY). OBJECT libraries pass individual .obj files to the linker
                // instead of .lib files. If we use .lib files instead, some DLL exports may
                // get dropped if there are no references to the .obj where the export is
                // defined:
                outs->format("add_library({} OBJECT\n", uniqueTargetName);
                break;
            }
            case BuildTargetType::DLL: {
                outs->format("add_library({} SHARED\n", uniqueTargetName);
                break;
            }
            case BuildTargetType::EXE: {
                outs->format("add_executable({}\n", uniqueTargetName);
                break;
            }
        }
        for (StringView varName : sourceVarNames) {
            outs->format("    ${{{}}}\n", varName);
        }
        if (targetType == BuildTargetType::DLL || targetType == BuildTargetType::EXE) {
            // Use TARGET_OBJECTS generator expression to support OBJECT libraries
            for (s32 i = dep->libs.numItems() - 1; i >= 0; i--) {
                StringView lib = dep->libs[i];
                if (lib.startsWith("$<TARGET_OBJECTS")) {
                    outs->format("    {}\n", lib);
                }
            }
        }
        *outs << ")\n";
        if (targetType == BuildTargetType::EXE) {
            outs->format("set_property(TARGET {} PROPERTY ENABLE_EXPORTS TRUE)\n", uniqueTargetName);
        }

        // Precompiled headers
        if (!buildTarget->precompiledHeader.pchInclude.isEmpty()) {
            for (StringView varName : sourceVarNames) {
                outs->format("SetPrecompiledHeader({} {}\n    \"{}\"\n    \"{}\"\n    \"{}\"\n)\n",
                           uniqueTargetName, varName,
                           filterPath(buildTarget->precompiledHeader.generatorSourcePath),
                           buildTarget->precompiledHeader.pchInclude,
                           uniqueTargetName + ".$<CONFIG>.pch");
            }
        }

        // Enable/disable C++ execptions
        if (targetType != BuildTargetType::HeaderOnly) {
            bool enableExceptions =
                (findItem(buildTarget->privateAbstractFlags.view(), StringView{"exceptions"}) >= 0);
            outs->format("EnableCppExceptions({} {})\n", uniqueTargetName,
                       enableExceptions ? "TRUE" : "FALSE");
        }

        // Include directories
        if (targetType != BuildTargetType::HeaderOnly) {
            // FIXME: Add ArrayView::reversed() iterator?
            outs->format("target_include_directories({} PRIVATE\n", uniqueTargetName);
            for (s32 i = buildTarget->privateIncludeDirs.numItems() - 1; i >= 0; i--) {
                outs->format("    \"{}\"\n", filterPath(buildTarget->privateIncludeDirs[i]));
            }
            *outs << ")\n";
        }

        // Defines
        if (targetType != BuildTargetType::HeaderOnly &&
            buildTarget->privateDefines.numItems() > 0) {
            outs->format("target_compile_definitions({} PRIVATE\n", uniqueTargetName);
            for (const PreprocessorDefinition& define : buildTarget->privateDefines) {
                PLY_ASSERT(define.key.findByte('=') < 0);
                PLY_ASSERT(define.value.findByte('=') < 0);
                outs->format("    \"{}={}\"\n", define.key, define.value);
            }
            *outs << ")\n";
        }

        if (targetType == BuildTargetType::DLL || targetType == BuildTargetType::EXE) {
            // Define a CMake variable for each macOS framework
            Array<String> frameworkVars;
            for (StringView fw : dep->frameworks) {
                String fwVar = fw.upperAsc() + "_FRAMEWORK";
                frameworkVars.append(fwVar);
                outs->format("find_library({} {})\n", fwVar, fw);
            }

            // Link libraries
            // List in reserve order so that dependencies follow dependents
            if (dep->libs.numItems() > 0 || frameworkVars.numItems() > 0) {
                outs->format("target_link_libraries({} PRIVATE\n", uniqueTargetName);
                for (s32 i = dep->libs.numItems() - 1; i >= 0; i--) {
                    StringView lib = dep->libs[i];
                    if (!lib.startsWith("$<TARGET_OBJECTS")) {
                        if (lib.startsWith("${")) {
                            outs->format("    {}\n", lib);
                        } else {
                            outs->format("    \"{}\"\n", filterPath(lib));
                        }
                    }
                }
                for (StringView fwVar : frameworkVars) {
                    outs->format("    ${{{}}}\n", fwVar);
                }
                *outs << ")\n";
            }

            // FIXME: SafeSEH

            // Copy DLLs
            if (dep->dlls.numItems() > 0) {
                outs->format("AddDLLCopyStep({}\n", uniqueTargetName);
                for (s32 i = dep->dlls.numItems() - 1; i >= 0; i--) {
                    outs->format("    \"{}\"\n", PosixPath::from<NativePath>(dep->dlls[i]));
                }
                *outs << ")\n";
            }

            // In bootstrap_CMakeLists.txt, add a post-build command that copies PlyTool to the
            // workspace root.
            if (cbf->forBootstrap && buildTarget->name == "plytool") {
                *outs << "add_custom_command(TARGET plytool POST_BUILD COMMAND\n";
                *outs << "   ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:plytool> "
                       "\"${WORKSPACE_FOLDER}\")\n";
            }
        }
    }
}

bool isMultiConfigCMakeGenerator(StringView generator) {
    if (generator.startsWith("Visual Studio")) {
        return true;
    } else if (generator == "Xcode") {
        return true;
    } else if (generator == "Unix Makefiles") {
        return false;
    } else {
        // FIXME: Make this a not-fatal warning instead, perhaps logging to some kind of
        // thread-local variable that can be set in the caller's scope.
        PLY_ASSERT(0); // Unrecognized generator
        return false;
    }
}

PLY_NO_INLINE bool cmakeBuildSystemExists(StringView cmakeListsFolder,
                                          const CMakeGeneratorOptions& generatorOpts,
                                          String solutionName, StringView config) {
    PLY_ASSERT(generatorOpts.generator);
    PLY_ASSERT(config);
    if (generatorOpts.generator.startsWith("Visual Studio")) {
        String slnPath = NativePath::join(cmakeListsFolder, "build", solutionName + ".sln");
        return FileSystem::native()->exists(slnPath) == ExistsResult::File;
    } else if (generatorOpts.generator == "Xcode") {
        String projPath = NativePath::join(cmakeListsFolder, "build", solutionName + ".xcodeproj");
        return FileSystem::native()->exists(projPath) == ExistsResult::Directory;
    } else if (generatorOpts.generator == "Unix Makefiles") {
        String makefilePath = NativePath::join(cmakeListsFolder, "build", config, "Makefile");
        return FileSystem::native()->exists(makefilePath) == ExistsResult::File;
    } else {
        PLY_ASSERT(0); // Unrecognized generator
        return false;
    }
}

PLY_NO_INLINE Tuple<s32, String> generateCMakeProject(StringView cmakeListsFolder,
                                                      const CMakeGeneratorOptions& generatorOpts,
                                                      StringView config,
                                                      Functor<void(StringView)> errorCallback) {
    PLY_ASSERT(generatorOpts.generator);
    PLY_ASSERT(config);
    String buildFolder = NativePath::join(cmakeListsFolder, "build");
    String relPathToCMakeLists = "..";
    bool isMultiConfig = isMultiConfigCMakeGenerator(generatorOpts.generator);
    if (!isMultiConfig) {
        buildFolder = NativePath::join(buildFolder, config);
        relPathToCMakeLists = "../..";
    }
    FSResult result = FileSystem::native()->makeDirs(buildFolder);
    if (result != FSResult::OK && result != FSResult::AlreadyExists) {
        if (errorCallback) {
            errorCallback.call(String::format("Can't create folder '{}'\n", buildFolder));
        }
        return {-1, ""};
    }
    PLY_ASSERT(!generatorOpts.generator.isEmpty());
    Array<String> args = {relPathToCMakeLists, "-G", generatorOpts.generator};
    if (generatorOpts.platform) {
        args.extend({"-A", generatorOpts.platform});
    }
    if (generatorOpts.toolset) {
        args.extend({"-T", generatorOpts.toolset});
    }
    if (generatorOpts.toolchainFile == "ios") {
        // FIXME: Verify that we're using CMake version 3.14 or higher
        args.append("-DCMAKE_SYSTEM_NAME=iOS");
    }
    if (!isMultiConfig) {
        args.append(String::format("-DCMAKE_BUILD_TYPE={}", config));
    }
    args.extend({"-DCMAKE_C_COMPILER_FORCED=1", "-DCMAKE_CXX_COMPILER_FORCED=1"});
    Owned<Subprocess> sub = Subprocess::exec(PLY_CMAKE_PATH, Array<StringView>{args.view()}.view(),
                                             buildFolder, Subprocess::Output::openMerged());
    String output = TextFormat::platformPreference()
                        .createImporter(Owned<InStream>::create(sub->readFromStdOut.borrow()))
                        ->readRemainingContents();
    s32 rc = sub->join();
    if (rc != 0) {
        if (errorCallback) {
            errorCallback.call(String::format(
                "Error generating build system using CMake for folder '{}'\n", buildFolder));
        }
    }
    return {rc, std::move(output)};
} // namespace build

PLY_NO_INLINE Tuple<s32, String> buildCMakeProject(StringView cmakeListsFolder,
                                                   const CMakeGeneratorOptions& generatorOpts,
                                                   StringView config, StringView targetName,
                                                   bool captureOutput) {
    PLY_ASSERT(generatorOpts.generator);
    PLY_ASSERT(config);
    String buildFolder = NativePath::join(cmakeListsFolder, "build");
    bool isMultiConfig = isMultiConfigCMakeGenerator(generatorOpts.generator);
    if (!isMultiConfig) {
        buildFolder = NativePath::join(buildFolder, config);
    }
    Subprocess::Output outputType =
        captureOutput ? Subprocess::Output::openMerged() : Subprocess::Output::inherit();
    Owned<Subprocess> sub;
    if (generatorOpts.generator == "Unix Makefiles") {
        Array<HybridString> args = {};
        u32 hwThreads = Affinity{}.getNumHWThreads();
        if (hwThreads > 1) {
            args.extend({"-j", String::from(hwThreads)});
        }
        if (targetName) {
            args.append(targetName);
        }
        sub = Subprocess::exec("make", Array<StringView>{args.view()}.view(), buildFolder,
                               outputType);
    } else {
        Array<StringView> args = {"--build", "."};
        if (isMultiConfig) {
            args.extend({"--config", config});
        }
        if (targetName) {
            args.extend({"--target", targetName});
        }
        sub = Subprocess::exec(PLY_CMAKE_PATH, args.view(), buildFolder, outputType);
    }
    String output;
    if (captureOutput) {
        output = TextFormat::platformPreference()
                     .createImporter(Owned<InStream>::create(sub->readFromStdOut.borrow()))
                     ->readRemainingContents();
    }
    return {sub->join(), std::move(output)};
}

String getTargetOutputPath(BuildTargetType targetType, StringView targetName,
                           StringView buildFolderPath, StringView config) {
    PLY_ASSERT(config);

    // FIXME: The following logic assumes we're always using a native toolchain. In order to make it
    // work with cross-compilers, we'll need to pass in more information about the target platform,
    // perhaps using ToolchainInfo. (In that case, the real question will be, in general, how to
    // initialize that ToolchainInfo.)
    StringView filePrefix;
    StringView fileExtension;
    if (targetType == BuildTargetType::EXE) {
#if PLY_TARGET_WIN32
        fileExtension = ".exe";
#endif
    } else if (targetType == BuildTargetType::DLL) {
#if PLY_TARGET_WIN32
        fileExtension = ".dll";
#elif PLY_TARGET_APPLE
        filePrefix = "lib";
        fileExtension = ".dylib";
#else
        filePrefix = "lib";
        fileExtension = ".so";
#endif
    } else if (targetType == BuildTargetType::DLL) {
#if PLY_TARGET_WIN32
        fileExtension = ".lib";
#else
        filePrefix = "lib";
        fileExtension = ".a";
#endif
    } else {
        PLY_ASSERT(0); // Not supported
    }

    // Compose full path to the target output:
    Array<StringView> pathComponents = {buildFolderPath, "build", config};
    String fullName = filePrefix + targetName + fileExtension;
    pathComponents.append(fullName);
    return NativePath::format().joinAndNormalize(Array<StringView>{pathComponents.view()}.view());
}

} // namespace build
} // namespace ply

#include "codegen/CMakeLists.inl" //%%
