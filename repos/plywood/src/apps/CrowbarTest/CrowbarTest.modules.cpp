/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repo/Module.h>

// [ply module="CrowbarTest"]
void module_CrowbarTest(ModuleArgs* args) {
    args->buildTarget->targetType = BuildTargetType::EXE;
    args->addSourceFiles(".", false);
    args->addTarget(Visibility::Private, "crowbar");
    args->addTarget(Visibility::Private, "test");
}
