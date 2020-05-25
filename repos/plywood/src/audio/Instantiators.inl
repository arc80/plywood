// ply instantiate audio-primitives
void inst_audioPrimitives(TargetInstantiatorArgs* args) {
    args->addSourceFiles("primitives/audio-primitives");
    args->addIncludeDir(Visibility::Public, "primitives");
    args->addTarget(Visibility::Public, "reflect");
}
