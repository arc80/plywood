#include <ply-build-repo/Module.h>

// [ply module="cpp"]
void module_ply_cpp(ModuleArgs* args) {
    args->addSourceFiles("ply-cpp");
    args->addIncludeDir(Visibility::Public, ".");
    args->addTarget(Visibility::Public, "reflect");
}
