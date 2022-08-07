/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repo/Module.h>

// [ply module="CairoToVideo"]
void module_CairoToVideo(ModuleArgs* args) {
    args->buildTarget->targetType = BuildTargetType::EXE;
    args->addSourceFiles(".", false);
    args->addIncludeDir(Visibility::Private, ".");
    args->addTarget(Visibility::Private, "runtime");
    args->addTarget(Visibility::Private, "math");
    args->addTarget(Visibility::Private, "codec");
    args->addTarget(Visibility::Private, "image-cairo");
}
