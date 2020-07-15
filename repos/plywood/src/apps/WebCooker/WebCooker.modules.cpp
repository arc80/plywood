/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repo/Module.h>

// [ply module="WebCooker"]
void module_WebCooker(ModuleArgs* args) {
    args->buildTarget->targetType = BuildTargetType::EXE;
    args->addSourceFiles(".");
    args->addIncludeDir(Visibility::Private, ".");
    args->addTarget(Visibility::Private, "pylon-reflect");
    args->addTarget(Visibility::Private, "web-sass");
    args->addTarget(Visibility::Private, "web-documentation");
    args->addTarget(Visibility::Private, "cook");
    args->addTarget(Visibility::Private, "web-cook-docs");
}

