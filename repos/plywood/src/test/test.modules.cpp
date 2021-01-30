/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repo/Module.h>

// [ply module="test"]
void module_ply_test(ModuleArgs* args) {
    args->addSourceFiles("test/ply-test");
    args->addIncludeDir(Visibility::Public, "test");
    args->addTarget(Visibility::Public, "runtime");
}

// [ply module="PlywoodTests"]
void module_PlywoodTests(ModuleArgs* args) {
    args->buildTarget->targetType = BuildTargetType::EXE;
    args->addSourceFiles("PlywoodTests");
    args->addTarget(Visibility::Private, "test");
    args->addTarget(Visibility::Private, "math-tests");
    args->addTarget(Visibility::Private, "runtime-tests");
}
