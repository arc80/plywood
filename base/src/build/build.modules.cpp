/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repo/Module.h>

// [ply module="build-common"]
void module_ply_build_common(ModuleArgs* args) {
    args->buildTarget->dynamicLinkPrefix = "PLY_BUILD";
    args->addSourceFiles("common/ply-build-common");
    args->addIncludeDir(Visibility::Public, "common");
    args->addTarget(Visibility::Public, "reflect");
}

// [ply module="build-target"]
void module_ply_build_target(ModuleArgs* args) {
    args->buildTarget->dynamicLinkPrefix = "PLY_BUILD";
    args->addSourceFiles("target/ply-build-target");
    args->addIncludeDir(Visibility::Public, "target");
    args->addIncludeDir(Visibility::Private, NativePath::join(args->projInst->env->buildFolderPath,
                                                              "codegen/ply-build-target"));
    args->addTarget(Visibility::Public, "build-common");

    if (args->projInst->env->isGenerating) {
        // Write codegen/ply-build-target/ply-build-target/NativeToolchain.inl
        // Note: If we ever cross-compile this module, the NativeToolchain will have to be different
        const CMakeGeneratorOptions* cmakeOptions = args->projInst->env->cmakeOptions;
        PLY_ASSERT(cmakeOptions);
        String nativeToolchainFile = String::format(
            R"(CMakeGeneratorOptions NativeToolchain = {{
    "{}",
    "{}",
    "{}",
    "",
}};
String DefaultNativeConfig = "{}";
)",
            cmakeOptions->generator, cmakeOptions->platform, cmakeOptions->toolset,
            args->projInst->env->config);
        FileSystem::native()->makeDirsAndSaveTextIfDifferent(
            NativePath::join(args->projInst->env->buildFolderPath,
                             "codegen/ply-build-target/ply-build-target/NativeToolchain.inl"),
            nativeToolchainFile, TextFormat::platformPreference());
    }
}

// [ply module="build-provider"]
void module_ply_build_provider(ModuleArgs* args) {
    args->buildTarget->dynamicLinkPrefix = "PLY_BUILD";
    args->addSourceFiles("provider/ply-build-provider");
    args->addIncludeDir(Visibility::Public, "provider");
    args->addTarget(Visibility::Public, "build-common");
    args->addTarget(Visibility::Private, "pylon-reflect");
}

// [ply module="build-repo"]
void module_ply_build_repo(ModuleArgs* args) {
    args->buildTarget->dynamicLinkPrefix = "PLY_BUILD";
    args->addSourceFiles("repo/ply-build-repo");
    args->addIncludeDir(Visibility::Public, "repo");
    args->addTarget(Visibility::Public, "build-target");
    args->addTarget(Visibility::Public, "build-provider");
    args->addTarget(Visibility::Private, "pylon-reflect");
    args->addTarget(Visibility::Private, "cpp");
}

// [ply module="build-folder"]
void module_ply_build_folder(ModuleArgs* args) {
    args->buildTarget->dynamicLinkPrefix = "PLY_BUILD";
    args->addSourceFiles("folder/ply-build-folder");
    args->addIncludeDir(Visibility::Public, "folder");
    args->addTarget(Visibility::Public, "build-repo");
    args->addTarget(Visibility::Public, "build-repository");
    args->addTarget(Visibility::Private, "pylon-reflect");
}

// [ply module="build-repository"]
void module_ply_build_repository(ModuleArgs* args) {
    args->buildTarget->dynamicLinkPrefix = "PLY_BUILD";
    args->addSourceFiles("repository/ply-build-repository");
    args->addIncludeDir(Visibility::Public, "repository");
    args->addTarget(Visibility::Public, "reflect");
    args->addTarget(Visibility::Public, "buildSteps");
    args->addTarget(Visibility::Public, "crowbar");
    args->addTarget(Visibility::Public, "build-common");
}
