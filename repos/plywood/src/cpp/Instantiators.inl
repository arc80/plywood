// ply instantiate cpp
void inst_ply_cpp(TargetInstantiatorArgs* args) {
    args->addSourceFiles("ply-cpp");
    args->addIncludeDir(Visibility::Public, ".");
    args->addTarget(Visibility::Public, "reflect");
}
