/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repo/Module.h>

// [ply module="ply"]
void module_ply(ModuleArgs* args) {
    args->buildTarget->targetType = BuildTargetType::EXE;
    args->addSourceFiles(".");
    args->addIncludeDir(Visibility::Private, ".");
    args->addTarget(Visibility::Private, "crowbar");
    args->addTarget(Visibility::Private, "build-folder");
    args->addTarget(Visibility::Private, "cpp");
    args->addTarget(Visibility::Private, "pylon-reflect");
    args->addTarget(Visibility::Private, "buildSteps");
}
