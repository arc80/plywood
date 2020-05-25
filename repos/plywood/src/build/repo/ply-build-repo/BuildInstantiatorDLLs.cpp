/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-repo/BuildInstantiatorDLLs.h>
#include <ply-build-repo/ExtractInstantiatorFunctions.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-build-target/CMakeLists.h>
#include <ply-build-target/BuildTarget.h>
#include <ply-build-repo/ErrorHandler.h>

namespace ply {
namespace build {

Array<InstantiatedDLL> buildInstantiatorDLLs(const CMakeGeneratorOptions& cmakeOptions) {
    PLY_ASSERT(cmakeOptions.isValid());

    ErrorHandler::log(ErrorHandler::Info, "Initializing repo registry...\n");
    String repoRootFolder = NativePath::join(PLY_WORKSPACE_FOLDER, "repos/");
    String dllBuildFolder = NativePath::join(PLY_WORKSPACE_FOLDER, "data/build/bootstrap/DLLs/");
    String backedUpSrcFolder = NativePath::join(PLY_WORKSPACE_FOLDER, "repos/plywood/src/");
#ifdef PLY_SRC_FOLDER
    backedUpSrcFolder = NativePath::normalize(PLY_SRC_FOLDER, "");
#endif

    Array<String> repoNames;
    for (const DirectoryEntry& entry : FileSystem::native()->listDir(repoRootFolder, 0)) {
        if (!entry.isDir)
            continue;

        // This is a separate repo.
        // Make a separate instantiator DLL just for it.
        // First, recursively find all files named Instantiators.inl:
        String repoFolder = NativePath::join(repoRootFolder, entry.name);
        Array<InstantiatorInlFile> inlFiles;
        for (const WalkTriple& triple : FileSystem::native()->walk(repoFolder)) {
            if (find(triple.files.view(), [](const WalkTriple::FileInfo& file) {
                    return file.name == "Instantiators.inl";
                }) < 0)
                continue;

            // Found one. Scan it and find all instantiator functions.
            InstantiatorInlFile inlFile;
            inlFile.absPath = NativePath::join(triple.dirPath, "Instantiators.inl");
            if (extractInstantiatorFunctions(&inlFile)) {
                inlFiles.append(std::move(inlFile));
            }
        }

        if (inlFiles.isEmpty())
            continue;

        // Generate .cpp file to include all the .inls
        StringWriter sw;
        sw << "#include <ply-runtime/algorithm/Find.h>\n";
        sw << "#include <ply-build-target/BuildTarget.h>\n";
        sw << "#include <ply-build-repo/TargetInstantiatorArgs.h>\n";
        sw << "#include <ply-build-repo/ProjectInstantiator.h>\n";
        sw << "#include <ply-build-repo/ProjectInstantiationEnv.h>\n";
        sw << "#include <ply-build-repo/Repo.h>\n";
        sw << "#include <ply-build-provider/ExternFolderRegistry.h>\n";
        sw << "#include <ply-build-provider/ExternHelpers.h>\n";
        sw << "#include <ply-build-provider/HostTools.h>\n";
        sw << "#include <ply-build-provider/PackageManager.h>\n";
        sw << "#include <ply-build-provider/ToolchainInfo.h>\n";
        sw << "\n";
        sw << "using namespace ply;\n";
        sw << "using namespace ply::build;\n";
        sw << "\n";

        for (const InstantiatorInlFile& inlFile : inlFiles) {
            sw.format("#include \"{}\"\n", PosixPath::from<NativePath>(inlFile.absPath));
        }
        sw << "\nextern \"C\" PLY_DLL_EXPORT void registerInstantiators(Repo* repo) "
              "{\n";
        for (const InstantiatorInlFile& inlFile : inlFiles) {
            for (const InstantiatorInlFile::TargetFunc& targetFunc : inlFile.targetFuncs) {
                sw.format("    repo->addTargetInstantiator(new TargetInstantiator{{\"{}\", \"{}\", "
                          "repo, {}, \"{}\"}});\n",
                          fmt::EscapedString(targetFunc.targetName),
                          fmt::EscapedString(NativePath::split(inlFile.absPath).first),
                          targetFunc.funcName, fmt::EscapedString(targetFunc.dynamicLinkPrefix));
            }
            for (const InstantiatorInlFile::ExternFunc& externFunc : inlFile.externFuncs) {
                sw.format("    repo->addExternProvider(\"{}\", {});\n",
                          fmt::EscapedString(externFunc.providerName), externFunc.externFunc);
            }
        }
        sw << "}\n";

        String generatedCppPath =
            NativePath::join(dllBuildFolder, "codegen", entry.name + "_Instantiators.cpp");
        FileSystem::native()->makeDirsAndSaveTextIfDifferent(generatedCppPath, sw.moveToString(),
                                                             TextFormat::platformPreference());
        repoNames.append(entry.name);
    }

    CMakeBuildFolder cbf;
    cbf.solutionName = "DLLs";
    cbf.absPath = dllBuildFolder;
    Array<Owned<BuildTarget>> ownedTargets;
    for (StringView repoName : repoNames) {
        BuildTarget* target = new BuildTarget;
        target->name = repoName + "_Instantiators";
        target->targetType = BuildTargetType::DLL;
        target->sourceFiles.append(
            {NativePath::join(dllBuildFolder, "codegen"), {repoName + "_Instantiators.cpp"}});
        target->sourceFiles.append(
            {NativePath::join(backedUpSrcFolder, "runtime/ply-runtime/memory"), {"Heap.cpp"}});
        target->addIncludeDir(Visibility::Private,
                              NativePath::join(PLY_BUILD_FOLDER, "codegen/ply-platform"));
        target->addIncludeDir(Visibility::Private, NativePath::join(backedUpSrcFolder, "platform"));
        target->addIncludeDir(Visibility::Private, NativePath::join(backedUpSrcFolder, "runtime"));
        target->addIncludeDir(Visibility::Private, NativePath::join(backedUpSrcFolder, "reflect"));
        target->addIncludeDir(Visibility::Private,
                              NativePath::join(backedUpSrcFolder, "build/target"));
        target->addIncludeDir(Visibility::Private,
                              NativePath::join(backedUpSrcFolder, "build/provider"));
        target->addIncludeDir(Visibility::Private,
                              NativePath::join(backedUpSrcFolder, "build/repo"));
        target->addIncludeDir(Visibility::Private,
                              NativePath::join(backedUpSrcFolder, "build/common"));
        target->setPreprocessorDefinition(Visibility::Private, "PLY_BUILD_IMPORTING", "1");
        target->setPreprocessorDefinition(Visibility::Private, "PLY_DLL_IMPORTING", "1");
#if PLY_TARGET_WIN32
        target->libs.append(NativePath::join(PLY_BUILD_FOLDER, "build/Debug/plytool.lib"));
#endif
        ownedTargets.append(target);
        cbf.targets.append(target);
    }
    StringWriter sw;
    writeCMakeLists(&sw, &cbf);
    String cmakeListsPath = NativePath::join(dllBuildFolder, "CMakeLists.txt");
    FSResult result = FileSystem::native()->makeDirsAndSaveTextIfDifferent(
        cmakeListsPath, sw.moveToString(), TextFormat::platformPreference());
    if (result != FSResult::OK && result != FSResult::Unchanged) {
        ErrorHandler::log(ErrorHandler::Error,
                          String::format("Can't write '{}'\n", cmakeListsPath));
        return {};
    }

    Tuple<s32, String> cmakeResult =
        generateCMakeProject(dllBuildFolder, cmakeOptions, [&](StringView errMsg) {
            ErrorHandler::log(ErrorHandler::Error, errMsg);
        });
    if (cmakeResult.first != 0) {
        ErrorHandler::log(ErrorHandler::Error,
                          StringView{"Failed to generate build system for instantiator DLLs:\n"} +
                              cmakeResult.second);
        return {};
    }

    cmakeResult = buildCMakeProject(dllBuildFolder, cmakeOptions);
    if (cmakeResult.first != 0) {
        ErrorHandler::log(ErrorHandler::Error,
                          StringView{"Failed to build instantiator DLLs:\n"} + cmakeResult.second);
        return {};
    }

    Array<InstantiatedDLL> idlls;
    for (StringView repoName : repoNames) {
        InstantiatedDLL& idll = idlls.append();
        idll.repoName = repoName;
#if PLY_TARGET_WIN32
        idll.dllPath = NativePath::join(dllBuildFolder, "build", cmakeOptions.buildType,
                                        repoName + "_Instantiators.dll");
#elif PLY_TARGET_APPLE
        idll.dllPath = NativePath::join(dllBuildFolder, "build", cmakeOptions.buildType,
                                        StringView{"lib"} + repoName + "_Instantiators.dylib");
#else
        idll.dllPath = NativePath::join(dllBuildFolder, "build",
                                        StringView{"lib"} + repoName + "_Instantiators.so");
#endif
    }
    return idlls;
}

} // namespace build
} // namespace ply
