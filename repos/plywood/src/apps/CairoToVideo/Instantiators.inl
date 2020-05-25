// ply instantiate CairoToVideo
void inst_CairoToVideo(TargetInstantiatorArgs* args) {
    args->buildTarget->targetType = BuildTargetType::EXE;
    args->addSourceFiles(".", false);
    args->addIncludeDir(Visibility::Private, ".");
    args->addTarget(Visibility::Private, "runtime");
    args->addTarget(Visibility::Private, "math");
    args->addTarget(Visibility::Private, "codec");
    args->addTarget(Visibility::Private, "image-cairo");
}
