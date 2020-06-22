/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-repo/BuildInstantiatorDLLs.h>
#include <ply-build-repo/ExtractModuleFunctions.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-runtime/container/Hash128.h>
#include <ply-build-target/CMakeLists.h>
#include <ply-build-target/BuildTarget.h>
#include <ply-build-repo/ErrorHandler.h>
#include <ply-build-repo/RepoRegistry.h>
#include <pylon/Parse.h>
#include <pylon/Write.h>
#include <pylon-reflect/Import.h>
#include <pylon-reflect/Export.h>

namespace ply {
namespace build {

struct GenerateDLLContext {
    String repoRootFolder;
    String dllBuildFolder;
    String backedUpSrcFolder;
};

struct ExtractedRepo {
    String repoName;
    Array<ModuleDefinitionFile> modDefFiles;
};

struct DLLSignature {
    PLY_REFLECT()
    String signature;
    // ply reflect off

    PLY_NO_INLINE bool load(StringView absPath) {
        String strContents = FileSystem::native()->loadTextAutodetect(absPath).first;
        if (FileSystem::native()->lastResult() != FSResult::OK)
            return false;

        auto aRoot = pylon::Parser{}.parse(strContents);
        if (!aRoot.isValid())
            return false;

        pylon::importInto(TypedPtr::bind(this), aRoot);
        return true;
    }

    PLY_NO_INLINE bool save(StringView absPath) const {
        auto aRoot = pylon::exportObj(TypedPtr::bind(this));
        String strContents = pylon::toString(aRoot);
        FSResult rc = FileSystem::native()->makeDirsAndSaveTextIfDifferent(
            absPath, strContents, TextFormat::platformPreference());
        return (rc == FSResult::OK || rc == FSResult::Unchanged);
    }
};

Owned<BuildTarget> createDLLTarget(const GenerateDLLContext& ctx, const ExtractedRepo& exRepo) {
    // Generate .cpp file to include all the .inls
    StringWriter sw;
    sw << "#include <ply-build-repo/Repo.h>\n";
    sw << "\n";
    sw << "using namespace ply::build;\n";
    sw << "\n";

    for (const ModuleDefinitionFile& modDefFile : exRepo.modDefFiles) {
        for (const ModuleDefinitionFile::ModuleFunc& moduleFunc : modDefFile.moduleFuncs) {
            sw.format("void {}(ModuleArgs* args);\n", moduleFunc.funcName);
        }
        for (const ModuleDefinitionFile::ExternProviderFunc& externProviderFunc :
             modDefFile.externProviderFuncs) {
            sw.format("ExternResult {}(ExternCommand cmd, ExternProviderArgs* args);\n",
                      externProviderFunc.funcName);
        }
    }

    sw << "\nextern \"C\" PLY_DLL_EXPORT void registerInstantiators(Repo* repo) "
          "{\n";
    for (const ModuleDefinitionFile& modDefFile : exRepo.modDefFiles) {
        for (const ModuleDefinitionFile::ModuleFunc& moduleFunc : modDefFile.moduleFuncs) {
            sw.format("    repo->addTargetInstantiator(new TargetInstantiator{{\"{}\", \"{}\", "
                      "repo, {}}});\n",
                      fmt::EscapedString(moduleFunc.moduleName),
                      fmt::EscapedString(NativePath::split(modDefFile.absPath).first),
                      moduleFunc.funcName);
        }
        for (const ModuleDefinitionFile::ExternProviderFunc& externProviderFunc :
             modDefFile.externProviderFuncs) {
            sw.format("    repo->addExternProvider(\"{}\", \"{}\", {});\n",
                      fmt::EscapedString(externProviderFunc.externName),
                      fmt::EscapedString(externProviderFunc.providerName),
                      externProviderFunc.funcName);
        }
    }
    sw << "}\n";

    String generatedCppPath =
        NativePath::join(ctx.dllBuildFolder, "codegen", exRepo.repoName + "_Instantiators.cpp");
    FileSystem::native()->makeDirsAndSaveTextIfDifferent(generatedCppPath, sw.moveToString(),
                                                         TextFormat::platformPreference());

    Owned<BuildTarget> target = new BuildTarget;
    target->name = exRepo.repoName + "_Instantiators";
    target->targetType = BuildTargetType::DLL;
    {
        // Add all *.modules.cpp files:
        String repoFolder = NativePath::join(ctx.repoRootFolder, exRepo.repoName);
        Array<String> relModDefPaths;
        for (const ModuleDefinitionFile& modDefFile : exRepo.modDefFiles) {
            relModDefPaths.append(NativePath::makeRelative(repoFolder, modDefFile.absPath));
        }
        target->sourceFiles.append({repoFolder, std::move(relModDefPaths)});
    }
    target->sourceFiles.append({NativePath::join(ctx.dllBuildFolder, "codegen"),
                                {exRepo.repoName + "_Instantiators.cpp"}});
    target->sourceFiles.append(
        {NativePath::join(ctx.backedUpSrcFolder, "runtime/ply-runtime/memory"), {"Heap.cpp"}});
    target->addIncludeDir(Visibility::Private,
                          NativePath::join(PLY_BUILD_FOLDER, "codegen/ply-platform"));
    target->addIncludeDir(Visibility::Private, NativePath::join(ctx.backedUpSrcFolder, "platform"));
    target->addIncludeDir(Visibility::Private, NativePath::join(ctx.backedUpSrcFolder, "runtime"));
    target->addIncludeDir(Visibility::Private, NativePath::join(ctx.backedUpSrcFolder, "reflect"));
    target->addIncludeDir(Visibility::Private,
                          NativePath::join(ctx.backedUpSrcFolder, "build/target"));
    target->addIncludeDir(Visibility::Private,
                          NativePath::join(ctx.backedUpSrcFolder, "build/provider"));
    target->addIncludeDir(Visibility::Private,
                          NativePath::join(ctx.backedUpSrcFolder, "build/repo"));
    target->addIncludeDir(Visibility::Private,
                          NativePath::join(ctx.backedUpSrcFolder, "build/common"));
    target->setPreprocessorDefinition(Visibility::Private, "PLY_BUILD_IMPORTING", "1");
    target->setPreprocessorDefinition(Visibility::Private, "PLY_DLL_IMPORTING", "1");
#if PLY_TARGET_WIN32
    target->libs.append(NativePath::join(PLY_BUILD_FOLDER, "build/Debug/plytool.lib"));
#endif
    return target;
}

InstantiatedDLLs buildInstantiatorDLLs(bool force) {
    PLY_ASSERT(NativeToolchain.generator);
    PLY_ASSERT(DefaultNativeConfig);

    GenerateDLLContext ctx;
    ctx.repoRootFolder = NativePath::join(PLY_WORKSPACE_FOLDER, "repos/");
    ctx.dllBuildFolder = NativePath::join(PLY_WORKSPACE_FOLDER, "data/build/bootstrap/DLLs/");
    ctx.backedUpSrcFolder = NativePath::join(PLY_WORKSPACE_FOLDER, "repos/plywood/src/");
#ifdef PLY_SRC_FOLDER
    ctx.backedUpSrcFolder = NativePath::normalize(PLY_SRC_FOLDER, "");
#endif
    String signaturePath = NativePath::join(ctx.dllBuildFolder, "signature.pylon");

    // Load existing signature.pylon
    DLLSignature dllSig;
    bool mustBuild = !dllSig.load(signaturePath);
    if (force) {
        mustBuild = true;
    }

    // Visit all repo folders
    u128 moduleDefSignature = 0;
    Array<ExtractedRepo> exRepos;
    for (const DirectoryEntry& entry : FileSystem::native()->listDir(ctx.repoRootFolder, 0)) {
        if (!entry.isDir)
            continue;

        // This is a separate repo.
        // Recursively find all files named *.modules.cpp:
        String repoFolder = NativePath::join(ctx.repoRootFolder, entry.name);
        Array<ModuleDefinitionFile> modDefFiles;
        for (const WalkTriple& triple : FileSystem::native()->walk(repoFolder)) {
            for (const WalkTriple::FileInfo& file : triple.files) {
                if (file.name == "Instantiators.inl") {
                    // FIXME: Remove this later
                    ErrorHandler::log(
                        ErrorHandler::Error,
                        String::format("Please rename '{}' to a filename that ends with "
                                       "'.modules.cpp' (and make sure it contains the line "
                                       "'#include <ply-build-repo/Module.h>')'\n",
                                       NativePath::join(triple.dirPath, file.name)));
                } else if (file.name.endsWith(".modules.cpp")) {
                    // Found one. Add it to the signature.
                    String absPath = NativePath::join(triple.dirPath, file.name);
                    {
                        FileStatus stat = FileSystem::native()->getFileStatus(absPath);
                        Hash128 h;
                        h.append(absPath.bufferView());
                        h.append({&stat.fileSize, sizeof(stat.fileSize)});
                        h.append({&stat.modificationTime, sizeof(stat.modificationTime)});
                        moduleDefSignature += h.get();
                    }

                    // Next, scan it and find all instantiator functions.
                    ModuleDefinitionFile modDefFile;
                    modDefFile.absPath = std::move(absPath);
                    modDefFiles.append(std::move(modDefFile));
                }
            }
        }

        if (!modDefFiles.isEmpty()) {
            ExtractedRepo& exRepo = exRepos.append();
            exRepo.repoName = entry.name;
            exRepo.modDefFiles = std::move(modDefFiles);
        }
    }

    // Compare signatures
    if (!mustBuild) {
        if (parseSignatureString(dllSig.signature) != moduleDefSignature) {
            mustBuild = true;
        }
    }

    InstantiatedDLLs result;
    result.signature = moduleDefSignature;
    for (const ExtractedRepo& exRepo : exRepos) {
        // Add to return value
        InstantiatedDLL& idll = result.dlls.append();
        idll.repoName = exRepo.repoName;
        idll.dllPath = getTargetOutputPath(BuildTargetType::DLL, exRepo.repoName + "_Instantiators",
                                           ctx.dllBuildFolder, DefaultNativeConfig);
    }

    if (mustBuild) {
        ErrorHandler::log(ErrorHandler::Info, "Initializing repo registry...\n");

        Array<Owned<BuildTarget>> ownedTargets;
        CMakeBuildFolder cbf;
        cbf.solutionName = "DLLs";
        cbf.absPath = ctx.dllBuildFolder;

        for (ExtractedRepo& exRepo : exRepos) {
            for (ModuleDefinitionFile& modDefFile : exRepo.modDefFiles) {
                // Extract instantiators from module definition file
                extractInstantiatorFunctions(&modDefFile);
            }
            // Create a DLL target for this repo
            BuildTarget* target = ownedTargets.append(createDLLTarget(ctx, exRepo));
            cbf.targets.append(target);
        }

        // Generate build system
        StringWriter sw;
        writeCMakeLists(&sw, &cbf);
        String cmakeListsPath = NativePath::join(ctx.dllBuildFolder, "CMakeLists.txt");
        FSResult result = FileSystem::native()->makeDirsAndSaveTextIfDifferent(
            cmakeListsPath, sw.moveToString(), TextFormat::platformPreference());
        if (result != FSResult::OK && result != FSResult::Unchanged) {
            ErrorHandler::log(ErrorHandler::Error,
                              String::format("Can't write '{}'\n", cmakeListsPath));
            return {};
        }
        Tuple<s32, String> cmakeResult = generateCMakeProject(
            ctx.dllBuildFolder, NativeToolchain, DefaultNativeConfig,
            [&](StringView errMsg) { ErrorHandler::log(ErrorHandler::Error, errMsg); });
        if (cmakeResult.first != 0) {
            ErrorHandler::log(
                ErrorHandler::Error,
                StringView{"Failed to generate build system for instantiator DLLs:\n"} +
                    cmakeResult.second);
            return {};
        }

        // Build DLLs
        cmakeResult =
            buildCMakeProject(ctx.dllBuildFolder, NativeToolchain, DefaultNativeConfig, {}, true);
        if (cmakeResult.first != 0) {
            ErrorHandler::log(ErrorHandler::Error,
                              StringView{"Failed to build instantiator DLLs:\n"} +
                                  cmakeResult.second);
            return {};
        }

        // Save signature
        dllSig.signature = signatureToString(moduleDefSignature);
        dllSig.save(signaturePath);
    }

    // Success!
    return result;
}

} // namespace build
} // namespace ply

#include "codegen/BuildInstantiatorDLLs.inl" //%%
