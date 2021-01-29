/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repo/Module.h>

// [ply module="math"]
void module_ply_math(ModuleArgs* args) {
    args->addSourceFiles("math/ply-math");
    args->addIncludeDir(Visibility::Public, "math");
    args->addTarget(Visibility::Public, "platform");
}

// [ply module="math-serial"]
void module_ply_mathSerial(ModuleArgs* args) {
    args->addSourceFiles("serial/ply-math-serial");
    args->addIncludeDir(Visibility::Public, "serial");
    args->addTarget(Visibility::Public, "math");
    args->addTarget(Visibility::Public, "reflect");
}

// [ply module="math-tests"]
void module_ply_mathTests(ModuleArgs* args) {
    args->buildTarget->targetType = BuildTargetType::ObjectLib;
    args->addSourceFiles("tests");
    args->addTarget(Visibility::Private, "math");
    args->addTarget(Visibility::Private, "test");
}
