// ply instantiate cook
void inst_ply_cook(TargetInstantiatorArgs* args) {
    args->addSourceFiles("ply-cook");
    args->addIncludeDir(Visibility::Public, ".");
    args->addTarget(Visibility::Public, "reflect");
}
