#include <ply-build-repo/Module.h>

// [ply module="HelloWorld"]
void module_HelloWorld(ModuleArgs* args) {
    args->buildTarget->targetType = BuildTargetType::EXE;
    args->addSourceFiles(".", false);
    args->addIncludeDir(Visibility::Private, ".");
    args->addTarget(Visibility::Private, "runtime");
}
