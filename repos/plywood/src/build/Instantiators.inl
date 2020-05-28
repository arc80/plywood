// ply instantiate build-common PLY_BUILD
void inst_ply_build_common(TargetInstantiatorArgs* args) {
    args->addSourceFiles("common/ply-build-common");
    args->addIncludeDir(Visibility::Public, "common");
    args->addTarget(Visibility::Public, "reflect");
}

// ply instantiate build-target PLY_BUILD
void inst_ply_build_target(TargetInstantiatorArgs* args) {
    args->addSourceFiles("target/ply-build-target");
    args->addIncludeDir(Visibility::Public, "target");
    args->addIncludeDir(Visibility::Private, NativePath::join(args->projInst->env->buildFolderPath,
                                                              "codegen/ply-build-target"));
    args->addTarget(Visibility::Public, "build-common");

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

// ply instantiate build-provider PLY_BUILD
void inst_ply_build_provider(TargetInstantiatorArgs* args) {
    args->addSourceFiles("provider/ply-build-provider");
    args->addIncludeDir(Visibility::Public, "provider");
    args->addTarget(Visibility::Public, "build-common");
    args->addTarget(Visibility::Private, "pylon-reflect");
}

// ply instantiate build-repo PLY_BUILD
void inst_ply_build_repo(TargetInstantiatorArgs* args) {
    args->addSourceFiles("repo/ply-build-repo");
    args->addIncludeDir(Visibility::Public, "repo");
    args->addTarget(Visibility::Public, "build-target");
    args->addTarget(Visibility::Public, "build-provider");
    args->addTarget(Visibility::Private, "pylon-reflect");
    args->addTarget(Visibility::Private, "cpp");
}

// ply instantiate build-folder PLY_BUILD
void inst_ply_build_folder(TargetInstantiatorArgs* args) {
    args->addSourceFiles("folder/ply-build-folder");
    args->addIncludeDir(Visibility::Public, "folder");
    args->addTarget(Visibility::Public, "build-repo");
    args->addTarget(Visibility::Private, "pylon-reflect");
}
