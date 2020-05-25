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
    args->addTarget(Visibility::Public, "build-common");
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

