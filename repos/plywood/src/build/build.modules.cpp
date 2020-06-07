#include <ply-build-repo/Module.h>

// ply instantiate build-common
void inst_ply_build_common(TargetInstantiatorArgs* args) {
    args->buildTarget->dynamicLinkPrefix = "PLY_BUILD";
    args->addSourceFiles("common/ply-build-common");
    args->addIncludeDir(Visibility::Public, "common");
    args->addTarget(Visibility::Public, "reflect");
}

// ply instantiate build-target
void inst_ply_build_target(TargetInstantiatorArgs* args) {
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
    "{}",
}};
)",
            cmakeOptions->generator, cmakeOptions->platform, cmakeOptions->toolset,
            cmakeOptions->buildType);
        FileSystem::native()->makeDirsAndSaveTextIfDifferent(
            NativePath::join(args->projInst->env->buildFolderPath,
                             "codegen/ply-build-target/ply-build-target/NativeToolchain.inl"),
            nativeToolchainFile, TextFormat::platformPreference());
    }
}

// ply instantiate build-provider
void inst_ply_build_provider(TargetInstantiatorArgs* args) {
    args->buildTarget->dynamicLinkPrefix = "PLY_BUILD";
    args->addSourceFiles("provider/ply-build-provider");
    args->addIncludeDir(Visibility::Public, "provider");
    args->addTarget(Visibility::Public, "build-common");
    args->addTarget(Visibility::Private, "pylon-reflect");
}

// ply instantiate build-repo
void inst_ply_build_repo(TargetInstantiatorArgs* args) {
    args->buildTarget->dynamicLinkPrefix = "PLY_BUILD";
    args->addSourceFiles("repo/ply-build-repo");
    args->addIncludeDir(Visibility::Public, "repo");
    args->addTarget(Visibility::Public, "build-target");
    args->addTarget(Visibility::Public, "build-provider");
    args->addTarget(Visibility::Private, "pylon-reflect");
    args->addTarget(Visibility::Private, "cpp");
}

// ply instantiate build-folder
void inst_ply_build_folder(TargetInstantiatorArgs* args) {
    args->buildTarget->dynamicLinkPrefix = "PLY_BUILD";
    args->addSourceFiles("folder/ply-build-folder");
    args->addIncludeDir(Visibility::Public, "folder");
    args->addTarget(Visibility::Public, "build-repo");
    args->addTarget(Visibility::Private, "pylon-reflect");
}
