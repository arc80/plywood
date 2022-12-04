/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repo/Module.h>

// [ply module="plytool"]
void module_plytool(ModuleArgs* args) {
    args->buildTarget->targetType = BuildTargetType::EXE;
    args->addSourceFiles("plytool");
    args->addIncludeDir(Visibility::Private, "plytool");
    args->addTarget(Visibility::Private, "build-folder");
    args->addTarget(Visibility::Private, "cpp");
    args->addTarget(Visibility::Private, "pylon-reflect");
    args->addTarget(Visibility::Private, "plytool-client");
    args->addTarget(Visibility::Private, "build-repository");
}

// [ply module="plytool-client"]
void module_plytool_client(ModuleArgs* args) {
    args->addSourceFiles("plytool-client/plytool-client");
    args->addIncludeDir(Visibility::Public, "plytool-client");
    args->addTarget(Visibility::Public, "reflect");
}
