/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repo/Module.h>

// [ply module="cook"]
void module_ply_cook(ModuleArgs* args) {
    args->addSourceFiles("ply-cook");
    args->addIncludeDir(Visibility::Public, ".");
    args->addTarget(Visibility::Public, "reflect");
}
