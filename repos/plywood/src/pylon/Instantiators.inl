// ply instantiate pylon
void inst_pylon(TargetInstantiatorArgs* args) {
    args->addSourceFiles("pylon/pylon");
    args->addIncludeDir(Visibility::Public, "pylon");
    args->addTarget(Visibility::Public, "runtime");
}

// ply instantiate pylon-reflect
void inst_pylonReflect(TargetInstantiatorArgs* args) {
    args->addSourceFiles("reflect/pylon-reflect");
    args->addIncludeDir(Visibility::Public, "reflect");
    args->addTarget(Visibility::Public, "pylon");
    args->addTarget(Visibility::Public, "reflect");
}
