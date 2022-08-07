/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repo/Module.h>

// [ply module="pylon"]
void module_pylon(ModuleArgs* args) {
    args->buildTarget->dynamicLinkPrefix = "PYLON";
    args->addSourceFiles("pylon/pylon");
    args->addIncludeDir(Visibility::Public, "pylon");
    args->addTarget(Visibility::Public, "runtime");
}

// [ply module="pylon-reflect"]
void module_pylonReflect(ModuleArgs* args) {
    args->addSourceFiles("reflect/pylon-reflect");
    args->addIncludeDir(Visibility::Public, "reflect");
    args->addTarget(Visibility::Public, "pylon");
    args->addTarget(Visibility::Public, "reflect");
}
