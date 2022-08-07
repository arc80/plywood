#include <ply-build-repo/Module.h>

// [ply module="buildSteps"]
void module_buildSteps(ModuleArgs* args) {
    args->addSourceFiles("buildSteps", false);
    args->addIncludeDir(Visibility::Public, ".");
    args->addTarget(Visibility::Public, "runtime");
    args->addTarget(Visibility::Public, "pylon");
}
